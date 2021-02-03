#include "multispec_functions.H"
#include "common_functions.H"

// FIXME: Fill ghost cells

void DiffusiveMassFluxdiv(const MultiFab& rho,
			  const MultiFab& rhotot,
			  const MultiFab& molarconc,
			  const MultiFab& rhoWchi,
			  const MultiFab& Gamma,
			  MultiFab& diff_mass_fluxdiv,
			  std::array< MultiFab, AMREX_SPACEDIM >& diff_mass_flux,
			  const Geometry& geom)
{

    BL_PROFILE_VAR("DiffusiveMassFluxdiv()",DiffusiveMassFluxdiv);

    // compute the face-centered flux (each direction: cells+1 faces while 
    // cells contain interior+2 ghost cells) 
    DiffusiveMassFlux(rho,rhotot,molarconc,rhoWchi,Gamma,diff_mass_flux,geom);
    
    // compute divergence of determinstic flux 
    ComputeDiv(diff_mass_fluxdiv,diff_mass_flux,0,0,nspecies,geom,0);  // increment = 0

}

void DiffusiveMassFlux(const MultiFab& rho,
		       const MultiFab& rhotot,
		       const MultiFab& molarconc,
		       const MultiFab& rhoWchi,
		       const MultiFab& Gamma,
		       std::array< MultiFab, AMREX_SPACEDIM >& diff_mass_flux,
		       const Geometry& geom)
{

    BL_PROFILE_VAR("DiffusiveMassFlux()",DiffusiveMassFlux);

    int i;
    BoxArray ba = rho.boxArray();
    DistributionMapping dmap = rho.DistributionMap();
    int nspecies = rho.nComp();
    int nspecies2 = nspecies*nspecies;

    const Real* dx = geom.CellSize();

    // build local face-centered multifab with nspecies^2 component, zero ghost cells 
    std::array< MultiFab, AMREX_SPACEDIM > rhoWchi_face;    // rho*W*chi*Gamma
    std::array< MultiFab, AMREX_SPACEDIM > Gamma_face;      // Gamma-matrix

    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        rhoWchi_face[d].define(convert(ba,nodal_flag_dir[d]), dmap, nspecies2, 0);
        Gamma_face[d]  .define(convert(ba,nodal_flag_dir[d]), dmap, nspecies2, 0);
    }

    // compute face-centered rhoWchi from cell-centered values 
    AverageCCToFace(rhoWchi, rhoWchi_face, 0, nspecies2, SPEC_BC_COMP, geom);

    // calculate face-centrered grad(molarconc) 
    ComputeGrad(molarconc, diff_mass_flux, 0, 0, nspecies, SPEC_BC_COMP, geom);

    // compute face-centered Gama from cell-centered values 
    AverageCCToFace(Gamma, Gamma_face, 0, nspecies2, SPEC_BC_COMP, geom);

    // compute Gama*grad(molarconc): Gama is nspecies^2 matrix; grad(x) is
    // nspecies component vector 
    for(i=0; i<AMREX_SPACEDIM; i++) {
      MatvecMul(diff_mass_flux[i], Gamma_face[i]);
    }

    if (is_nonisothermal) {
        //
        //
        //
    }

    if (barodiffusion_type > 0) {
        //
        //
        //
    }

    // compute -rhoWchi * (Gamma*grad(x) + ... ) on faces
    for(i=0;i<AMREX_SPACEDIM;i++) {
      MatvecMul(diff_mass_flux[i], rhoWchi_face[i]);
    }

    // If there are walls with zero-flux boundary conditions
    if (is_nonisothermal) {
        ZeroEdgevalWalls(diff_mass_flux, geom, 0, nspecies);
    }

    //correct fluxes to ensure mass conservation to roundoff
    if (correct_flux==1 && (nspecies > 1)) {
        CorrectionFlux(rho,rhotot,diff_mass_flux);
    }

}

void ComputeHigherOrderTerm(const MultiFab& molarconc,
                            std::array<MultiFab,AMREX_SPACEDIM>& diff_mass_flux,
                            const Geometry& geom)
{
    
    BoxArray ba = molarconc.boxArray();
    DistributionMapping dmap = molarconc.DistributionMap();
    const GpuArray<Real, AMREX_SPACEDIM> dx = geom.CellSizeArray();

    if (dx[0] != dx[1]) {
        Abort("ComputeHigherOrderTerm needs dx=dy=dz");
    }
#if (AMREX_SPACEDIM == 3)
    if (dx[0] != dx[2]) {
        Abort("ComputeHigherOrderTerm needs dx=dy=dz");
    }
#endif

    MultiFab laplacian(ba, dmap, nspecies, 1);

    Real dxinv = 1./dx[0];
    Real twodxinv = 2.*dxinv;
    Real sixth = 1./6.;
    
    for ( MFIter mfi(laplacian,TilingIfNotGPU()); mfi.isValid(); ++mfi ) {
        
        const Box& bx = mfi.growntilebox(1);

        const Array4<Real>& lap = laplacian.array(mfi);
        
        const Array4<Real const>& phi = molarconc.array(mfi);

        amrex::ParallelFor(bx, nspecies, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) noexcept
        {
#if (AMREX_SPACEDIM == 2)
            lap(i,j,k,n) = ( phi(i+1,j-1,k,n)-2.*phi(i,j-1,k,n)+phi(i-1,j-1,k,n) + phi(i-1,j+1,k,n)-2.*phi(i-1,j,k,n)+phi(i-1,j-1,k,n) ) * (sixth*dxinv*dxinv)
                + 4.*( phi(i+1,j,k,n)-2.*phi(i,j,k,n)+phi(i-1,j,k,n) + phi(i,j+1,k,n)-2.*phi(i,j,k,n)+phi(i,j-1,k,n) ) * (sixth*dxinv*dxinv)
                + ( phi(i+1,j+1,k,n)-2.*phi(i,j+1,k,n)+phi(i-1,j+1,k,n) + phi(i+1,j+1,k,n)-2.*phi(i+1,j,k,n)+phi(i+1,j-1,k,n) ) * (sixth*dxinv*dxinv);
#elif (AMREX_SPACEDIM == 3)

#endif
        });
    }

for ( MFIter mfi(molarconc,TilingIfNotGPU()); mfi.isValid(); ++mfi ) {

        AMREX_D_TERM(const Array4<Real> & fluxx = diff_mass_flux[0].array(mfi);,
                     const Array4<Real> & fluxy = diff_mass_flux[1].array(mfi);,
                     const Array4<Real> & fluxz = diff_mass_flux[2].array(mfi););

        const Array4<Real const>& phi = molarconc.array(mfi);
        
        const Array4<Real>& lap = laplacian.array(mfi);
        
        AMREX_D_TERM(const Box & bx_x = mfi.nodaltilebox(0);,
                     const Box & bx_y = mfi.nodaltilebox(1);,
                     const Box & bx_z = mfi.nodaltilebox(2););

#if (AMREX_SPACEDIM == 2)
        
        amrex::ParallelFor(bx_x, nspecies, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) noexcept
        {
            Real phiavg = 0.5*(amrex::max(amrex::min(phi(i,j,k,n),1.),0.) + amrex::max(amrex::min(phi(i-1,j,k,n),1.),0.));
            fluxx(i,j,k,n) = fluxx(i,j,k,n) - 0.5* kc_tension*phiavg*( lap(i,j,k,n)-lap(i-1,j,k,n) ) * dxinv;
        },
                           bx_y, nspecies, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) noexcept
        {
            Real phiavg = 0.5*(amrex::max(amrex::min(phi(i,j,k,n),1.),0.) + amrex::max(amrex::min(phi(i,j-1,k,n),1.),0.));
            fluxy(i,j,k,n) = fluxy(i,j,k,n) - 0.5* kc_tension*phiavg*( lap(i,j,k,n)-lap(i,j-1,k,n) ) * dxinv;
        });
        
#elif (AMREX_SPACEDIM == 3)
        
        
        amrex::ParallelFor(bx_x, nspecies, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) noexcept
        {
            
        },
                           bx_y, nspecies, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) noexcept
        {
            
        },
                           bx_z, nspecies, [=] AMREX_GPU_DEVICE (int i, int j, int k, int n) noexcept
        {
            
        });
                           
#endif
    }


}

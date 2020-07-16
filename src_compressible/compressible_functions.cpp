#include "compressible_functions.H"


void InitializeCompressibleNamespace() {

    BL_PROFILE_VAR("InitializeCompressibleNamespace()",InitializeCompressibleNamespace);

    bc_Yk.resize(MAX_SPECIES*LOHI*AMREX_SPACEDIM);
    bc_Xk.resize(MAX_SPECIES*LOHI*AMREX_SPACEDIM);

    initialize_compressible_namespace(bc_Yk.dataPtr(), bc_Xk.dataPtr(), &plot_means, &plot_vars);
}


void InitConsVar(MultiFab& cons, const MultiFab& prim,
                 const amrex::Geometry geom) {

    const Real* dx_host = geom.CellSize();
    const RealBox& realDomain = geom.ProbDomain();

    GpuArray<Real,AMREX_SPACEDIM> dx;
    GpuArray<Real,AMREX_SPACEDIM> reallo;
    GpuArray<Real,AMREX_SPACEDIM> realhi;
    GpuArray<Real,AMREX_SPACEDIM> center;

    for (int d=0; d<AMREX_SPACEDIM; ++d) {
        dx[d] = dx_host[d];
        reallo[d] = realDomain.lo(d);
        realhi[d] = realDomain.hi(d);
        center[d] = ( realhi[d] - reallo[d] ) / 2.;
    }
    
    // from namelist
    int nspecies_gpu = nspecies;
    Real Runiv_gpu = Runiv;
    int prob_type_gpu = prob_type;

    GpuArray<Real,MAX_SPECIES> molmass_gpu;
    for (int n=0; n<nspecies; ++n) {
        molmass_gpu[n] = molmass[n];
    }
    GpuArray<Real,MAX_SPECIES> hcv_gpu;
    for (int n=0; n<nspecies; ++n) {
        hcv_gpu[n] = hcv[n];
    }
    GpuArray<Real,MAX_SPECIES> grav_gpu;
    for (int n=0; n<nspecies; ++n) {
        grav_gpu[n] = grav[n];
    }
    GpuArray<Real,MAX_SPECIES> bc_Yk_y_lo_gpu;
    GpuArray<Real,MAX_SPECIES> bc_Yk_y_hi_gpu;
    
    for (int n=0; n<nspecies; ++n) {
        bc_Yk_y_lo_gpu[n] = bc_Yk[1 + n*LOHI*AMREX_SPACEDIM];
        bc_Yk_y_hi_gpu[n] = bc_Yk[1 + AMREX_SPACEDIM + n*LOHI*AMREX_SPACEDIM];
    }

    // local variables
    Real mach = 0.3;
    Real velscale = 30565.2*mach;

    Real hy = ( prob_hi[1] - prob_lo[1] ) / 3.;
    Real pi = acos(-1.);
    
    for ( MFIter mfi(cons); mfi.isValid(); ++mfi ) {
        const Array4<const Real> pu = prim.array(mfi);
        const Array4<      Real> cu = cons.array(mfi);

        const Box& bx = mfi.tilebox();

        amrex::ParallelFor(bx,
        [=] AMREX_GPU_DEVICE (int i, int j, int k) {

            GpuArray<Real,AMREX_SPACEDIM> itVec;
            GpuArray<Real,AMREX_SPACEDIM> pos;
            GpuArray<Real,AMREX_SPACEDIM> relpos;

            GpuArray<Real,MAX_SPECIES> massvec;

            AMREX_D_TERM(itVec[0] = (i+0.5)*dx[0]; ,
                         itVec[1] = (j+0.5)*dx[1]; ,
                         itVec[2] = (k+0.5)*dx[2]);
            
            for (int d=0; d<AMREX_SPACEDIM; ++d) {
                pos[d] = reallo[d] + itVec[d];
                relpos[d] = pos[d] - center[d];
            }

            // Total density must be pre-set
            
            if (prob_type_gpu == 2) { // Rayleigh-Taylor
                
                if (relpos[2] >= 0.) {
                    massvec[0] = 0.4;
                    massvec[1] = 0.4;
                    massvec[2] = 0.1;
                    massvec[3] = 0.1;
                } else {
                    massvec[0] = 0.1;
                    massvec[1] = 0.1;
                    massvec[2] = 0.4;
                    massvec[3] = 0.4;
                }

                Real pamb;
                GetPressureGas(pamb, massvec, cu(i,j,k,0), pu(i,j,k,4),
                               nspecies_gpu, Runiv_gpu, molmass_gpu);
                
                Real molmix = 0.;

                for (int l=0; l<nspecies_gpu; ++l) {
                    molmix = molmix + massvec[l]/molmass_gpu[l];
                }
                molmix = 1.0/molmix;
                Real rgasmix = Runiv_gpu/molmix;
                Real alpha = grav_gpu[2]/(rgasmix*pu(i,j,k,4));

                // rho = exponential in z-dir to init @ hydrostatic eqm.
                // must satisfy system: dP/dz = -rho*g & P = rhogasmix*rho*T
                // Assumes temp=const
                cu(i,j,k,0) = pamb*exp(alpha*pos[2])/(rgasmix*pu(i,j,k,4));
                
                for (int l=0; l<nspecies_gpu; ++l) {
                    cu(i,j,k,5+l) = cu(i,j,k,0)*massvec[l];
                }

                Real intEnergy;
                GetEnergy(intEnergy, massvec, pu(i,j,k,4), hcv_gpu, nspecies_gpu);

                cu(i,j,k,4) = cu(i,j,k,0)*intEnergy + 0.5*cu(i,j,k,0)*(pu(i,j,k,1)*pu(i,j,k,1) +
                                                                       pu(i,j,k,2)*pu(i,j,k,2) +
                                                                       pu(i,j,k,3)*pu(i,j,k,3));
            }

#if 0
            else if (prob_type_gpu == 3) { // diffusion barrier

                for (int l=0; l<nspecies_gpu; ++l) {



                    
                    
           Ygrad = (bc_Yk(2,2,l) - bc_Yk(2,1,l))/(realhi(2) - reallo(2))
           massvec(l) = Ygrad*pos(2) + bc_Yk(2,1,l)
           cu(i,j,k,5+l) = cu(i,j,k,1)*massvec(l)
        enddo

        call get_energy(intEnergy, massvec, pu(i,j,k,5))
        cu(i,j,k,5) = cu(i,j,k,1)*intEnergy + 0.5*cu(i,j,k,1)*(pu(i,j,k,2)**2 + &
             pu(i,j,k,3)**2 + pu(i,j,k,4)**2)

     elseif (prob_type_gpu .eq. 4) then ! Taylor Green Vortex

        x=itVec(1)
        y=itVec(2)
        z=itVec(3)

        cu(i,j,k,1) = 1.784e-3
        cu(i,j,k,2) =  velscale*cu(i,j,k,1)*sin(2.*pi*x/Lf)*cos(2.*pi*y/Lf)*cos(2.*pi*z/Lf)
        cu(i,j,k,3) = -velscale*cu(i,j,k,1)*cos(2.*pi*x/Lf)*sin(2.*pi*y/Lf)*cos(2.*pi*z/Lf)
        cu(i,j,k,4) = 0.
        pres = 1.01325d6+cu(i,j,k,1)*velscale**2*cos(2.*pi*x/Lf)*cos(4.*pi*y/Lf)*(cos(4.*pi*z/Lf)+2.)
        cu(i,j,k,5) = pres/(5./3.-1.) + 0.5*(cu(i,j,k,2)**2 + &
             cu(i,j,k,3)**2 + cu(i,j,k,4)**2) / cu(i,j,k,1)
        cu(i,j,k,6) = cu(i,j,k,1)
        cu(i,j,k,7) = 0.
        

     elseif (prob_type_gpu .eq. 5) then ! Taylor Green Vortex

       

        cu(i,j,k,1) = rho0
        cu(i,j,k,2) = 0
        cu(i,j,k,3) = 0
        cu(i,j,k,4) = 0
        if((prob_lo(2) + itVec(2)) < hy) then
          massvec = bc_Yk(1,1,1:2)
          call get_energy(intEnergy, massvec, t_lo(2));
          cu(i,j,k,5) = cu(i,j,k,1)*intEnergy
          cu(i,j,k,6) = cu(i,j,k,1)*bc_Yk(1,1,1)
          cu(i,j,k,7) = cu(i,j,k,1)*bc_Yk(1,1,2)
        elseif((prob_lo(2) + itVec(2)) < 2*hy) then
          massvec = bc_Yk(1,2,1:2)
          call get_energy(intEnergy, massvec, t_hi(2));
          cu(i,j,k,5) = cu(i,j,k,1)*intEnergy
          cu(i,j,k,6) = cu(i,j,k,1)*bc_Yk(1,2,1)
          cu(i,j,k,7) = cu(i,j,k,1)*bc_Yk(1,2,2)
        else
          massvec = bc_Yk(1,1,1:2)
          call get_energy(intEnergy, massvec, t_lo(2));
          cu(i,j,k,5) = cu(i,j,k,1)*intEnergy
          cu(i,j,k,6) = cu(i,j,k,1)*bc_Yk(1,1,1)
          cu(i,j,k,7) = cu(i,j,k,1)*bc_Yk(1,1,2)
        endif
      endif
#endif
              
              });
/*    
    // initialize conserved variables
    for ( MFIter mfi(cons); mfi.isValid(); ++mfi ) {
        const Box& bx = mfi.validbox();
        init_consvar(BL_TO_FORTRAN_BOX(bx),
                     BL_TO_FORTRAN_ANYD(cons[mfi]),
                     BL_TO_FORTRAN_ANYD(prim[mfi]),
                     dx, ZFILL(realDomain.lo()), ZFILL(realDomain.hi()));
    }
*/
    } // end MFIter

}

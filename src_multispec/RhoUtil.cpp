#include "common_functions.H"
#include "multispec_functions.H"

// compute rhotot from rho in VALID REGION
void ComputeRhotot(const MultiFab& rho,
		   MultiFab& rhotot,
                   int include_ghost) // include_ghost=0 by default
{
  
    BL_PROFILE_VAR("ComputeRhotot()",ComputeRhotot);

    int ng = (include_ghost == 1) ? rhotot.nGrow() : 0;

    if (include_ghost == 1) {
        int ng_r = rho.nGrow();
        if (ng > ng_r) {
            Abort("ComputeRhotot: rho needs as many ghost cells as rhotot");
        }
    }
    
    rhotot.setVal(0.0);
    for (int i=0; i<nspecies; i++) {
        MultiFab::Add(rhotot,rho,i,0,1,ng);
    }

}

void ConvertRhoCToC(MultiFab& rho, const MultiFab& rhotot, MultiFab& conc, int rho_to_c)
{
    Abort("ConvertRhoCToC: write me!");
    
    if (rho_to_c == 1) {
        // rho to conc - NO GHOST CELLS
        MultiFab::Copy(conc,rho,0,0,nspecies,0);
        for (int i=0; i<nspecies; ++i) {
            MultiFab::Divide(conc,rhotot,0,i,1,0);
        }

    }
    else if (rho_to_c == 0) {

        int ng = rho.nGrow();
        int ng_c = conc.nGrow();
        int ng_r = rhotot.nGrow();

        if (ng > ng_c || ng > ng_r) {
            Abort("ConvertRhoCToC: conc needs as many ghost cells as rho or rhotot");
        }
        
        // conc to rho - VALID + GHOST
        MultiFab::Copy(rho,conc,0,0,nspecies,ng);
        for (int i=0; i<nspecies; ++i) {
            MultiFab::Multiply(rho,rhotot,0,i,1,ng);
        }

    }
    else {
        Abort("ConvertRhoCToC: invalid rho_to_c");
    }
}

void FillRhoRhototGhost(MultiFab& rho, MultiFab& rhotot, const Geometry& geom) {

    BoxArray ba = rho.boxArray();
    DistributionMapping dmap = rho.DistributionMap();
    int ng = rho.nGrow();

    MultiFab conc(ba, dmap, nspecies, ng);

    // rho to conc - VALID REGION ONLY
    ConvertRhoCToC(rho,rhotot,conc,1);

    // fill conc ghost cells
    conc.FillBoundary(geom.periodicity());
    MultiFabPhysBC(conc,geom,0,nspecies,1);

    // fill rhotot ghost cells
    FillRhototGhost(rhotot,conc,geom);

    // conc to rho - INCLUDING GHOST CELLS
    ConvertRhoCToC(rho,rhotot,conc,0);    
}


void FillRhototGhost(MultiFab& rhotot_in, const MultiFab& conc_in, const Geometry& geom) {

    Abort("FillRhototGhost FINISHME");
    
    rhotot_in.FillBoundary(geom.periodicity());
                            
    if (geom.isAllPeriodic()) {
        return;
    }

    if (algorithm_type == 6) {
        MultiFabPhysBC(rhotot_in,geom,0,1,-1);
        return;
    }    

    // Physical Domain
    Box dom(geom.Domain());

    int ng = rhotot_in.nGrow();
    int ng_c = conc_in.nGrow();

    if (ng_c < 1 || ng < 1) {
        Abort("FillRhototGhost: rhotot and conc both need a ghost cell");
    }
    
    Vector<int> bc_lo(AMREX_SPACEDIM);
    Vector<int> bc_hi(AMREX_SPACEDIM);

    // compute mathematical boundary conditions
    BCPhysToMath(1,bc_lo,bc_hi);

    for (MFIter mfi(rhotot_in, TilingIfNotGPU()); mfi.isValid(); ++mfi) {

        // one ghost cell
        Box bx = mfi.growntilebox(ng);

        const Array4<Real>& rhotot = rhotot_in.array(mfi);
        const Array4<Real const>& conc = conc_in.array(mfi);

        //___________________________________________________________________________
        // Apply x-physbc to data

        // lo-x faces
        // bc_vel check is to see if we have a wall bc
        // bx/dom comparison is to see if the grid touches a wall

        int lo = dom.smallEnd(0);
        int hi = dom.bigEnd(0);
   
        if (bx.smallEnd(0) < lo) {
            if (bc_lo[0] == FOEXTRAP || bc_lo[0] == EXT_DIR) {
                amrex::ParallelFor(bx,[=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
                {
                    if (i < lo) {
                        Real rhoinv = 0.;
                        for (int n=0; n<nspecies; ++n) {
                            rhoinv += conc(lo-1,j,k,n)/rhobar[n];
                        }
                        rhotot(i,j,k) = 1./rhoinv;
                    }
                });
            }
        }
        
        if (bx.bigEnd(0) > hi) {
            if (bc_hi[0] == FOEXTRAP || bc_hi[0] == EXT_DIR) {
                amrex::ParallelFor(bx,[=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
                {
                    if (i > hi) {
                        Real rhoinv = 0.;
                        for (int n=0; n<nspecies; ++n) {
                            rhoinv += conc(hi+1,j,k,n)/rhobar[n];
                        }
                        rhotot(i,j,k) = 1./rhoinv;
                    }
                });
            }
        }

#if (AMREX_SPACEDIM >= 2)
        //___________________________________________________________________________
        // Apply y-physbc to data

        lo = dom.smallEnd(1);
        hi = dom.bigEnd(1);
        
        if (bx.smallEnd(1) < lo) {
            if (bc_lo[1] == FOEXTRAP || bc_lo[1] == EXT_DIR) {
                amrex::ParallelFor(bx,[=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
                {
                    if (j < lo) {
                        Real rhoinv = 0.;
                        for (int n=0; n<nspecies; ++n) {
                            rhoinv += conc(i,lo-1,k,n)/rhobar[n];
                        }
                        rhotot(i,j,k) = 1./rhoinv;
                    }
                });
            }
        }

        if (bx.bigEnd(1) > hi) {
            if (bc_hi[1] == FOEXTRAP || bc_hi[1] == EXT_DIR) {
                amrex::ParallelFor(bx,[=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
                {
                    if (j > hi) {
                        Real rhoinv = 0.;
                        for (int n=0; n<nspecies; ++n) {
                            rhoinv += conc(i,hi+1,k,n)/rhobar[n];
                        }
                        rhotot(i,j,k) = 1./rhoinv;
                    }
                });
            }
        }
#endif

#if (AMREX_SPACEDIM >= 3)
        //___________________________________________________________________________
        // Apply z-physbc to data

        lo = dom.smallEnd(2);
        hi = dom.bigEnd(2);
        
        if (bx.smallEnd(2) < lo) {
            if (bc_lo[2] == FOEXTRAP || bc_lo[2] == EXT_DIR) {
                amrex::ParallelFor(bx,[=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
                {
                    if (k < lo) {
                        Real rhoinv = 0.;
                        for (int n=0; n<nspecies; ++n) {
                            rhoinv += conc(i,j,lo-1,n)/rhobar[n];
                        }
                        rhotot(i,j,k) = 1./rhoinv;
                    }
                });
            }
        }

        if (bx.bigEnd(2) > hi) {
            if (bc_hi[2] == FOEXTRAP || bc_hi[2] == EXT_DIR) {
                amrex::ParallelFor(bx,[=] AMREX_GPU_DEVICE (int i, int j, int k) noexcept
                {
                    if (k > hi) {
                        Real rhoinv = 0.;
                        for (int n=0; n<nspecies; ++n) {
                            rhoinv += conc(i,j,hi+1,n)/rhobar[n];
                        }
                        rhotot(i,j,k) = 1./rhoinv;
                    }
                });
            }
        }
#endif
        
    } // end MFIter
}
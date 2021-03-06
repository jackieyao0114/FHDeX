#ifndef _multispec_functions_H_
#define _multispec_functions_H_

// used for binary diffusion coefficients
// needs to be MAX_SPECIES*(MAX_SPECIES-1)/2, MAX_SPECIES is in common_functions.H
#define MAX_ELEMENT 6

#include <AMReX_MultiFab.H>

#include "multispec_functions_F.H"
#include "multispec_namespace.H"

#include "common_functions.H"

#include "StochMassFlux.H"
#include "StochMomFlux.H"

using namespace multispec;
using namespace amrex;

/////////////////////////////////////////////////////////////////////////////////
// in multispec_functions.cpp

void InitializeMultispecNamespace();

/////////////////////////////////////////////////////////////////////////////////
// in ComputeDivReversibleStress.cpp
void ComputeDivReversibleStress(std::array<MultiFab,AMREX_SPACEDIM>& div_reversible_stress,
                                const MultiFab& rhotot_in,
                                MultiFab& rho_in,
                                const Geometry& geom);

/////////////////////////////////////////////////////////////////////////////////
// in ComputeMassFluxdiv.cpp

void ComputeMassFluxdiv(MultiFab& rho,
                        MultiFab& rhotot,
                        const MultiFab& Temp,
			MultiFab& diff_mass_fluxdiv,
                        MultiFab& stoch_mass_fluxdiv,
			std::array<MultiFab,AMREX_SPACEDIM>& diff_mass_flux,
                        std::array<MultiFab,AMREX_SPACEDIM>& stoch_mass_flux,
                        StochMassFlux& sMassFlux,
			const Real& dt, const Real& stage_time, const Geometry& geom,
                        Vector<Real>& weights,
                        MultiFab& charge,
                        std::array<MultiFab,AMREX_SPACEDIM>& grad_Epot,
                        MultiFab& Epot,
                        MultiFab& permittivity,
                        const int& zero_initial_Epot=1);

void ComputeHigherOrderTerm(const MultiFab& molarconc,
                            std::array<MultiFab,AMREX_SPACEDIM>& diff_mass_flux,
                            const Geometry& geom);

/////////////////////////////////////////////////////////////////////////////////
// in DiffusiveMassFlux.cpp

void DiffusiveMassFluxdiv(const MultiFab& rho,
			  const MultiFab& rhotot,
			  const MultiFab& molarconc,
			  const MultiFab& rhoWchi,
			  const MultiFab& Gamma,
			  MultiFab& diff_mass_fluxdiv,
			  std::array< MultiFab, AMREX_SPACEDIM >& diff_mass_flux,
			  const Geometry& geom);

void DiffusiveMassFlux(const MultiFab& rho,
		       const MultiFab& rhotot,
		       const MultiFab& molarconc,
		       const MultiFab& rhoWchi,
		       const MultiFab& Gamma,
		       std::array< MultiFab, AMREX_SPACEDIM >& diff_mass_flux,
		       const Geometry& geom);

/////////////////////////////////////////////////////////////////////////////////
// in ComputeMixtureProperties.cpp
void ComputeMixtureProperties(const MultiFab& rho,
			      const MultiFab& rhotot,
			      MultiFab& D_bar,
			      MultiFab& D_therm,
			      MultiFab& Hessian);

/////////////////////////////////////////////////////////////////////////////////
// in CorrectionFlux.cpp

void CorrectionFlux(const MultiFab& rho, const MultiFab& rhotot,
		    std::array< MultiFab, AMREX_SPACEDIM >& flux);

/////////////////////////////////////////////////////////////////////////////////
// in CorrectionFlux.cpp

void CorrectionFlux(const MultiFab& rho, const MultiFab& rhotot,
		    std::array< MultiFab, AMREX_SPACEDIM >& flux);

/////////////////////////////////////////////////////////////////////////////////
// in ElectroDiffusiveMassFluxdiv.cpp

void ElectroDiffusiveMassFluxdiv(const MultiFab& rho,
                                 const MultiFab& Temp,
                                 const MultiFab& rhoWchi,
                                 std::array< MultiFab, AMREX_SPACEDIM >& diff_mass_flux,
                                 MultiFab& diff_mass_fluxdiv,
                                 std::array< MultiFab, AMREX_SPACEDIM >& stoch_mass_flux,
                                 MultiFab& charge,
                                 std::array< MultiFab, AMREX_SPACEDIM >& grad_Epot,
                                 MultiFab& Epot,
                                 const MultiFab& permittivity,
                                 Real dt,
                                 int zero_initial_Epot,
                                 const Geometry& geom);

void ElectroDiffusiveMassFlux(const MultiFab& rho,
                              const MultiFab& Temp,
                              const MultiFab& rhoWchi,
                              std::array< MultiFab, AMREX_SPACEDIM >& electro_mass_flux,
                              std::array< MultiFab, AMREX_SPACEDIM >& diff_mass_flux,
                              std::array< MultiFab, AMREX_SPACEDIM >& stoch_mass_flux,
                              MultiFab& charge,
                              std::array< MultiFab, AMREX_SPACEDIM >& grad_Epot,
                              MultiFab& Epot,
                              const MultiFab& permittivity,
                              Real dt,
                              int zero_initial_Epot,
                              const Geometry& geom);

void LimitEMF(const MultiFab& rho_in,
              std::array< MultiFab, AMREX_SPACEDIM >& electro_mass_flux);

/////////////////////////////////////////////////////////////////////////////////
// in FluicCharge.cpp

void DotWithZ(const MultiFab& mf,
              MultiFab& mfdotz,
              int abs_z=0);

void DotWithZFace(std::array< const MultiFab, AMREX_SPACEDIM >& mf,
                  std::array< MultiFab, AMREX_SPACEDIM >& mfdotz,
                  int abs_0);

void ComputeChargeCoef(const MultiFab& rho_in,
                       const MultiFab& Temp_in,
                       MultiFab& charge_coef_in);

void EnforceChargeNeutrality();

void ImplicitPotentialCoef();

void ModifyS();

void ComputePermittivity();

void ComputeLorentzForce(std::array< MultiFab, AMREX_SPACEDIM >& Lorentz_force,
                         std::array< MultiFab, AMREX_SPACEDIM >& grad_Epot,
                         const MultiFab& permittivity,
                         const MultiFab& charge,
                         const Geometry& geom);

void ComputeE_ext(std::array< MultiFab, AMREX_SPACEDIM >& E_ext);

void ZeroEpsOnWall(std::array< MultiFab, AMREX_SPACEDIM >& beta);

/////////////////////////////////////////////////////////////////////////////////
// in InitialProjection.cpp

void InitialProjection(std::array< MultiFab, AMREX_SPACEDIM >& umac,
                       MultiFab& rho, MultiFab& rhotot,
                       MultiFab& diff_mass_fluxdiv,
                       MultiFab& stoch_mass_fluxdiv,
                       std::array< MultiFab, AMREX_SPACEDIM >& stoch_mass_flux,
                       StochMassFlux& sMassFlux,
                       const MultiFab& Temp, const MultiFab& eta,
                       const std::array< MultiFab, NUM_EDGE >& eta_ed,
                       const Real& dt, const Real& time, const Geometry& geom,
                       MultiFab& charge_old,
                       std::array<MultiFab,AMREX_SPACEDIM>& grad_Epot_old,
                       MultiFab& Epot,
                       MultiFab& permittivity);

/////////////////////////////////////////////////////////////////////////////////
// in MassFluxUtil.cpp

void ComputeMolconcMolmtot(const MultiFab& rho,
			   const MultiFab& rhotot,
			   MultiFab& molarconc,
			   MultiFab& molmtot);

void ComputeGamma(const MultiFab& molarconc,
		  const MultiFab& Hessian,
		  MultiFab& Gamma);

void ComputeRhoWChi(const MultiFab& rho,
		    const MultiFab& rhotot,
		    const MultiFab& molarconc,
		    MultiFab& rhoWchi,
		    const MultiFab& D_bar);

void ComputeZetaByTemp(const MultiFab& molarconc,
 		       const MultiFab& D_bar,
 		       const MultiFab& Temp,
 		       MultiFab& zeta_by_Temp,
 		       const MultiFab& D_therm);

void ComputeSqrtLonsagerFC(const MultiFab& rho,
                           const MultiFab& rhotot,
                           std::array< MultiFab, AMREX_SPACEDIM >& sqrtLonsager_fc,
                           const Geometry& geom);

/////////////////////////////////////////////////////////////////////////////////
// in MatvecMul.cpp

void MatvecMul(MultiFab& x,
	       const MultiFab& A);

/////////////////////////////////////////////////////////////////////////////////
// in MkDiffusiveMFluxdiv.cpp

void MkDiffusiveMFluxdiv(std::array<MultiFab, AMREX_SPACEDIM> & m_update,
                         const std::array<MultiFab, AMREX_SPACEDIM> & umac,
                         const MultiFab& eta,
                         const std::array<MultiFab, NUM_EDGE> & eta_ed,
                         const MultiFab& kappa,
                         const Geometry& geom,
                         const Real* dx,
                         const int& increment);

/////////////////////////////////////////////////////////////////////////////////
// in ProjectOntoEOS.cpp

void ProjectOntoEOS(MultiFab& rho_in);

/////////////////////////////////////////////////////////////////////////////////
// in RhoUtil.cpp

void RhototBCInit();

void ComputeRhotot(const MultiFab& rho, MultiFab& rhotot, int include_ghost=0);

void ConvertRhoCToC(MultiFab& rho, const MultiFab& rhotot, MultiFab& conc, int rho_to_c);

void FillRhoRhototGhost(MultiFab& rho, MultiFab& rhotot, const Geometry& geom);

void FillRhototGhost(MultiFab& rhotot_in, const MultiFab& conc_in, const Geometry& geom);

/////////////////////////////////////////////////////////////////////////////////
// Device calls from  MassFluxUtil.cpp

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void GetMolTot(const GpuArray<Real, MAX_SPECIES>& W,
                Real& molmtot)
{
    Real sumOverN = 0;

    for (int n=0; n<nspecies; ++n){
        sumOverN += W[n] / molmass[n];                
    }  

    molmtot = 1.0 / sumOverN; 
}  

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void ComputeMolconcMolmtotLocal( int nspecies_in,
                GpuArray<Real, MAX_SPECIES>& molmass_in,
                GpuArray<Real, MAX_SPECIES>& RhoN,
                Real rhotot_in,
                GpuArray<Real, MAX_SPECIES>& MolarConcN,
                Real& molmtot_in){

    GpuArray<Real, MAX_SPECIES> W;

    // calculate mass fraction and total molar mass (1/m=Sum(w_i/m_i))

    for (int n=0; n<nspecies_in; ++n){
        W[n] = RhoN[n] / rhotot_in;
    }

    GetMolTot(W, molmtot_in); 

    // calculate molar concentrations in each cell (x_i=m*w_i/m_i) 

    for (int n=0; n<nspecies_in; ++n){
        MolarConcN[n] = molmtot_in * W[n] / molmass[n];
    }
    
}

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void MatrixToZeros(int nspecies,
                Array2D<Real, 1, MAX_SPECIES, 1, MAX_SPECIES>& matrixIn) {

    for (int n=1; n<=nspecies; ++n ){
        for (int m=1; m<=nspecies; ++m ){
            matrixIn(n,m) = 0.0;
        }
    }
}


AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void PopulateX_xxT(int nspecies,
                Array2D<Real, 1, MAX_SPECIES, 1, MAX_SPECIES>& X_xxT,
                GpuArray<Real, MAX_SPECIES>& MolarConcN) {

    for (int row=1; row<=nspecies;++row ){ 
        // diagonal entries
        X_xxT(row,row) = MolarConcN[row-1] - std::pow(MolarConcN[row-1],2);
        for (int column=1; column<=row-1; ++column ){
            // off-diagonal entries
            // form x*traspose(x) off diagonals 
            X_xxT(row,column) = -MolarConcN[row-1]*MolarConcN[column-1];
            //symmetric
            X_xxT(column,row) = X_xxT(row,column);
        }
    }
}

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void GammaIPlusMatmul(int nspecies,
                Array2D<Real, 1, MAX_SPECIES, 1, MAX_SPECIES>& GammaN,
                Array2D<Real, 1, MAX_SPECIES, 1, MAX_SPECIES>& X_xxT,
                Array2D<Real, 1, MAX_SPECIES, 1, MAX_SPECIES>& HessianN) {
        
    //Compute Gamma 
    for (int row=1; row<=nspecies; ++row){
        for (int column=1; column<=nspecies; ++column){
            if (row == column) {
                GammaN(row,column) = 1.0;   // add the identity matrix
            } else {
                GammaN(row,column) = 0.0;   // intialize off-diagonals to 0
            }
            for (int n=1; n<=nspecies; ++n){ 
                GammaN(row, column) += X_xxT(row,n) * HessianN(n,column);
            }
        }
    }
}

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
void ComputeGammaLocal(
                GpuArray<Real, MAX_SPECIES>& MolarConcN,
                Array2D<Real, 1, MAX_SPECIES, 1, MAX_SPECIES>& HessianN, 
                Array2D<Real, 1, MAX_SPECIES, 1, MAX_SPECIES>& GammaN,
                int nspecies){
    
    //Local variables
    Array2D<Real, 1, MAX_SPECIES, 1, MAX_SPECIES> X_xxT;

    if ((use_multiphase == 1) && (nspecies == 2)){ 
                    
        // local to define Gamma matrix
        Real w1 = MolarConcN[0];
        Real w2 = MolarConcN[1];

        if (std::abs(w1+w2-1.0) > 1e-14){  
            Print() << "Mole fractions do not add up in gamma computation" << std::endl; 
        }
        if (w1 < 0){ 
            w1 = 0.0;
            w2 = 1.0;
        }
        if (w2 < 0){ 
            w1 = 1.0;
            w2 = 0.0;
        }

        GammaN(1,2) = w1 * n_gex * n_gex * alpha_gex * std::pow(w1,n_gex-1) * std::pow(w2,n_gex-1);
        GammaN(2,1) = w2 * n_gex * n_gex * alpha_gex * std::pow(w2,n_gex-1) * std::pow(w1,n_gex-1);
        GammaN(1,1) = 1.0 + w1 * n_gex * (n_gex-1) * alpha_gex * std::pow(w1,n_gex-2) * std::pow(w2,n_gex); 
        GammaN(2,2) = 1.0 + w2 * n_gex * (n_gex-1) * alpha_gex * std::pow(w2,n_gex-2) * std::pow(w1,n_gex); 

    } else {

        //populate X_xxT
        if (is_ideal_mixture == 1){   
            MatrixToZeros(nspecies, X_xxT);
        } else {
            PopulateX_xxT(nspecies, X_xxT, MolarConcN);
        }
    }
    
    //Compute Gamma 
    GammaIPlusMatmul(nspecies, GammaN, X_xxT, HessianN);
}

#endif

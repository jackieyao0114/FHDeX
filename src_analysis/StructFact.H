#ifndef _StructFact_H
#define _StructFact_H_

#include <AMReX.H>
#include <AMReX_MultiFab.H>
#include <AMReX_Vector.H>
#include <AMReX_VisMF.H>

// These are for SWFFT
#include <Distribution.H>
#include <AlignedAllocator.h>
#include <Dfft.H>

// These are for FFTW
#include <fftw3.h>
#include <fftw3-mpi.h>
#include <AMReX_GpuComplex.H>

#include <string>

#include "common_functions.H"

#define ALIGN 16

using namespace amrex;

class StructFact {

    int NVAR = 1;        // Number of variable
    int NVARU = NVAR;    // Number of unique variables
    int NCOV = 1;        // Number of covariances (lower diagonal of matrix - by default)

    int verbosity = 0;

    // Total number of states to average over, updated by FortStructure()
    int nsamples = 0;

    // Vector containing running sums of real and imaginary components 
    // of inner products (covariances) of DFTs
    MultiFab cov_real;
    MultiFab cov_imag;

    // Vector of MultiFabs containing final magnitude of covariances
    MultiFab cov_mag;

    // Vector containing covariance scaling
    Vector< Real > scaling;

    // Vector containing names of covariances
    amrex::Vector< std::string > cov_names;

    // 2 vectors containing structure factor pairs
    amrex::Vector< int > s_pairA;
    amrex::Vector< int > s_pairB;

    // Define vector of unique selected variables
    amrex::Vector< int > var_u;

    // use fftw (1) or swfft (0)
    // note: fftw does not work for flattened MultiFabs yet
    // swfft is buggy for non-cubic domains and large flattened MultiFabs
    int use_fftw;

public:

    StructFact();
    
    StructFact(const amrex::BoxArray&, const amrex::DistributionMapping&, 
               const amrex::Vector< std::string >&,
               const amrex::Vector< amrex::Real >&,
               const amrex::Vector< int >&, const amrex::Vector< int >&,
               const int& verbosity=0);

    StructFact(const amrex::BoxArray&, const amrex::DistributionMapping&, 
               const amrex::Vector< std::string >&,
               const amrex::Vector< amrex::Real >&,
               const int& verbosity=0);

    void define(const amrex::BoxArray&, const amrex::DistributionMapping&, 
                const amrex::Vector< std::string >&,
                const amrex::Vector< amrex::Real >&,
                const int& verbosity=0);

    void FortStructure(const amrex::MultiFab&, const amrex::Geometry&,
                       const int& fft_type_in,
                       const int& reset=0);

    void Reset();
    
    void ComputeSWFFT(const amrex::MultiFab&, amrex::MultiFab&,
                      amrex::MultiFab&, const amrex::Geometry&);
    
    void ComputeFFTW(const amrex::MultiFab&, amrex::MultiFab&,
                    amrex::MultiFab&, const amrex::Geometry&);
    
    void WritePlotFile(const int, const amrex::Real, const amrex::Geometry&, 
                       std::string, const int& zero_avg=1);
    
    void StructOut(amrex::MultiFab&);
    
    void Finalize(amrex::MultiFab&, amrex::MultiFab&, const Geometry& geom,
                  const int& zero_avg=1);
    
    void ShiftFFT(amrex::MultiFab&,  const Geometry& geom,
                  const int& zero_avg=1);

    void IntegratekShells(const int& step, const amrex::Geometry& geom);
};

#endif


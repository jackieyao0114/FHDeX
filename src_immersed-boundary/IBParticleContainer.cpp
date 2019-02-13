#include <AMReX.H>
#include <AMReX_AmrParGDB.H>
#include <AMReX_ParmParse.H>
#include <AMReX_Particles.H>

#include <AMReX_VisMF.H>  // amrex::VisMF::Write(MultiFab)

#include <IBParticleContainer.H>
#include <ib_functions_F.H>
#include <MFUtil.H>



using namespace amrex;

bool IBParticleContainer::use_neighbor_list  {true};
bool IBParticleContainer::sort_neighbor_list {false};


IBParticleContainer::IBParticleContainer(AmrCore * amr_core, int n_nbhd)
    : NeighborParticleContainer<IBP_realData::count, IBP_intData::count>(
        amr_core->GetParGDB(), n_nbhd
        ),
      m_amr_core(amr_core)
{
    ReadStaticParameters();

    this->SetVerbose(0);

    // Turn off certain components for ghost particle communication
    // Field numbers: {0, 1, 2} => {x, y, z} particle coordinates
    //      => 3 corresponds to the start of IBP_realData
    setRealCommComp(4, false);   // IBP_realData.volume
    setRealCommComp(5, false);   // IBP_realData.mass
    setRealCommComp(6, false);   // IBP_realData.density
    setRealCommComp(7, false);   // IBP_realData.oneOverI
    setRealCommComp(14, false);  // IBP_realData.dragx
    setRealCommComp(15, false);  // IBP_realData.dragy
    setRealCommComp(16, false);  // IBP_realData.dragz

    // Field numbers: {0, 1} => {ID, CPU}
    //      => 2 corresponds to the start of IBP_intData
    // We _do_ want the the neighbour particles to have ID and cpu init data.
    //setIntCommComp(0, false);  // IBP_intData.phase
    //setIntCommComp(1, false);  // IBP_intData.state
    setIntCommComp(3, false);    // IBP_intData.phase
}


void IBParticleContainer::InitList(int lev,
                                   const Vector<RealVect> & pos,
                                   const Vector<Real> & r,
                                   const Vector<Real> & rho) {

    // Inverse cell-size vector => used for determining index corresponding to
    // IBParticle position (pos)
    RealVect inv_dx = RealVect(
                AMREX_D_DECL(
                    Geom(lev).InvCellSize(0),
                    Geom(lev).InvCellSize(1),
                    Geom(lev).InvCellSize(2)
                )
        );


    int total_np = 0;

    // This uses the particle tile size. Note that the default is to tile so if
    // we remove the true and don't explicitly add false it will still tile.
    for (MFIter mfi = MakeMFIter(lev,true); mfi.isValid(); ++mfi) {

        // This is particles per grid so we reset to 0
        int pcount = 0;

        // Current tile box
        const Box & tile_box = mfi.tilebox();

        // Create a particle container for this grid and add the
        // immersed-boundary particles to it if the particle's position (pos[i])
        // is within current tile box.
        const int grid_id = mfi.index();
        const int tile_id = mfi.LocalTileIndex();
        auto & particles = GetParticles(lev)[std::make_pair(grid_id,tile_id)];

        for(int i = 0; i < pos.size(); i++) {
            // IntVect representing particle's position in the tile_box grid.
            RealVect pos_grid = pos[i];
            pos_grid *= inv_dx;
            IntVect pos_ind = IntVect(
                    AMREX_D_DECL(
                        (int) pos_grid[0],
                        (int) pos_grid[1],
                        (int) pos_grid[2]
                    )
                );

            // Add particle at position pos iff it's vector index is contained
            // within tile_box.
            if(tile_box.contains(pos_ind)) {
                pcount ++;

                ParticleType p_new;

                // Set id and cpu for this particle
                p_new.id()  = ParticleType::NextID();
                p_new.cpu() = ParallelDescriptor::MyProc();

                // Set particle position
                p_new.pos(0) = pos[i][0];
                p_new.pos(1) = pos[i][1];
                p_new.pos(2) = pos[i][2];

                // Initialize particle velocity (and angular velocity) as well
                // as drag to 0
                p_new.rdata(IBP_realData::velx)   = 0.;
                p_new.rdata(IBP_realData::vely)   = 0.;
                p_new.rdata(IBP_realData::velz)   = 0.;

                p_new.rdata(IBP_realData::omegax) = 0.;
                p_new.rdata(IBP_realData::omegay) = 0.;
                p_new.rdata(IBP_realData::omegaz) = 0.;

                p_new.rdata(IBP_realData::dragx)  = 0.;
                p_new.rdata(IBP_realData::dragy)  = 0.;
                p_new.rdata(IBP_realData::dragz)  = 0.;

                // Calculate particle volume, mass, moment of inertia from
                // radius and density
                int pstate = 1;
                Real pradius = r[i], pdensity = rho[i];
                Real pvol, pmass, pomoi, pomega;
                // TODO: this only works for spherical particles
                set_particle_properties(& pstate, & pradius, & pdensity,
                                        & pvol,   & pmass,   & pomoi,     & pomega );

                // TODO: Audit
                p_new.idata(IBP_intData::phase)     = 1;
                p_new.idata(IBP_intData::state)     = pstate;

                p_new.rdata(IBP_realData::volume)   = pvol;
                p_new.rdata(IBP_realData::density)  = pdensity;
                p_new.rdata(IBP_realData::mass)     = pmass;
                p_new.rdata(IBP_realData::oneOverI) = pomoi;
                p_new.rdata(IBP_realData::radius)   = pradius;

                // Add to the data structure
                particles.push_back(p_new);
            }
        }

        const int np = pcount;
        total_np += np;
    }

    ParallelDescriptor::ReduceIntSum(total_np,ParallelDescriptor::IOProcessorNumber());
    amrex::Print() << "Total number of generated particles: " << total_np << std::endl;

    // We shouldn't need this if the particles are tiled with one tile per
    // grid, but otherwise we do need this to move particles from tile 0 to the
    // correct tile.
    Redistribute();

}


void IBParticleContainer::ReadStaticParameters() {
    static bool initialized = false;

    if (!initialized)
    {
        ParmParse pp("particles");

        do_tiling = true;  // because the default in amrex is false

        pp.query("do_tiling",  do_tiling);

        Vector<int> ts(BL_SPACEDIM);

        if (pp.queryarr("tile_size", ts))
            tile_size = IntVect(ts);

        pp.query("use_neighbor_list", use_neighbor_list);
        pp.query("sort_neighbor_list", sort_neighbor_list);

        initialized = true;
    }
}


void IBParticleContainer::AllocData() {
    reserveData();
    resizeData();

    /****************************************************************************
    * Allocate FLUID data containers                                            *
    ****************************************************************************/

    //int nlevs_max = m_amr_core->maxLevel() + 1;
}


void IBParticleContainer::AllocateArrays(int lev, int a_nghost) {

    // For future reference:
    nghost = a_nghost;


    /****************************************************************************
    * Cell-based arrays                                                         *
    ****************************************************************************/

    const BoxArray & ba            = ParticleBoxArray(lev);
    const DistributionMapping & dm = ParticleDistributionMap(lev);
}


//
// NOTE: kept for reference:
//
//void IBParticleContainer::RegridArrays(int lev, BoxArray & new_grids, DistributionMapping & new_dmap) {
//
//   /****************************************************************************
//    * Cell-based arrays                                                        *
//    ****************************************************************************/
//
//    // Void fraction
//    std::unique_ptr<MultiFab> ep_g_new = make_copy(new_grids, new_dmap, * ep_g[lev], lev);
//    ep_g[lev] = std::move(ep_g_new);
//
//}


void IBParticleContainer::CopyFluidData(int lev, const mfix_level & mf_lev) {
    //
    // NOTE: kept for reference
    //

    // ep_g[lev]->copy(*  mf_lev.ep_g[lev], 0, 0, 1,  mf_lev.ep_g[lev]->nGrow(),  ep_g[lev]->nGrow());
    //  p_g[lev]->copy(*   mf_lev.p_g[lev], 0, 0, 1,   mf_lev.p_g[lev]->nGrow(),   p_g[lev]->nGrow());
    // ro_g[lev]->copy(*  mf_lev.ro_g[lev], 0, 0, 1,  mf_lev.ro_g[lev]->nGrow(),  ro_g[lev]->nGrow());
    //rop_g[lev]->copy(* mf_lev.rop_g[lev], 0, 0, 1, mf_lev.rop_g[lev]->nGrow(), rop_g[lev]->nGrow());
    //  u_g[lev]->copy(*   mf_lev.u_g[lev], 0, 0, 1,   mf_lev.u_g[lev]->nGrow(),   u_g[lev]->nGrow());
    //  v_g[lev]->copy(*   mf_lev.v_g[lev], 0, 0, 1,   mf_lev.v_g[lev]->nGrow(),   v_g[lev]->nGrow());
    //  w_g[lev]->copy(*   mf_lev.w_g[lev], 0, 0, 1,   mf_lev.w_g[lev]->nGrow(),   w_g[lev]->nGrow());
    // mu_g[lev]->copy(*  mf_lev.mu_g[lev], 0, 0, 1,  mf_lev.mu_g[lev]->nGrow(),  mu_g[lev]->nGrow());
    // p0_g[lev]->copy(*  mf_lev.p0_g[lev], 0, 0, 1,  mf_lev.p0_g[lev]->nGrow(),  p0_g[lev]->nGrow());
}


void IBParticleContainer::PrintParticleData(int lev) {

    // Inverse cell-size vector => max is used for determining IBParticle
    // radius in units of cell size
    Vector<Real> inv_dx = {
            AMREX_D_DECL(
                Geom(lev).InvCellSize(0),
                Geom(lev).InvCellSize(1),
                Geom(lev).InvCellSize(2)
            )
        };
    // Find max inv_dx (in case we have an anisotropic grid)
    Real mx_inv_dx = * std::max_element(inv_dx.begin(), inv_dx.end());

    amrex::AllPrintToFile("ib_particle_data") << "Particles on each box:" << std::endl;

    fillNeighbors();
    long local_count = 0;

    // ParIter skips tiles without particles => Iterate over MultiFab instead
    // of ParticleIter
    for(MFIter pti = this->MakeMFIter(lev); pti.isValid(); ++pti) {
        // MuliFabs are indexed using a pair: (BoxArray index, tile index):
        PairIndex index(pti.index(), pti.LocalTileIndex());

        // Neighbours are stored as raw data (see below), hence the number of
        // grown particles is given by the size of the raw data array over the
        // number of raw data per particle.
        int ng = neighbors[lev][index].size() / pdata_size;

        //long np = NumberOfParticles(pti);
        auto & particle_data = GetParticles(lev)[index];
        long np = particle_data.size();

        local_count += np;

        // Print current box info
        AllPrintToFile("ib_particle_data") << "Box:"         << pti.index()
                                           << " "            << pti.tilebox()
                                           << ", count: "    << np
                                           << ", nb count: " << ng
                                           << std::endl;

        // Print IBParticle
        AllPrintToFile("ib_particle_data") << " * IBPartcies:" << std::endl;

        //AoS & particles = pti.GetArrayOfStructs();
        AoS & particles = particle_data.GetArrayOfStructs();
        for(int i = 0; i < np; i++){
            ParticleType & part = particles[i];
            Real r              = part.rdata(IBP_realData::radius);

            int r_ncx = (int) (r * mx_inv_dx);

            AllPrintToFile("ib_particle_data") << "   +- " << part << std::endl;
            AllPrintToFile("ib_particle_data") << "   +---> Radius [NCells]: " << r_ncx << std::endl;
        }

        // Print neighbour IBParticles
        AllPrintToFile("ib_particle_data") << " * Grown IBParticles:" << std::endl;

        // TODO: HAXOR!!! This should be fixed ASAP: if I understand this
        // correctly, the neighbor data contains the particle data as a binary
        // array (char). By casting to ParticleType, what we're doing is
        // interpreting the data in neighbours[index] as valid particle data.
        // Also we stride the neighbors[index] array in units of
        // sizeof(ParticleData). All of this is a little too dangerous for my
        // taste: never hide what you're doing from your compiler!!!
        ParticleType * nbhd_data = (ParticleType *) neighbors[lev][index].dataPtr();
        for(int i = 0; i < ng; i++){
            ParticleType & part = nbhd_data[i];
            Real r              = part.rdata(IBP_realData::radius);

            int r_ncx = (int) (r * mx_inv_dx);

            AllPrintToFile("ib_particle_data") << "   +- " << part << std::endl;
            AllPrintToFile("ib_particle_data") << "   +---> Radius [NCells]: " << r_ncx << std::endl;
        }
    }

    AllPrintToFile("ib_particle_data") << "Total for this process: "
                                       << local_count << std::endl << std::endl;
}


Vector<IBP_info> IBParticleContainer::get_IBParticle_info(int lev, PairIndex index) {
    Vector<IBP_info> info;

    // Inverse cell-size vector => used for determining index corresponding to
    // IBParticle position (pos)
    RealVect inv_dx = RealVect(
                AMREX_D_DECL(
                    Geom(lev).InvCellSize(0),
                    Geom(lev).InvCellSize(1),
                    Geom(lev).InvCellSize(2)
                )
        );


    // Neighbours are stored as raw data (see below), hence the number of
    // grown particles is given by the size of the raw data array over the
    // number of raw data per particle.
    int ng = neighbors[lev][index].size() / pdata_size;

    auto & particle_data = GetParticles(lev)[index];
    long np = particle_data.size();

    // Iterate over local particle data
    AoS & particles = particle_data.GetArrayOfStructs();
    for(int i = 0; i < np; i++){
        ParticleType & part = particles[i];

        // Position of IBParticle
        RealVect pos = RealVect(
                AMREX_D_DECL(part.pos(0), part.pos(1), part.pos(2))
            );

        // Velocity of IBParticle
        RealVect vel = RealVect(
                AMREX_D_DECL(part.rdata(IBP_realData::velx),
                             part.rdata(IBP_realData::vely),
                             part.rdata(IBP_realData::velz)   )
            );

        // Position of IBParticle on grid
        RealVect pos_grid = pos * inv_dx;
        IntVect  pos_ind  = IntVect(
                AMREX_D_DECL( (int) pos_grid[0], (int) pos_grid[1], (int) pos_grid[2] )
            );

        // Radius
        Real r = part.rdata(IBP_realData::radius);

        // Construct info struct
        IBP_info part_info;
        part_info.pos    = pos;
        part_info.vel    = vel;
        part_info.index  = pos_ind;
        part_info.radius = r;
        part_info.id     = part.id();
        part_info.cpu    = part.cpu();

        // Add to list
        info.push_back(part_info);
    }

    // Iterate over neighbour particles:
    // TODO: HAXOR!!! This should be fixed ASAP: if I understand this correctly,
    // the neighbor data contains the particle data as a binary array (char). By
    // casting to ParticleType, what we're doing is interpreting the data in
    // neighbours[index] as valid particle data. Also we stride the
    // neighbors[index] array in units of sizeof(ParticleData). All of this is a
    // little too dangerous for my taste: never hide what you're doing from your
    // compiler!!!
    ParticleType * nbhd_data = (ParticleType *) neighbors[lev][index].dataPtr();
    for(int i = 0; i < ng; i++){
        ParticleType & part = nbhd_data[i];

        // Position of neighbour IBParticle
        RealVect pos = RealVect(
                AMREX_D_DECL(part.pos(0), part.pos(1), part.pos(2))
            );

        // Velocity of IBParticle
        RealVect vel = RealVect(
                AMREX_D_DECL(part.rdata(IBP_realData::velx),
                             part.rdata(IBP_realData::vely),
                             part.rdata(IBP_realData::velz)   )
            );

        // Position of neighbour IBParticle on grid
        RealVect pos_grid = pos * inv_dx;
        IntVect  pos_ind  = IntVect(
                AMREX_D_DECL( (int) pos_grid[0], (int) pos_grid[1], (int) pos_grid[2] )
            );

        // Radius
        Real r = part.rdata(IBP_realData::radius);

        // Construct info struct
        IBP_info part_info;
        part_info.pos    = pos;
        part_info.vel    = vel;
        part_info.index  = pos_ind;
        part_info.radius = r;
        part_info.id     = part.id();
        part_info.cpu    = part.cpu();

        // Add to list
        info.push_back(part_info);
    }
    return info;
}
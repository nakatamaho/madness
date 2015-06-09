/*
  This file is part of MADNESS.

  Copyright (C) 2007,2010 Oak Ridge National Laboratory

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

  For more information please contact:

  Robert J. Harrison
  Oak Ridge National Laboratory
  One Bethel Valley Road
  P.O. Box 2008, MS-6367

  email: harrisonrj@ornl.gov
  tel:   865-241-3937
  fax:   865-572-0680
*/

/// \file SCFOperators.cc
/// \brief Operators for the molecular HF and DFT code
/// \defgroup chem The molecular density functional and Hartree-Fock code


#include <apps/chem/SCFOperators.h>
#include <apps/chem/SCF.h>
#include <apps/chem/nemo.h>
#include <apps/chem/correlationfactor.h>


using namespace madness;

namespace madness {

template<typename T, std::size_t NDIM>
DistributedMatrix<T> Kinetic<T,NDIM>::kinetic_energy_matrix(World & world,
        const vecfuncT & v) const {
    int n = v.size();
    DistributedMatrix<T> r = column_distributed_matrix<T>(world, n, n);
    reconstruct(world, v);

    // apply the derivative operator on each function for each dimension
    std::vector<vecfuncT> dv(NDIM);
    for (std::size_t i=0; i<NDIM; ++i) {
        dv[i]=apply(world, *(gradop[i]), v, false);
    }
    world.gop.fence();
    for (std::size_t i=0; i<NDIM; ++i) {
        compress(world,dv[i],false);
    }
    world.gop.fence();
    for (std::size_t i=0; i<NDIM; ++i) {
        r += matrix_inner(r.distribution(), dv[i], dv[i], true);
    }
    r *= 0.5;
    return r;
}


template<typename T, std::size_t NDIM>
DistributedMatrix<T> Kinetic<T,NDIM>::kinetic_energy_matrix(World & world,
        const vecfuncT & vbra, const vecfuncT & vket) const {
    int n = vbra.size();
    int m = vket.size();
    DistributedMatrix<T> r = column_distributed_matrix<T>(world, n, m);
    reconstruct(world, vbra);
    reconstruct(world, vket);

    // apply the derivative operator on each function for each dimension
    std::vector<vecfuncT> dvbra(NDIM),dvket(NDIM);
    for (std::size_t i=0; i<NDIM; ++i) {
        dvbra[i]=apply(world, *(gradop[i]), vbra, false);
        dvket[i]=apply(world, *(gradop[i]), vket, false);
    }
    world.gop.fence();
    for (std::size_t i=0; i<NDIM; ++i) {
        compress(world,dvbra[i],false);
        compress(world,dvket[i],false);
    }
    world.gop.fence();
    for (std::size_t i=0; i<NDIM; ++i) {
        r += matrix_inner(r.distribution(), dvbra[i], dvket[i], true);
    }
    r *= 0.5;
    return r;
}

// explicit instantiation
template class Kinetic<double,1>;
template class Kinetic<double,2>;
template class Kinetic<double,3>;
template class Kinetic<double,4>;
template class Kinetic<double,5>;
template class Kinetic<double,6>;

Coulomb::Coulomb(World& world, const Nemo* nemo) : world(world),
        R_square(nemo->R_square) {
    vcoul=compute_potential(nemo);
}


real_function_3d Coulomb::compute_density(const SCF* calc) const {
    real_function_3d density = calc->make_density(world, calc->get_aocc(),
            calc->get_amo());
    if (calc->is_spin_restricted()) {
        density.scale(2.0);
    } else {
        real_function_3d brho = calc->make_density(world, calc->get_bocc(),
                calc->get_bmo());
        density+=brho;
    }
    density.truncate();
    return density;
}

real_function_3d Coulomb::compute_potential(const madness::SCF* calc) const {
    real_function_3d density=compute_density(calc);
    return calc->make_coulomb_potential(density);
}

/// same as above, but with the additional factor R^2 in the density
real_function_3d Coulomb::compute_potential(const madness::Nemo* nemo) const {
    real_function_3d density=compute_density(nemo->get_calc().get());
    if (R_square.is_initialized()) density=(density*R_square).truncate();
    return nemo->get_calc()->make_coulomb_potential(density);
}

real_function_3d Coulomb::compute_potential(const real_function_3d& density,
        double lo, double econv) const {
    real_convolution_3d poisson = CoulombOperator(world, lo, econv);
    return poisson(density).truncate();
}


Nuclear::Nuclear(World& world, const SCF* calc) : world(world) {
    ncf=std::shared_ptr<NuclearCorrelationFactor>(
            new PseudoNuclearCorrelationFactor(world,
            calc->molecule,calc->potentialmanager,1.0));
}

Nuclear::Nuclear(World& world, const Nemo* nemo) : world(world) {
    ncf=nemo->nuclear_correlation;
}

vecfuncT Nuclear::operator()(const vecfuncT& vket) const {

    const static std::size_t NDIM=3;

    // shortcut for local nuclear potential (i.e. no correlation factor)
    if (ncf->type()==NuclearCorrelationFactor::None) {
        vecfuncT result=mul(world,ncf->U2(),vket);
        truncate(world,result);
        return result;
    }

    std::vector< std::shared_ptr<Derivative<double,NDIM> > > gradop =
            gradient_operator<double,NDIM>(world);
    reconstruct(world, vket);
    vecfuncT vresult=zero_functions_compressed<double,NDIM>(world,vket.size());

    // memory-saving algorithm: outer loop over the dimensions
    // apply the derivative operator on each function for each dimension
    for (std::size_t i=0; i<NDIM; ++i) {
        std::vector<Function<double,NDIM> > dv=apply(world, *(gradop[i]), vket, true);
        real_function_3d U1=ncf->U1(i%3);
        std::vector<Function<double,NDIM> > U1dv=mul(world,U1,dv);
        vresult=add(world,vresult,U1dv);
    }

    real_function_3d U2=ncf->U2();
    std::vector<Function<double,NDIM> > U2v=mul(world,U2,vket);
    vresult=add(world,vresult,U2v);
    truncate(world,vresult);
    return vresult;
}


Exchange::Exchange(World& world, const SCF* calc, const int ispin)
        : world(world), small_memory_(true), same_(false) {
    if (ispin==0) { // alpha spin
        mo_ket=calc->amo;
        occ=calc->aocc;
    } else if (ispin==1) {  // beta spin
        mo_ket=calc->bmo;
        occ=calc->bocc;
    }
    mo_bra=mo_ket;
    poisson = std::shared_ptr<real_convolution_3d>(
            CoulombOperatorPtr(world, calc->param.lo, calc->param.econv));
}

Exchange::Exchange(World& world, const Nemo* nemo, const int ispin)
    : world(world), small_memory_(true), same_(false) {

    if (ispin==0) { // alpha spin
        mo_ket=nemo->get_calc()->amo;
        occ=nemo->get_calc()->aocc;
    } else if (ispin==1) {  // beta spin
        mo_ket=nemo->get_calc()->bmo;
        occ=nemo->get_calc()->bocc;
    }

    mo_bra=mul(world,nemo->nuclear_correlation->square(),mo_ket);
    truncate(world,mo_bra);
    poisson = std::shared_ptr<real_convolution_3d>(
            CoulombOperatorPtr(world, nemo->get_calc()->param.lo,
                    nemo->get_calc()->param.econv));

}


void Exchange::set_parameters(const vecfuncT& bra, const vecfuncT& ket,
        const Tensor<double>& occ1, const double lo, const double econv) {
    mo_bra=copy(world,bra);
    mo_ket=copy(world,ket);
    occ=copy(occ1);
    poisson = std::shared_ptr<real_convolution_3d>(
            CoulombOperatorPtr(world, lo, econv));
}


vecfuncT Exchange::operator()(const vecfuncT& vket) const {
    const bool same = this->same();
    int nocc = mo_bra.size();
    int nf = vket.size();
    double tol = FunctionDefaults < 3 > ::get_thresh(); /// Important this is consistent with Coulomb
    vecfuncT Kf = zero_functions_compressed<double, 3>(world, nf);
    reconstruct(world, mo_bra);
    norm_tree(world, mo_bra);
    if (!same) {
        reconstruct(world, vket);
        norm_tree(world, vket);
    }

    if (small_memory_) {     // Smaller memory algorithm ... possible 2x saving using i-j sym
        for(int i=0; i<nocc; ++i){
            if(occ[i] > 0.0){
                vecfuncT psif = mul_sparse(world, mo_bra[i], vket, tol); /// was vtol
                truncate(world, psif);
                psif = apply(world, *poisson.get(), psif);
                truncate(world, psif);
                psif = mul_sparse(world, mo_ket[i], psif, tol); /// was vtol
                gaxpy(world, 1.0, Kf, occ[i], psif);
            }
        }
    } else {    // Larger memory algorithm ... use i-j sym if psi==f
        vecfuncT psif;
        for (int i = 0; i < nocc; ++i) {
            int jtop = nf;
            if (same)
                jtop = i + 1;
            for (int j = 0; j < jtop; ++j) {
                psif.push_back(mul_sparse(mo_bra[i], vket[j], tol, false));
            }
        }

        world.gop.fence();
        truncate(world, psif);
        psif = apply(world, *poisson.get(), psif);
        truncate(world, psif, tol);
        reconstruct(world, psif);
        norm_tree(world, psif);
        vecfuncT psipsif = zero_functions<double, 3>(world, nf * nocc);
        int ij = 0;
        for (int i = 0; i < nocc; ++i) {
            int jtop = nf;
            if (same)
                jtop = i + 1;
            for (int j = 0; j < jtop; ++j, ++ij) {
                psipsif[i * nf + j] = mul_sparse(psif[ij], mo_ket[i], false);
                if (same && i != j) {
                    psipsif[j * nf + i] = mul_sparse(psif[ij], mo_ket[j], false);
                }
            }
        }
        world.gop.fence();
        psif.clear();
        world.gop.fence();
        compress(world, psipsif);
        for (int i = 0; i < nocc; ++i) {
            for (int j = 0; j < nf; ++j) {
                Kf[j].gaxpy(1.0, psipsif[i * nf + j], occ[i], false);
            }
        }
        world.gop.fence();
        psipsif.clear();
        world.gop.fence();
    }
    truncate(world, Kf, tol);
    return Kf;

}


Fock::Fock(World& world, const SCF* calc,
        std::shared_ptr<NuclearCorrelationFactor> ncf )
    : world(world),
      J(world,calc),
      K(world,calc,0),
      T(world),
      V(world,ncf) {
}



} // namespace madness



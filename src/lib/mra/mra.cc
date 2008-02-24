/*
  This file is part of MADNESS.
  
  Copyright (C) <2007> <Oak Ridge National Laboratory>
  
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

  
  $Id$
*/

  
#define WORLD_INSTANTIATE_STATIC_TEMPLATES
#include <mra/mra.h>
//#include <mra/loadbal.h>

/// \file mra.cc
/// \file Declaration and initialization of static data, some implementation, some instantiation

namespace madness {

    // Definition and initialization of FunctionDefaults static members
    // It cannot be an instance of FunctionFactory since we want to
    // set the defaults independent of the data type.  


    template <typename T, int NDIM>
    void FunctionCommonData<T,NDIM>::_make_disp() {
        Vector<int, NDIM> d;
        int bmax;
        if (NDIM == 1) bmax = 7; // !! Make sure that SimpleCache is consistent!!!!
        else if (NDIM == 2) bmax = 5;
        else if (NDIM == 3) bmax = 2;
        else bmax = 1;

        int num = 1;
        for (int i=0; i<NDIM; i++) num *= (2*bmax + 1);
        disp = std::vector< Displacement<NDIM> >(num);

        num = 0;
        if (NDIM == 1) {
            for (d[0]=-bmax; d[0]<=bmax; d[0]++)
                disp[num++] = Displacement<NDIM>(d);
        }
        else if (NDIM == 2) {
            for (d[0]=-bmax; d[0]<=bmax; d[0]++)
                for (d[1]=-bmax; d[1]<=bmax; d[1]++)
                    disp[num++] = Displacement<NDIM>(d);
        }
        else if (NDIM == 3) {
            for (d[0]=-bmax; d[0]<=bmax; d[0]++)
                for (d[1]=-bmax; d[1]<=bmax; d[1]++)
                    for (d[2]=-bmax; d[2]<=bmax; d[2]++)
                        disp[num++] = Displacement<NDIM>(d);
        }
        else if (NDIM == 4) {
            for (d[0]=-bmax; d[0]<=bmax; d[0]++)
                for (d[1]=-bmax; d[1]<=bmax; d[1]++)
                    for (d[2]=-bmax; d[2]<=bmax; d[2]++)
                        for (d[3]=-bmax; d[3]<=bmax; d[3]++)
                            disp[num++] = Displacement<NDIM>(d);
        }
        else if (NDIM == 5) {
            for (d[0]=-bmax; d[0]<=bmax; d[0]++)
                for (d[1]=-bmax; d[1]<=bmax; d[1]++)
                    for (d[2]=-bmax; d[2]<=bmax; d[2]++)
                        for (d[3]=-bmax; d[3]<=bmax; d[3]++)
                            for (d[4]=-bmax; d[4]<=bmax; d[4]++)
                                disp[num++] = Displacement<NDIM>(d);
        }
        else if (NDIM == 6) {
            for (d[0]=-bmax; d[0]<=bmax; d[0]++)
                for (d[1]=-bmax; d[1]<=bmax; d[1]++)
                    for (d[2]=-bmax; d[2]<=bmax; d[2]++)
                        for (d[3]=-bmax; d[3]<=bmax; d[3]++)
                            for (d[4]=-bmax; d[4]<=bmax; d[4]++)
                                for (d[5]=-bmax; d[5]<=bmax; d[5]++)
                                    disp[num++] = Displacement<NDIM>(d);
        }
        else {
            MADNESS_EXCEPTION("_make_disp: hard dimension loop",NDIM);
        }

        std::sort(disp.begin(), disp.end());
    }


    template <typename T, int NDIM>
    void FunctionCommonData<T,NDIM>::_make_dc_periodic() {
        // See ABGV for details
        r0 = Tensor<double>(k,k);
        rp = Tensor<double>(k,k);
        rm = Tensor<double>(k,k);

        double iphase = 1.0;
        for (int i=0; i<k; i++) {
            double jphase = 1.0;
            for (int j=0; j<k; j++) {
                double gammaij = sqrt(double((2*i+1)*(2*j+1)));
                double Kij;
                if (((i-j)>0) && (((i-j)%2)==1))
                    Kij = 2.0;
                else
                    Kij = 0.0;

                r0(i,j) = 0.5*(1.0 - iphase*jphase - 2.0*Kij)*gammaij;
                rm(i,j) = 0.5*jphase*gammaij;
                rp(i,j) =-0.5*iphase*gammaij;
                jphase = -jphase;
            }
            iphase = -iphase;
        }

        // Make the rank-1 forms of rm and rp
        rm_left = Tensor<double>(k);
        rm_right = Tensor<double>(k);
        rp_left = Tensor<double>(k);
        rp_right = Tensor<double>(k);

        iphase = 1.0;
        for (int i=0; i<k; i++) {
            double gamma = sqrt(0.5*(2*i+1));
            rm_left(i)  = rp_right(i) = gamma;
            rm_right(i) = rp_left(i)  = gamma*iphase;
            iphase *= -1.0;
        }
        rp_left.scale(-1.0);

//         Tensor<double> rm_test = outer(rm_left,rm_right);
//         Tensor<double> rp_test = outer(rp_left,rp_right);
    }

    template <typename T, int NDIM>
    void FunctionCommonData<T,NDIM>::_init_twoscale() {
        if (! two_scale_hg(k, &hg)) throw "failed to get twoscale coefficients";
        hgT = transpose(hg);
        hgsonly = copy(hg(Slice(0,k-1),_));
    }

    template <typename T, int NDIM>
    void FunctionCommonData<T,NDIM>::_init_quadrature
    (int k, int npt, Tensor<double>& quad_x, Tensor<double>& quad_w, 
     Tensor<double>& quad_phi, Tensor<double>& quad_phiw, Tensor<double>& quad_phit) {
        quad_x = Tensor<double>(npt);
        quad_w = Tensor<double>(npt);
        quad_phi = Tensor<double>(npt,k);
        quad_phiw = Tensor<double>(npt,k);

        gauss_legendre(npt,0.0,1.0,quad_x.ptr(),quad_w.ptr());
        for (int mu=0; mu<npt; mu++) {
            double phi[200];
            legendre_scaling_functions(quad_x(mu),k,phi);
            for (int j=0; j<k; j++) {
                quad_phi(mu,j) = phi[j];
                quad_phiw(mu,j) = quad_w(mu)*phi[j];
            }
        }
        quad_phit = transpose(quad_phi);
    }


    template <typename T, int NDIM>
    void FunctionImpl<T,NDIM>::verify_tree() const {
        world.gop.fence();  // Make sure nothing is going on
        
        // Verify consistency of compression status, existence and size of coefficients,
        // and has_children() flag.
        for(typename dcT::const_iterator it=coeffs.begin(); it!=coeffs.end(); ++it) {
            const keyT& key = it->first;
            const nodeT& node = it->second;
            bool bad;
            
            if (is_compressed()) {
                if (node.has_children()) {
                    bad = node.coeff().dim[0] != 2*cdata.k;
                }
                else {
                    bad = node.coeff().size != 0;
                }
            }
            else {
                if (node.has_children()) {
                    bad = node.coeff().size != 0;
                }
                else {
                    bad = node.coeff().dim[0] != cdata.k;
                }
            }
            
            if (bad) {
                print(world.rank(), "FunctionImpl: verify: INCONSISTENT TREE NODE, key =", key, ", node =", node,
                      ", dim[0] =",node.coeff().dim[0],", compressed =",is_compressed());
                std::cout.flush();
                MADNESS_EXCEPTION("FunctionImpl: verify: INCONSISTENT TREE NODE", 0);
            }
        }
        
        // Ensure that parents and children exist appropriately
        for(typename dcT::const_iterator it=coeffs.begin(); it!=coeffs.end(); ++it) {
            const keyT& key = it->first;
            const nodeT& node = it->second;
            
            if (key.level() > 0) {
                const keyT parent = key.parent();
                typename dcT::const_iterator pit = coeffs.find(parent).get();
                if (pit == coeffs.end()) {
                    print(world.rank(), "FunctionImpl: verify: MISSING PARENT",key,parent);
                    std::cout.flush();
                    MADNESS_EXCEPTION("FunctionImpl: verify: MISSING PARENT", 0);
                }
                const nodeT& pnode = pit->second;
                if (!pnode.has_children()) {
                    print(world.rank(), "FunctionImpl: verify: PARENT THINKS IT HAS NO CHILDREN",key,parent);
                    std::cout.flush();
                    MADNESS_EXCEPTION("FunctionImpl: verify: PARENT THINKS IT HAS NO CHILDREN", 0);
                }
            }
            
            for (KeyChildIterator<NDIM> kit(key); kit; ++kit) {
                typename dcT::const_iterator cit = coeffs.find(kit.key()).get();
                if (cit == coeffs.end()) {
                    if (node.has_children()) {
                        print(world.rank(), "FunctionImpl: verify: MISSING CHILD",key,kit.key());
                        std::cout.flush();
                        MADNESS_EXCEPTION("FunctionImpl: verify: MISSING CHILD", 0);
                    }
                }
                else {
                    if (! node.has_children()) {
                        print(world.rank(), "FunctionImpl: verify: UNEXPECTED CHILD",key,kit.key());
                        std::cout.flush();
                        MADNESS_EXCEPTION("FunctionImpl: verify: UNEXPECTED CHILD", 0);
                    }
                }
            }
        }
        
        world.gop.fence();
    }

    template <typename T, int NDIM>
    T FunctionImpl<T,NDIM>::eval_cube(Level n, coordT x, const tensorT c) const {
        const int k = cdata.k;
        double px[NDIM][k];
        T sum = T(0.0);
        
        for (int i=0; i<NDIM; i++) legendre_scaling_functions(x[i],k,px[i]);
        
        if (NDIM == 1) {
            for (int p=0; p<k; p++) 
                sum += c(p)*px[0][p];
        }
        else if (NDIM == 2) {
            for (int p=0; p<k; p++) 
                for (int q=0; q<k; q++) 
                    sum += c(p,q)*px[0][p]*px[1][q];
        }
        else if (NDIM == 3) {
            for (int p=0; p<k; p++) 
                for (int q=0; q<k; q++) 
                    for (int r=0; r<k; r++) 
                        sum += c(p,q,r)*px[0][p]*px[1][q]*px[2][r];
        }
        else if (NDIM == 4) {
            for (int p=0; p<k; p++) 
                for (int q=0; q<k; q++) 
                    for (int r=0; r<k; r++) 
                        for (int s=0; s<k; s++) 
                            sum += c(p,q,r,s)*px[0][p]*px[1][q]*px[2][r]*px[3][s];
        }
        else if (NDIM == 5) {
            for (int p=0; p<k; p++) 
                for (int q=0; q<k; q++) 
                    for (int r=0; r<k; r++) 
                        for (int s=0; s<k; s++) 
                            for (int t=0; t<k; t++) 
                                sum += c(p,q,r,s,t)*px[0][p]*px[1][q]*px[2][r]*px[3][s]*px[4][t];
        }
        else if (NDIM == 6) {
            for (int p=0; p<k; p++) 
                for (int q=0; q<k; q++) 
                    for (int r=0; r<k; r++) 
                        for (int s=0; s<k; s++) 
                            for (int t=0; t<k; t++) 
                                for (int u=0; u<k; u++) 
                                    sum += c(p,q,r,s,t,u)*px[0][p]*px[1][q]*px[2][r]*px[3][s]*px[4][t]*px[5][u];
        }
        else {
            MADNESS_EXCEPTION("FunctionImpl:eval_cube:NDIM?",NDIM);
        }
        return sum*pow(2.0,0.5*NDIM*n)/sqrt(cell_volume);
    }

    template <typename T, int NDIM>
    Void FunctionImpl<T,NDIM>::reconstruct_op(const keyT& key, const tensorT& s) {
        // Note that after application of an integral operator not all
        // siblings may be present so it is necessary to check existence
        // and if absent insert an empty leaf node.
        //
        // If summing the result of an integral operator (i.e., from
        // non-standard form) there will be significant scaling function
        // coefficients at all levels and possibly difference coefficients
        // in leaves, hence the tree may refine as a result.
        typename dcT::iterator it = coeffs.find(key).get();
        if (it == coeffs.end()) {
            coeffs.insert(key,nodeT(tensorT(),false));
            it = coeffs.find(key).get();
        }
        nodeT& node = it->second;
        
        // The integral operator will correctly connect interior nodes
        // to children but may leave interior nodes without coefficients
        // ... but they still need to sum down so just give them zeros
        if (node.has_children() && !node.has_coeff()) {
            node.set_coeff(tensorT(cdata.v2k));
        }
        
        if (node.has_coeff()) {
            tensorT d = node.coeff();
            if (key.level() > 0) d(cdata.s0) += s; // -- note accumulate for NS summation
            d = unfilter(d);
            node.clear_coeff();
            node.set_has_children(true); 
            for (KeyChildIterator<NDIM> kit(key); kit; ++kit) {
                const keyT& child = kit.key();
                tensorT ss = copy(d(child_patch(child)));
                task(coeffs.owner(child), &implT::reconstruct_op, child, ss);
            }
        }
        else {
            if (key.level()) node.set_coeff(copy(s));
            else node.set_coeff(s);
        }
        return None;
    }


    template <typename T, int NDIM>
    void FunctionImpl<T,NDIM>::fcube(const keyT& key, const FunctionFunctorInterface<T,NDIM>& f, const Tensor<double>& qx, tensorT& fval) const {
        const Vector<Translation,NDIM>& l = key.translation();
        const Level n = key.level();
        const double h = std::pow(0.5,double(n)); 
        coordT c; // will hold the point in user coordinates
        const int npt = qx.dim[0];
        
        if (NDIM == 1) {
            for (int i=0; i<npt; i++) {
                c[0] = cell(0,0) + h*cell_width[0]*(l[0] + qx(i)); // x
                fval(i) = f(c);
            }
        }
        else if (NDIM == 2) {
            for (int i=0; i<npt; i++) {
                c[0] = cell(0,0) + h*cell_width[0]*(l[0] + qx(i)); // x
                for (int j=0; j<npt; j++) {
                    c[1] = cell(1,0) + h*cell_width[1]*(l[1] + qx(j)); // y
                    fval(i,j) = f(c);
                }
            }
        }
        else if (NDIM == 3) {
            for (int i=0; i<npt; i++) {
                c[0] = cell(0,0) + h*cell_width[0]*(l[0] + qx(i)); // x
                for (int j=0; j<npt; j++) {
                    c[1] = cell(1,0) + h*cell_width[1]*(l[1] + qx(j)); // y
                    for (int k=0; k<npt; k++) {
                        c[2] = cell(2,0) + h*cell_width[2]*(l[2] + qx(k)); // z
                        fval(i,j,k) = f(c);
                    }
                }
            }
        }
        else if (NDIM == 4) {
            for (int i=0; i<npt; i++) {
                c[0] = cell(0,0) + h*cell_width[0]*(l[0] + qx(i)); // x
                for (int j=0; j<npt; j++) {
                    c[1] = cell(1,0) + h*cell_width[1]*(l[1] + qx(j)); // y
                    for (int k=0; k<npt; k++) {
                        c[2] = cell(2,0) + h*cell_width[2]*(l[2] + qx(k)); // z
                        for (int m=0; m<npt; m++) {
                            c[3] = cell(3,0) + h*cell_width[3]*(l[3] + qx(m)); // xx
                            fval(i,j,k,m) = f(c);
                        }
                    }
                }
            }
        }
        else if (NDIM == 5) {
            for (int i=0; i<npt; i++) {
                c[0] = cell(0,0) + h*cell_width[0]*(l[0] + qx(i)); // x
                for (int j=0; j<npt; j++) {
                    c[1] = cell(1,0) + h*cell_width[1]*(l[1] + qx(j)); // y
                    for (int k=0; k<npt; k++) {
                        c[2] = cell(2,0) + h*cell_width[2]*(l[2] + qx(k)); // z
                        for (int m=0; m<npt; m++) {
                            c[3] = cell(3,0) + h*cell_width[3]*(l[3] + qx(m)); // xx
                            for (int n=0; n<npt; n++) {
                                c[4] = cell(4,0) + h*cell_width[4]*(l[4] + qx(n)); // yy
                                fval(i,j,k,m,n) = f(c);
                            }
                        }
                    }
                }
            }
        }
        else if (NDIM == 6) {
            for (int i=0; i<npt; i++) {
                c[0] = cell(0,0) + h*cell_width[0]*(l[0] + qx(i)); // x
                for (int j=0; j<npt; j++) {
                    c[1] = cell(1,0) + h*cell_width[1]*(l[1] + qx(j)); // y
                    for (int k=0; k<npt; k++) {
                        c[2] = cell(2,0) + h*cell_width[2]*(l[2] + qx(k)); // z
                        for (int m=0; m<npt; m++) {
                            c[3] = cell(3,0) + h*cell_width[3]*(l[3] + qx(m)); // xx
                            for (int n=0; n<npt; n++) {
                                c[4] = cell(4,0) + h*cell_width[4]*(l[4] + qx(n)); // yy
                                for (int p=0; p<npt; p++) {
                                    c[5] = cell(5,0) + h*cell_width[5]*(l[5] + qx(p)); // zz
                                    fval(i,j,k,m,n,p) = f(c);
                                }
                            }
                        }
                    }
                }
            }
        }
        else {
            MADNESS_EXCEPTION("FunctionImpl: fcube: confused about NDIM?",NDIM);
        }
    }

    template <typename T, int NDIM>
    Void FunctionImpl<T,NDIM>::project_refine_op(const keyT& key, bool do_refine) {
        if (do_refine) {
            // Make in r child scaling function coeffs at level n+1
            tensorT r(cdata.v2k);
            for (KeyChildIterator<NDIM> it(key); it; ++it) {
                const keyT& child = it.key();
                r(child_patch(child)) = project(child);
            }
            
            // Insert empty node for parent
            coeffs.insert(key,nodeT(tensorT(),true));
            
            // Filter then test difference coeffs at level n
            tensorT d = filter(r);
            d(cdata.s0) = T(0);
            if ( (d.normf() < truncate_tol(thresh,key.level())) && 
                 (key.level() < max_refine_level) ) {
                for (KeyChildIterator<NDIM> it(key); it; ++it) {
                    const keyT& child = it.key();
                    coeffs.insert(child,nodeT(copy(r(child_patch(child))),false));
                }
            }
            else {
                if (key.level()==max_refine_level) print("MAX REFINE LEVEL",key);
                for (KeyChildIterator<NDIM> it(key); it; ++it) {
                    const keyT& child = it.key();
                    //ProcessID p = coeffs.owner(child);
                    ProcessID p = world.random_proc();
                    task(p, &implT::project_refine_op, child, do_refine); // ugh
                }
            }
        }
        else {
            coeffs.insert(key,nodeT(project(key),false));
        }
        return None;
    }

    template <typename T, int NDIM>
    void FunctionImpl<T,NDIM>::add_scalar_inplace(T t, bool fence) {
        std::vector<long> v0(NDIM,0L);
        if (is_compressed()) {
            if (world.rank() == coeffs.owner(cdata.key0)) {
                typename dcT::iterator it = coeffs.find(cdata.key0).get();
                MADNESS_ASSERT(it != coeffs.end());
                nodeT& node = it->second;
                MADNESS_ASSERT(node.has_coeff());
                node.coeff()(v0) += t*sqrt(cell_volume);
            }
        }
        else {
            for(typename dcT::iterator it=coeffs.begin(); it!=coeffs.end(); ++it) {
                Level n = it->first.level();
                nodeT& node = it->second;
                if (node.has_coeff()) {
                    node.coeff()(v0) += t*sqrt(cell_volume*pow(0.5,double(NDIM*n)));
                }
            }
        }
        if (fence) world.gop.fence();
    }

    template <typename T, int NDIM>
    void FunctionImpl<T,NDIM>::insert_zero_down_to_initial_level(const keyT& key) {
        if (coeffs.is_local(key)) {
            if (compressed) {
                coeffs.insert(key, nodeT(tensorT(cdata.v2k), key.level()<initial_level));
            }
            else if (key.level()<initial_level) {
                coeffs.insert(key, nodeT(tensorT(), true));
            }
            else {
                coeffs.insert(key, nodeT(tensorT(cdata.vk), false));
            }
        }
        if (key.level() < initial_level) {
            for (KeyChildIterator<NDIM> kit(key); kit; ++kit) {
                insert_zero_down_to_initial_level(kit.key());
            }
        }
        
    }
    
    
    template <typename T, int NDIM>
    Future<bool> FunctionImpl<T,NDIM>::truncate_spawn(const keyT& key, double tol) {
        const nodeT& node = coeffs.find(key).get()->second;   // Ugh!
        if (node.has_children()) {
            std::vector< Future<bool> > v = future_vector_factory<bool>(1<<NDIM);
            int i=0;
            for (KeyChildIterator<NDIM> kit(key); kit; ++kit,++i) {
                v[i] = send(coeffs.owner(kit.key()), &implT::truncate_spawn, kit.key(), tol);
            }
            return task(world.rank(),&implT::truncate_op, key, tol, v);
        }
        else {
            MADNESS_ASSERT(!node.has_coeff());  // In compressed form leaves should not have coeffs
            return Future<bool>(false); 
        }
    }
    

    template <typename T, int NDIM>
    bool FunctionImpl<T,NDIM>::truncate_op(const keyT& key, double tol, const std::vector< Future<bool> >& v) {
        // If any child has coefficients, a parent cannot truncate
        for (int i=0; i<(1<<NDIM); i++) if (v[i].get()) return true;
        nodeT& node = coeffs.find(key).get()->second;
        if (key.level() > 0) {
            double dnorm = node.coeff().normf();
            if (dnorm < truncate_tol(tol,key)) {
                node.clear_coeff();
                node.set_has_children(false);
                for (KeyChildIterator<NDIM> kit(key); kit; ++kit) {
                    coeffs.erase(kit.key());
                }
            }
        }
        return node.has_coeff();
    }


    
    template <typename T, int NDIM>
    void FunctionImpl<T,NDIM>::print_tree(Level maxlevel) const {
        if (world.rank() == 0) do_print_tree(cdata.key0, maxlevel);
        world.gop.fence();
        if (world.rank() == 0) std::cout.flush();
        world.gop.fence();
    }
    
    
    template <typename T, int NDIM>
    void FunctionImpl<T,NDIM>::do_print_tree(const keyT& key, Level maxlevel) const {
        typename dcT::const_iterator it = coeffs.find(key).get();
        if (it == coeffs.end()) {
            MADNESS_EXCEPTION("FunctionImpl: do_print_tree: null node pointer",0);
        }
        const nodeT& node = it->second;
        for (int i=0; i<key.level(); i++) std::cout << "  ";
        std::cout << key << "  " << node << " --> " << coeffs.owner(key) << "\n";
        if (key.level() < maxlevel  &&  node.has_children()) {
            for (KeyChildIterator<NDIM> kit(key); kit; ++kit) {
                do_print_tree(kit.key(),maxlevel);
            }
        }
    }
    
    template <typename T, int NDIM>
    Tensor<T> FunctionImpl<T,NDIM>::project(const keyT& key) const {
        tensorT fval(cdata.vq,false); // this will be the returned result
        tensorT& work = cdata.work1; // initially evaluate the function in here
        
        if (functor) {
            fcube(key,*functor,cdata.quad_x,work);
        }
        else {
            MADNESS_EXCEPTION("FunctionImpl: project: confusion about function?",0);
        }
        
        work.scale(sqrt(cell_volume*pow(0.5,double(NDIM*key.level()))));
        //return transform(work,cdata.quad_phiw);
        return fast_transform(work,cdata.quad_phiw,fval,cdata.workq);
    }

    template <typename T, int NDIM>
    Future<double> FunctionImpl<T,NDIM>::get_norm_tree_recursive(const keyT& key) const {
        if (coeffs.probe(key)) {
            return Future<double>(coeffs[key].get_norm_tree());
        }
        MADNESS_ASSERT(key.level());
        keyT parent = key.parent();
        return send(coeffs.owner(parent), &implT::get_norm_tree_recursive, parent);
    }


    template <typename T, int NDIM>
    Void FunctionImpl<T,NDIM>::sock_it_to_me(const keyT& key, 
                                             const RemoteReference< FutureImpl< std::pair<keyT,tensorT> > >& ref) const {
        if (coeffs.probe(key)) {
            const nodeT& node = coeffs.find(key).get()->second;
            Future< std::pair<keyT,tensorT> > result(ref);
            if (node.has_coeff()) {
                //madness::print("sock found it with coeff",key);
                result.set(std::pair<keyT,tensorT>(key,node.coeff()));
            }
            else {
                //madness::print("sock found it without coeff",key);
                result.set(std::pair<keyT,tensorT>(key,tensorT()));
            }
        }
        else {
            keyT parent = key.parent();
            //madness::print("sock forwarding to parent",key,parent);
            send(coeffs.owner(parent), &FunctionImpl<T,NDIM>::sock_it_to_me, parent, ref);
        }
        return None;
    }

    template <typename T, int NDIM>
    Void FunctionImpl<T,NDIM>::eval(const Vector<double,NDIM>& xin, 
                                    const keyT& keyin, 
                                    const typename Future<T>::remote_refT& ref) {
        
        // This is ugly.  We must figure out a clean way to use 
        // owner computes rule from the container.
        Vector<double,NDIM> x = xin;
        keyT key = keyin;
        Vector<Translation,NDIM> l = key.translation();
        ProcessID me = world.rank();
        while (1) {
            ProcessID owner = coeffs.owner(key);
            if (owner != me) {
                send(owner, &implT::eval, x, key, ref);
                return None;
            }
            else {
                typename dcT::futureT fut = coeffs.find(key);
                typename dcT::iterator it = fut.get();
                nodeT& node = it->second;
                if (node.has_coeff()) {
                    Future<T>(ref).set(eval_cube(key.level(), x, node.coeff()));
                    return None;
                }
                else {
                    for (int i=0; i<NDIM; i++) {
                        double xi = x[i]*2.0;
                        int li = int(xi);
                        if (li == 2) li = 1;
                        x[i] = xi - li;
                        l[i] = 2*l[i] + li;
                    }
                    key = keyT(key.level()+1,l);
                }
            }
        }
        //MADNESS_EXCEPTION("should not be here",0);
    }
    
    template <typename T, int NDIM>
    void FunctionImpl<T,NDIM>::tnorm(const tensorT& t, double* lo, double* hi) const {
        // Chosen approach looks stupid but it is more accurate
        // than the simple approach of summing everything and
        // subtracting off the low-order stuff to get the high
        // order (assuming the high-order stuff is small relative
        // to the low-order)
        cdata.work1(___) = t(___);
        tensorT tlo = cdata.work1(cdata.sh);
        *lo = tlo.normf();
        tlo.fill(0.0);
        *hi = cdata.work1.normf();
    }
    
    template <typename T, int NDIM>
    Void FunctionImpl<T,NDIM>::do_square_inplace(const keyT& key) {
        Level n = key.level();
        nodeT& node = coeffs[key];
        
        double scale = pow(2.0,0.5*NDIM*n)/sqrt(cell_volume);
        tensorT tval = transform(node.coeff(),cdata.quad_phit).scale(scale);
        
        tval.emul(tval);
        
        scale = pow(0.5,0.5*NDIM*n)*sqrt(cell_volume);
        tval = transform(tval,cdata.quad_phiw).scale(scale);
        
        node.set_coeff(tval);
        
        return None;
    }
    
    template <typename T, int NDIM>
    Void FunctionImpl<T,NDIM>::FunctionImpl<T,NDIM>::do_square_inplace2(const keyT& parent, const keyT& child, const tensorT& parent_coeff) {
        tensorT tval = fcube_for_mul (child, parent, parent_coeff);
        
        tval.emul(tval);
        
        Level n = child.level();
        double scale = pow(0.5,0.5*NDIM*n)*sqrt(cell_volume);
        tval = transform(tval,cdata.quad_phiw).scale(scale);
        
        coeffs.insert(child,nodeT(tval,false));
        
        return None;
    }
    
    template <typename T, int NDIM>
    void FunctionImpl<T,NDIM>::square_inplace(bool fence) {
        for(typename dcT::iterator it=coeffs.begin(); it!=coeffs.end(); ++it) {
            const keyT& key = it->first;
            nodeT& node = it->second;
            if (node.has_coeff()) {
                if (autorefine && autorefine_square_test(key, node.coeff())) {
                    tensorT t = node.coeff();
                    node.clear_coeff();
                    node.set_has_children(true);
                    for (KeyChildIterator<NDIM> kit(key); kit; ++kit) {         
                        const keyT& child = kit.key();
                        task(coeffs.owner(child), &implT::do_square_inplace2, key, child, t);
                    }
                }
                else {
                    task(world.rank(), &implT::do_square_inplace, key);
                }
            }
        }
        if (fence) world.gop.fence();
    }
    
    template <typename T, int NDIM>
    void FunctionImpl<T,NDIM>::diff(const implT& f, int axis, bool fence) {
        typedef std::pair<keyT,tensorT> argT;
        for(typename dcT::const_iterator it=f.coeffs.begin(); it!=f.coeffs.end(); ++it) {
            const keyT& key = it->first;
            const nodeT& node = it->second;
            if (node.has_coeff()) {
                Future<argT> left   = f.find_neighbor(key,axis,-1);
                argT center(key,node.coeff());
                Future<argT> right  = f.find_neighbor(key,axis, 1);
                task(world.rank(), &implT::do_diff1, &f, axis, key, left, center, right, task_attr_generator());
            }
            else {
                // Internal empty node can be safely inserted
                coeffs.insert(key,nodeT(tensorT(),true));
            }
        }
        if (fence) world.gop.fence();
    }
    
    template <typename T, int NDIM>
    Key<NDIM> FunctionImpl<T,NDIM>::neighbor(const keyT& key, int axis, int step) const {
        Vector<Translation,NDIM> l = key.translation();
        const Translation two2n = Translation(1)<<key.level();
        
        // Translation is an unsigned type so need to be careful if results
        // or intermediates are possibly negative.
        if (step < 0  &&  l[axis] < (unsigned)(-step)) {
            if (bc(axis,0) == 0) {
                return keyT::invalid(); // Zero BC
            }
            else if (bc(axis,0) == 1) {
                l[axis] = (l[axis] + two2n) + step; // Periodic BC
            }
            else {
                MADNESS_EXCEPTION("neighbor: confused left BC?",bc(axis,0));
            }
        }
        else if (l[axis]+step >= two2n) {
            if (bc(axis,1) == 0) {
                return keyT::invalid(); // Zero BC
            }
            else if (bc(axis,1) == 1) {
                l[axis] = (l[axis] + step) - two2n; // Periodic BC
            }
            else {
                MADNESS_EXCEPTION("neighbor: confused BC right?",bc(axis,1));
            }
        }
        else {
            l[axis] += step;
        }
        
        return keyT(key.level(),l);
    }
    
    template <typename T, int NDIM>
    Key<NDIM> FunctionImpl<T,NDIM>::neighbor(const keyT& key, const Displacement<NDIM>& d) const {
        Translation two2n = 1ul << key.level();
        Vector<Translation,NDIM> l = key.translation();
        for (int axis=0; axis<NDIM; axis++) {
            int step = d[axis];
            if (step < 0  &&  l[axis] < (unsigned)(-step)) {
                if (bc(axis,0) == 0) {
                    return keyT::invalid(); // Zero BC
                }
                else if (bc(axis,0) == 1) {
                    l[axis] = (l[axis] + two2n) + step; // Periodic BC
                }
                else {
                    MADNESS_EXCEPTION("neighbor: confused left BC?",bc(axis,0));
                }
            }
            else if (l[axis]+step >= two2n) {
                if (bc(axis,1) == 0) {
                    return keyT::invalid(); // Zero BC
                }
                else if (bc(axis,1) == 1) {
                    l[axis] = (l[axis] + step) - two2n; // Periodic BC
                }
                else {
                    MADNESS_EXCEPTION("neighbor: confused BC right?",bc(axis,1));
                }
            }
            else {
                l[axis] += step;
            }
        }
        return keyT(key.level(),l);
    }
    
    template <typename T, int NDIM>
    Future< std::pair< Key<NDIM>,Tensor<T> > > 
    FunctionImpl<T,NDIM>::find_neighbor(const Key<NDIM>& key, int axis, int step) const {
        typedef std::pair< Key<NDIM>,Tensor<T> > argT;
        keyT neigh = neighbor(key, axis, step);
        if (neigh.is_invalid()) {
            return Future<argT>(argT(neigh,tensorT(cdata.vk))); // Zero bc
        }
        else {
            Future<argT> result;
            send(coeffs.owner(neigh), &implT::sock_it_to_me, neigh, result.remote_ref(world));
            return result;
        }
    }
    
    template <typename T, int NDIM>
    Void FunctionImpl<T,NDIM>::forward_do_diff1(const implT* f, int axis, const keyT& key, 
                                                const std::pair<keyT,tensorT>& left,
                                                const std::pair<keyT,tensorT>& center,
                                                const std::pair<keyT,tensorT>& right) {
        ProcessID owner = coeffs.owner(key);
        if (owner == world.rank()) {
            if (left.second.size == 0) {
                task(owner, &implT::do_diff1, f, axis, key, 
                     f->find_neighbor(key,axis,-1), center, right, 
                     task_attr_generator());
            }
            else if (right.second.size == 0) {
                task(owner, &implT::do_diff1, f, axis, key, 
                     left, center, f->find_neighbor(key,axis,1), 
                     task_attr_generator());
            }
            else {
                task(owner, &implT::do_diff2, f, axis, key, 
                     left, center, right);
            }
        }
        else {
            send(owner, &implT::forward_do_diff1, f, axis, key, 
                 left, center, right);
        }
        return None;
    }
    
    template <typename T, int NDIM>
    Void FunctionImpl<T,NDIM>::do_diff1(const implT* f, int axis, const keyT& key, 
                                        const std::pair<keyT,tensorT>& left,
                                        const std::pair<keyT,tensorT>& center,
                                        const std::pair<keyT,tensorT>& right) {
        typedef std::pair<keyT,tensorT> argT;
        
        MADNESS_ASSERT(axis>=0 && axis<NDIM);
        
        if (left.second.size==0 || right.second.size==0) {
            // One of the neighbors is below us in the tree ... recur down
            coeffs.insert(key,nodeT(tensorT(),true));
            for (KeyChildIterator<NDIM> kit(key); kit; ++kit) {
                const keyT& child = kit.key();
                if ((child.translation()[axis]&1) == 0) { 
                    // leftmost child automatically has right sibling
                    forward_do_diff1(f, axis, child, left, center, center);
                }
                else { 
                    // rightmost child automatically has left sibling
                    forward_do_diff1(f, axis, child, center, center, right);
                }
            }
        }
        else {
            forward_do_diff1(f, axis, key, left, center, right);
        }
        return None;
    }
    
    template <typename T, int NDIM>
    Void FunctionImpl<T,NDIM>::do_diff2(const implT* f, int axis, const keyT& key, 
                                        const std::pair<keyT,tensorT>& left,
                                        const std::pair<keyT,tensorT>& center,
                                        const std::pair<keyT,tensorT>& right) {
        typedef std::pair<keyT,tensorT> argT;
        
        tensorT d = madness::inner(cdata.rp, 
                                   parent_to_child(left.second, left.first, neighbor(key,axis,-1)).swapdim(axis,0), 
                                   1, 0);
        inner_result(cdata.r0, 
                     parent_to_child(center.second, center.first, key).swapdim(axis,0),
                     1, 0, d);
        inner_result(cdata.rm, 
                     parent_to_child(right.second, right.first, neighbor(key,axis,1)).swapdim(axis,0), 
                     1, 0, d);
        if (axis) d = copy(d.swapdim(axis,0)); // make it contiguous
        d.scale(rcell_width[axis]*pow(2.0,(double) key.level()));
        coeffs.insert(key,nodeT(d,false));
        return None;
    }
    
    template <typename T, int NDIM>
    void FunctionImpl<T,NDIM>::mapdim(const implT& f, const std::vector<long>& map, bool fence) {
        for(typename dcT::const_iterator it=f.coeffs.begin(); it!=f.coeffs.end(); ++it) {
            const keyT& key = it->first;
            const nodeT& node = it->second;
            
            Vector<Translation,NDIM> l;
            for (int i=0; i<NDIM; i++) l[map[i]] = key.translation()[i];
            tensorT c = node.coeff();
            if (c.size) c = copy(c.mapdim(map));
            
            coeffs.insert(keyT(key.level(),l), nodeT(c,node.has_children()));
        }
        if (fence) world.gop.fence();
    }
    
    
    template <typename T, int NDIM>
    Future< Tensor<T> > FunctionImpl<T,NDIM>::compress_spawn(const Key<NDIM>& key, bool nonstandard) {
        MADNESS_ASSERT(coeffs.probe(key));
        nodeT& node = coeffs.find(key).get()->second;
        if (node.has_children()) {
            std::vector< Future<tensorT> > v = future_vector_factory<tensorT>(1<<NDIM);
            int i=0;
            for (KeyChildIterator<NDIM> kit(key); kit; ++kit,++i) {
                v[i] = send(coeffs.owner(kit.key()), &implT::compress_spawn, kit.key(), nonstandard);
            }
            return task(world.rank(),&implT::compress_op, key, v, nonstandard);
        }
        else {
            Future<tensorT> result(node.coeff());
            if (!nonstandard) node.clear_coeff();
            return result;
        }
    }
    
    
    
    template <typename T, int NDIM>
    FunctionCommonData<T,NDIM> FunctionCommonData<T,NDIM>::data[MAXK+1];
    
    template <int NDIM> int FunctionDefaults<NDIM>::k;               
    template <int NDIM> double FunctionDefaults<NDIM>::thresh;       
    template <int NDIM> int FunctionDefaults<NDIM>::initial_level;   
    template <int NDIM> int FunctionDefaults<NDIM>::max_refine_level;
    template <int NDIM> int FunctionDefaults<NDIM>::truncate_mode; 
    template <int NDIM> bool FunctionDefaults<NDIM>::refine;         
    template <int NDIM> bool FunctionDefaults<NDIM>::autorefine;     
    template <int NDIM> bool FunctionDefaults<NDIM>::debug;          
    template <int NDIM> Tensor<int> FunctionDefaults<NDIM>::bc;      
    template <int NDIM> Tensor<double> FunctionDefaults<NDIM>::cell ;
    template <int NDIM> SharedPtr< WorldDCPmapInterface< Key<NDIM> > > FunctionDefaults<NDIM>::pmap;


#ifdef FUNCTION_INSTANTIATE_1
    template class FunctionDefaults<1>;
    template class Function<double, 1>;
    template class Function<std::complex<double>, 1>;
    template class FunctionImpl<double, 1>;
    template class FunctionImpl<std::complex<double>, 1>;
    template class FunctionCommonData<double, 1>;
    template class FunctionCommonData<double_complex, 1>;
#endif

#ifdef FUNCTION_INSTANTIATE_2
    template class FunctionDefaults<2>;
    template class Function<double, 2>;
    template class Function<std::complex<double>, 2>;
    template class FunctionImpl<double, 2>;
    template class FunctionImpl<std::complex<double>, 2>;
    template class FunctionCommonData<double, 2>;
    template class FunctionCommonData<double_complex, 2>;
#endif

#ifdef FUNCTION_INSTANTIATE_3
    template class FunctionDefaults<3>;
    template class Function<double, 3>;
    template class Function<std::complex<double>, 3>;
    template class FunctionImpl<double, 3>;
    template class FunctionImpl<std::complex<double>, 3>;
    template class FunctionCommonData<double, 3>;
    template class FunctionCommonData<double_complex, 3>;
#endif

#ifdef FUNCTION_INSTANTIATE_4
    template class FunctionDefaults<4>;
    template class Function<double, 4>;
    template class Function<std::complex<double>, 4>;
    template class FunctionImpl<double, 4>;
    template class FunctionImpl<std::complex<double>, 4>;
    template class FunctionCommonData<double, 4>;
    template class FunctionCommonData<double_complex, 4>;
#endif

#ifdef FUNCTION_INSTANTIATE_5
    template class FunctionDefaults<5>;
    template class Function<double, 5>;
    template class Function<std::complex<double>, 5>;
    template class FunctionImpl<double, 5>;
    template class FunctionImpl<std::complex<double>, 5>;
    template class FunctionCommonData<double, 5>;
    template class FunctionCommonData<double_complex, 5>;
#endif

#ifdef FUNCTION_INSTANTIATE_6
    template class FunctionDefaults<6>;
    template class Function<double, 6>;
    template class Function<std::complex<double>, 6>;
    template class FunctionImpl<double, 6>;
    template class FunctionImpl<std::complex<double>, 6>;
    template class FunctionCommonData<double, 6>;
    template class FunctionCommonData<double_complex, 6>;
#endif

}

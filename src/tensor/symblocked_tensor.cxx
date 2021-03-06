/* Copyright (c) 2013, Devin Matthews
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following
 * conditions are met:
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL DEVIN MATTHEWS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE. */

#include <fstream>

#include "symblocked_tensor.hpp"

using namespace std;
using namespace aquarius::tensor;
using namespace aquarius::symmetry;
using namespace aquarius::task;

template <class T>
SymmetryBlockedTensor<T>::SymmetryBlockedTensor(const SymmetryBlockedTensor<T>& other)
: IndexableCompositeTensor<SymmetryBlockedTensor<T>,CTFTensor<T>,T>(other), Resource(other.arena),
  group(other.group), len(other.len), sym(other.sym), factor(other.factor), reorder(other.reorder)
{
    register_scalar();
}

template <class T>
SymmetryBlockedTensor<T>::SymmetryBlockedTensor(SymmetryBlockedTensor<T>* other)
: IndexableCompositeTensor<SymmetryBlockedTensor<T>,CTFTensor<T>,T>(other->name, other->ndim, 0), Resource(other->arena),
  group(other->group), len(other->len), sym(other->sym)
{
    factor.swap(other->factor);
    reorder.swap(other->reorder);
    tensors.swap(other->tensors);
    register_scalar();
    delete other;
}

template <class T>
SymmetryBlockedTensor<T>::SymmetryBlockedTensor(const string& name, const SymmetryBlockedTensor<T>& other)
: IndexableCompositeTensor<SymmetryBlockedTensor<T>,CTFTensor<T>,T>(name, other), Resource(other.arena),
  group(other.group), len(other.len), sym(other.sym), factor(other.factor), reorder(other.reorder)
{
    register_scalar();
}

template <class T>
SymmetryBlockedTensor<T>::SymmetryBlockedTensor(const string& name, SymmetryBlockedTensor<T>* other)
: IndexableCompositeTensor<SymmetryBlockedTensor<T>,CTFTensor<T>,T>(name, other->ndim, 0), Resource(other->arena),
  group(other->group), len(other->len), sym(other->sym)
{
    factor.swap(other->factor);
    reorder.swap(other->reorder);
    tensors.swap(other->tensors);
    register_scalar();
    delete other;
}

template <class T>
SymmetryBlockedTensor<T>::SymmetryBlockedTensor(const string& name, const SymmetryBlockedTensor<T>& other, T scalar)
: IndexableCompositeTensor<SymmetryBlockedTensor<T>,CTFTensor<T>,T>(name, 0, 0), Resource(other.arena),
  group(other.group), len(0), sym(0)
{
    factor.resize(1, 1.0);
    reorder.resize(1, vector<int>(1, 0));
    tensors.resize(1, NULL);
    tensors[0].tensor = new CTFTensor<T>(name, other.arena, scalar);
    tensors[0].isAlloced = true;
    register_scalar();
}

template <class T>
SymmetryBlockedTensor<T>::SymmetryBlockedTensor(const string& name, const SymmetryBlockedTensor<T>& A,
                                                const vector<vector<int> >& start_A,
                                                const vector<vector<int> >& len_A)
: IndexableCompositeTensor<SymmetryBlockedTensor<T>,CTFTensor<T>,T>(name, A.ndim, 0), Resource(A.arena),
  group(A.group), len(len_A), sym(A.sym)
{
    allocate(false);
    slice((T)1, false, A, start_A, (T)0);
    register_scalar();
}

template <class T>
SymmetryBlockedTensor<T>::SymmetryBlockedTensor(const string& name, const Arena& arena, const PointGroup& group,
                                                int ndim, const vector<vector<int> >& len,
                                                const vector<int>& sym, bool zero)
: IndexableCompositeTensor<SymmetryBlockedTensor<T>,CTFTensor<T>,T>(name, ndim, 0), Resource(arena),
  group(group), len(len), sym(sym)
{
    assert(sym.size() == ndim);
    assert(len.size() == ndim);

    allocate(zero);
    register_scalar();
}

template <class T>
SymmetryBlockedTensor<T>::~SymmetryBlockedTensor()
{
    unregister_scalar();
}

template <class T>
void SymmetryBlockedTensor<T>::allocate(bool zero)
{
    int n = group.getNumIrreps();
    vector<Representation> irreps;
    for (int i = 0;i < n;i++) irreps.push_back(group.getIrrep(i));

    int ntensors = 1;
    vector<int> sublen(ndim);

    for (int i = 0;i < ndim;i++)
    {
        assert(len[i].size() == n);
        ntensors *= n;
        sublen[i] = len[i][0];
    }

    tensors.resize(ntensors);
    factor.resize(ntensors, 1.0);
    reorder.resize(ntensors);

    int t = 0;
    vector<int> idx(ndim, 0);
    vector<Representation> prod(ndim+1, group.totallySymmetricIrrep());
    for (bool done = false;!done;t++)
    {
        if (prod[0].isTotallySymmetric())
        {
            reorder[t].resize(ndim);
            vector<int> subsym(sym);

            bool ok = true;
            for (int i = 0;i < ndim-1;i++)
            {
                reorder[t][i] = i;
                if (sym[i] != NS)
                {
                    if (idx[i] < idx[i+1]) subsym[i] = NS;
                    else if (idx[i] > idx[i+1]) ok = false;
                }
            }
            if (ndim > 0) reorder[t][ndim-1] = ndim-1;

            assert(t < ntensors);
            if (ok)
            {
                tensors[t].tensor = new CTFTensor<T>(this->name, this->arena, ndim, sublen, subsym, zero);
                tensors[t].isAlloced = true;
            }
        }

        for (int i = 0;i < ndim;i++)
        {
            idx[i] = (idx[i] == n-1 ? 0 : idx[i]+1);
            sublen[i] = len[i][idx[i]];

            if (idx[i] != 0)
            {
                for (int j = i;j >= 0;j--)
                {
                    prod[j] = prod[j+1];
                    prod[j] *= irreps[idx[j]];
                }
                break;
            }
            else if (i == ndim-1)
            {
                done = true;
            }
        }

        if (ndim == 0) done = true;
    }

    t = 0;
    idx.assign(ndim, 0);
    prod.assign(ndim+1, group.totallySymmetricIrrep());
    for (bool done = false;!done;t++)
    {
        if (prod[0].isTotallySymmetric() && tensors[t].tensor == NULL)
        {
            vector<int> idxreal(idx);
            for (int i = 0;i < ndim;)
            {
                int j; for (j = i;j < ndim && sym[j] != NS;j++); j++;
                cosort(   idxreal.begin()+i,    idxreal.begin()+j,
                       reorder[t].begin()+i, reorder[t].begin()+j);
                if (sym[i] == AS) factor[t] *= relativeSign(idxreal.begin()+i, idxreal.begin()+j,
                                                                idx.begin()+i,     idx.begin()+j);
                i = j;
            }

            vector<int> invorder(ndim);
            for (int i = 0;i < ndim;i++)
            {
                int j; for (j = 0;j < ndim && reorder[t][j] != i;j++);
                invorder[i] = j;
            }
            swap(invorder, reorder[t]);

            int treal = 0;
            int stride = 1;
            for (int i = 0;i < ndim;i++)
            {
                treal += stride*idxreal[i];
                stride *= n;
            }

            assert(t < ntensors && treal < ntensors);
            tensors[t].tensor = tensors[treal].tensor;
            tensors[t].ref = treal;
        }

        for (int i = 0;i < ndim;i++)
        {
            idx[i] = (idx[i] == n-1 ? 0 : idx[i]+1);
            sublen[i] = len[i][idx[i]];

            if (idx[i] != 0)
            {
                for (int j = i;j >= 0;j--)
                {
                    prod[j] = prod[j+1];
                    prod[j] *= irreps[idx[j]];
                }
                break;
            }
            else if (i == ndim-1)
            {
                done = true;
            }
        }

        if (ndim == 0) done = true;
    }
}

template <class T>
CTFTensor<T>& SymmetryBlockedTensor<T>::operator()(const vector<int>& irreps)
{
    return const_cast<CTFTensor<T>&>(const_cast<const SymmetryBlockedTensor<T>&>(*this)(irreps));
}

template <class T>
const CTFTensor<T>& SymmetryBlockedTensor<T>::operator()(const vector<int>& irreps) const
{
    assert(irreps.size() == this->ndim);

    int n = group.getNumIrreps();

    int off = 0;
    int stride = 1;
    for (int i = 0;i < ndim;i++)
    {
        off += stride*irreps[i];
        stride *= n;
    }

    assert(tensors[off] != NULL && tensors[off].isAlloced);

    return *tensors[off].tensor;
}

template <class T>
bool SymmetryBlockedTensor<T>::exists(const vector<int>& irreps) const
{
    assert(irreps.size() == this->ndim);

    int n = group.getNumIrreps();

    int off = 0;
    int stride = 1;
    for (int i = 0;i < ndim;i++)
    {
        off += stride*irreps[i];
        stride *= n;
    }

    return tensors[off] != NULL && tensors[off].isAlloced;
}

template <class T>
void SymmetryBlockedTensor<T>::slice(T alpha, bool conja, const SymmetryBlockedTensor<T>& A,
                                     const vector<vector<int> >& start_A, T beta)
{
    int n = group.getNumIrreps();
    slice(alpha, conja, A, start_A, beta, vector<vector<int> >(this->ndim,vector<int>(n,0)), len);
}

template <class T>
void SymmetryBlockedTensor<T>::slice(T alpha, bool conja, const SymmetryBlockedTensor<T>& A,
                                     T beta, const vector<vector<int> >& start_B)
{
    int n = group.getNumIrreps();
    slice(alpha, conja, A, vector<vector<int> >(this->ndim,vector<int>(n,0)), beta, start_B, A.len);
}

template <class T>
void SymmetryBlockedTensor<T>::slice(T alpha, bool conja, const SymmetryBlockedTensor<T>& A,
                                                          const vector<vector<int> >& start_A,
                                     T  beta,             const vector<vector<int> >& start_B,
                                                          const vector<vector<int> >& len)
{
    assert(this->ndim == A.ndim);

    int n = group.getNumIrreps();

    vector<vector<int> > end_A(ndim);
    vector<vector<int> > end_B(ndim);

    for (int i = 0;i < ndim;i++)
    {
        end_A[i].resize(n);
        end_B[i].resize(n);

        assert(sym[i] == A.sym[i] && sym[i] == NS);
        for (int j = 0;j < n;j++)
        {
            end_A[i][j] = start_A[i][j]+len[i][j];
            end_B[i][j] = start_B[i][j]+len[i][j];
            assert(start_A[i][j] >= 0);
            assert(start_B[i][j] >= 0);
            assert(end_A[i][j] <= A.len[i][j]);
            assert(end_B[i][j] <= this->len[i][j]);
        }
    }

    vector<int> stride(ndim,1);
    for (int i = 1;i < ndim;i++) stride[i] = stride[i-1]*n;

    int off_A = 0;
    vector<int> iA(ndim, 0);
    vector<int> start_A_sub(ndim);
    vector<int> start_B_sub(ndim);
    vector<int> len_sub(ndim);
    for (bool doneA = false;!doneA;)
    {
        if (  tensors[off_A] != NULL &&   tensors[off_A].isAlloced &&
            A.tensors[off_A] != NULL && A.tensors[off_A].isAlloced)
        {
            for (int i = 0;i < ndim;i++)
            {
                start_A_sub[i] = start_A[i][iA[i]];
                start_B_sub[i] = start_B[i][iA[i]];
                len_sub[i] = len[i][iA[i]];
            }

            tensors[off_A].tensor->slice(alpha, conja, *A.tensors[off_A].tensor, start_A_sub,
                                          beta,                                  start_B_sub, len_sub);
        }

        for (int i = 0;i < ndim;i++)
        {
            iA[i]++;
            off_A += stride[i];

            if (iA[i] == n)
            {
                iA[i] = 0;
                off_A -= stride[i]*n;
                if (i == ndim-1) doneA = true;
            }
            else
            {
                break;
            }
        }

        if (this->ndim == 0) doneA = true;
    }
}

template <class T>
vector<int> SymmetryBlockedTensor<T>::getStrides(const string& indices, const int ndim,
                                                 const int len, const string& idx_A)
{
    vector<int> strides(indices.size(), 0);
    vector<int> stride_A(ndim);

    if (ndim > 0) stride_A[0] = 1;
    for (int i = 1;i < ndim;i++)
    {
        stride_A[i] = stride_A[i-1]*len;
    }

    for (int i = 0;i < indices.size();i++)
    {
        for (int j = 0;j < ndim;j++)
        {
            if (indices[i] == idx_A[j])
            {
                strides[i] += stride_A[j];
            }
        }
    }

    return strides;
}

template <class T>
void SymmetryBlockedTensor<T>::mult(T alpha, bool conja, const SymmetryBlockedTensor<T>& A, const string& idx_A,
                                             bool conjb, const SymmetryBlockedTensor<T>& B, const string& idx_B,
                                    T  beta,                                                const string& idx_C)
{
    assert(group == A.group);
    assert(group == B.group);

    int n = group.getNumIrreps();

    string idx_A_(idx_A);
    string idx_B_(idx_B);
    string idx_C_(idx_C);
    string idx_A__(idx_A);
    string idx_B__(idx_B);
    string idx_C__(idx_C);

    double f1 = align_symmetric_indices(A.ndim, idx_A_, A.sym.data(),
                                        B.ndim, idx_B_, B.sym.data(),
                                          ndim, idx_C_,   sym.data());
    f1 *= overcounting_factor(A.ndim, idx_A_, A.sym.data(),
                              B.ndim, idx_B_, B.sym.data(),
                                ndim, idx_C_,   sym.data());


    vector<int> stride_idx_A = getStrides(idx_A_, A.ndim, n, idx_A_);
    vector<int> stride_idx_B = getStrides(idx_B_, B.ndim, n, idx_B_);
    vector<int> stride_idx_C = getStrides(idx_C_,   ndim, n, idx_C_);

    vector<char> inds(A.ndim+B.ndim+ndim);
    vector<int> syms(A.ndim+B.ndim+ndim, NS);
    vector<int> stride_A(A.ndim+B.ndim+ndim, 0);
    vector<int> stride_B(A.ndim+B.ndim+ndim, 0);
    vector<int> stride_C(A.ndim+B.ndim+ndim, 0);

    int m = 0;
    for (int i = 0;i < A.ndim;i++)
    {
        bool found = false;
        for (int j = 0;j < m;j++)
        {
            if (inds[j] == idx_A_[i])
            {
                found = true;
                stride_A[j] += stride_idx_A[i];
            }
        }
        if (!found)
        {
            stride_A[m] += stride_idx_A[i];
            inds[m++] = idx_A_[i];
            if (i == A.ndim-1) continue;

            bool in_B = false, aligned_in_B = true;
            for (int i_in_B = 0;;i_in_B++)
            {
                for (;i_in_B < B.ndim && idx_A_[i] != idx_B_[i_in_B];i_in_B++);
                if (i_in_B == B.ndim) break;

                in_B = true;

                if (A.sym[i] == NS || A.sym[i] != B.sym[i_in_B] ||
                    idx_A_[i+1] != idx_B_[i_in_B+1])
                {
                    aligned_in_B = false;
                    break;
                }
            }

            bool in_C = false, aligned_in_C = true;
            for (int i_in_C = 0;;i_in_C++)
            {
                for (;i_in_C < ndim && idx_A_[i] != idx_C_[i_in_C];i_in_C++);
                if (i_in_C == ndim) break;

                in_C = true;

                if (A.sym[i] == NS || A.sym[i] != sym[i_in_C] ||
                    idx_A_[i+1] != idx_C_[i_in_C+1])
                {
                    aligned_in_C = false;
                    break;
                }
            }

            if (!(in_B && !aligned_in_B) &&
                !(in_C && !aligned_in_C) &&
                A.sym[i] != NS) syms[m-1] = AS;
        }
    }
    for (int i = 0;i < B.ndim;i++)
    {
        bool found = false;
        for (int j = 0;j < m;j++)
        {
            if (inds[j] == idx_B_[i])
            {
                found = true;
                stride_B[j] += stride_idx_B[i];
            }
        }
        if (!found)
        {
            stride_B[m] += stride_idx_B[i];
            inds[m++] = idx_B_[i];
            if (i == B.ndim-1) continue;

            bool in_C = false, aligned_in_C = true;
            for (int i_in_C = 0;;i_in_C++)
            {
                for (;i_in_C < ndim && idx_A_[i] != idx_C_[i_in_C];i_in_C++);
                if (i_in_C == ndim) break;

                in_C = true;

                if (A.sym[i] == NS || A.sym[i] != sym[i_in_C] ||
                    idx_A_[i+1] != idx_C_[i_in_C+1])
                {
                    aligned_in_C = false;
                    break;
                }
            }

            if (!(in_C && !aligned_in_C) &&
                B.sym[i] != NS) syms[m-1] = AS;
        }
    }
    for (int i = 0;i < ndim;i++)
    {
        bool found = false;
        for (int j = 0;j < m;j++)
        {
            if (inds[j] == idx_C_[i])
            {
                found = true;
                stride_C[j] += stride_idx_C[i];
            }
        }
        if (!found)
        {
            stride_C[m] += stride_idx_C[i];
            inds[m++] = idx_C_[i];
            if (i == ndim-1) continue;
            if (sym[i] != NS) syms[m-1] = AS;
        }
    }
    inds.resize(m);
    stride_A.resize(m);
    stride_B.resize(m);
    stride_C.resize(m);

    vector<T> beta_(tensors.size(), beta);

    int off_A = 0;
    int off_B = 0;
    int off_C = 0;
    vector<int> idx(m, 0);
    for (bool done = false;!done;)
    {
        if ((A.tensors[off_A] != NULL   && B.tensors[off_B] != NULL   && tensors[off_C] != NULL  ) &&
            (A.tensors[off_A].isAlloced || B.tensors[off_B].isAlloced || tensors[off_C].isAlloced))
        {
            double f2 = overcounting_factor(A.ndim, idx_A_, A.tensors[off_A].tensor->getSymmetry().data(),
                                            B.ndim, idx_B_, B.tensors[off_B].tensor->getSymmetry().data(),
                                              ndim, idx_C_,   tensors[off_C].tensor->getSymmetry().data());
            double f3 = A.factor[off_A]*B.factor[off_B]*factor[off_C];

            for (int i = 0;i < A.ndim;i++) idx_A__[i] = idx_A_[A.reorder[off_A][i]];
            for (int i = 0;i < B.ndim;i++) idx_B__[i] = idx_B_[B.reorder[off_B][i]];
            for (int i = 0;i <   ndim;i++) idx_C__[i] = idx_C_[  reorder[off_C][i]];

            int off_C_ = (tensors[off_C].isAlloced ? off_C : tensors[off_C].ref);
            assert(off_C_ >= 0 && off_C_ < tensors.size());
            tensors[off_C].tensor->mult(alpha*f1*f3/f2, conja, *A.tensors[off_A].tensor, idx_A__,
                                                        conjb, *B.tensors[off_B].tensor, idx_B__,
                                         beta_[off_C_],                                  idx_C__);

            beta_[off_C_] = 1.0;
        }

        for (int i = 0;i < m;i++)
        {
            idx[i]++;
            off_A += stride_A[i];
            off_B += stride_B[i];
            off_C += stride_C[i];

            if (idx[i] == (syms[i] == NS ? n : idx[i+1]+1))
            {
                int lower = (i == 0 || syms[i-1] == NS ? 0 : idx[i-1]);
                off_A -= stride_A[i]*(idx[i]-lower);
                off_B -= stride_B[i]*(idx[i]-lower);
                off_C -= stride_C[i]*(idx[i]-lower);
                idx[i] = lower;
                if (i == m-1) done = true;
            }
            else
            {
                break;
            }
        }

        if (m == 0) done = true;
    }
}

template <class T>
void SymmetryBlockedTensor<T>::sum(T alpha, bool conja, const SymmetryBlockedTensor<T>& A, const string& idx_A,
                                   T  beta,                                                const string& idx_B)
{
    assert(group == A.group);

    int n = group.getNumIrreps();

    string idx_A_(idx_A);
    string idx_B_(idx_B);
    string idx_A__(idx_A);
    string idx_B__(idx_B);

    double f = align_symmetric_indices(A.ndim, idx_A_, A.sym.data(),
                                         ndim, idx_B_,   sym.data());

    string inds_A = idx_A_;
    string inds_B = idx_B_;

    vector<int> stride_idx_A = getStrides(idx_A_, A.ndim, n, idx_A_);
    vector<int> stride_idx_B = getStrides(idx_B_,   ndim, n, idx_B_);

    vector<char> inds(A.ndim+ndim);
    vector<int> syms(A.ndim+ndim, NS);
    vector<int> stride_A(A.ndim+ndim, 0);
    vector<int> stride_B(A.ndim+ndim, 0);

    int m = 0;
    for (int i = 0;i < A.ndim;i++)
    {
        bool found = false;
        for (int j = 0;j < m;j++)
        {
            if (inds[j] == idx_A_[i])
            {
                found = true;
                stride_A[j] += stride_idx_A[i];
            }
        }
        if (!found)
        {
            stride_A[m] += stride_idx_A[i];
            inds[m++] = idx_A_[i];
            if (i == A.ndim-1) continue;

            bool in_B = false, aligned_in_B = true;
            for (int i_in_B = 0;;i_in_B++)
            {
                for (;i_in_B < ndim && idx_A_[i] != idx_B_[i_in_B];i_in_B++);
                if (i_in_B == ndim) break;

                in_B = true;

                if (A.sym[i] == NS || A.sym[i] != sym[i_in_B] ||
                    idx_A_[i+1] != idx_B_[i_in_B+1])
                {
                    aligned_in_B = false;
                    break;
                }
            }

            if (!(in_B && !aligned_in_B) &&
                A.sym[i] != NS) syms[m-1] = AS;
        }
    }
    for (int i = 0;i < ndim;i++)
    {
        bool found = false;
        for (int j = 0;j < m;j++)
        {
            if (inds[j] == idx_B_[i])
            {
                found = true;
                stride_B[j] += stride_idx_B[i];
            }
        }
        if (!found)
        {
            stride_B[m] += stride_idx_B[i];
            inds[m++] = idx_B_[i];
            if (i == ndim-1) continue;
            if (sym[i] != NS) syms[m-1] = AS;
        }
    }
    inds.resize(m);
    stride_A.resize(m);
    stride_B.resize(m);

    vector<T> beta_(tensors.size(), beta);

    int off_A = 0;
    int off_B = 0;
    vector<int> idx(m, 0);
    for (bool done = false;!done;)
    {
        if ((A.tensors[off_A] != NULL   && tensors[off_B] != NULL  ) &&
            (A.tensors[off_A].isAlloced || tensors[off_B].isAlloced))
        {
            double f3 = A.factor[off_A]*factor[off_B];

            for (int i = 0;i < A.ndim;i++) idx_A__[i] = idx_A_[A.reorder[off_A][i]];
            for (int i = 0;i <   ndim;i++) idx_B__[i] = idx_B_[  reorder[off_B][i]];

            int off_B_ = (tensors[off_B].isAlloced ? off_B : tensors[off_B].ref);
            assert(off_B_ >= 0 && off_B_ < tensors.size());
            tensors[off_B].tensor->sum(   alpha*f*f3, conja, *A.tensors[off_A].tensor, idx_A__,
                                       beta_[off_B_],                                  idx_B__);
            beta_[off_B_] = 1.0;
        }

        for (int i = 0;i < m;i++)
        {
            idx[i]++;
            off_A += stride_A[i];
            off_B += stride_B[i];

            if (idx[i] == (syms[i] == NS ? n : idx[i+1]+1))
            {
                int lower = (i == 0 || syms[i-1] == NS ? 0 : idx[i-1]);
                off_A -= stride_A[i]*(idx[i]-lower);
                off_B -= stride_B[i]*(idx[i]-lower);
                idx[i] = lower;
                if (i == m-1) done = true;
            }
            else
            {
                break;
            }
        }

        if (m == 0) done = true;
    }
}

template <class T>
void SymmetryBlockedTensor<T>::scale(T alpha, const string& idx_A)
{
    int n = group.getNumIrreps();

    string inds_A(idx_A);

    uniq(inds_A);

    int nA = inds_A.size();

    vector<int> stride_A = getStrides(inds_A, ndim, n, idx_A);

    int off_A = 0;
    vector<int> iA(nA, 0);
    for (bool doneA = false;!doneA;)
    {
        if (tensors[off_A] != NULL && tensors[off_A].isAlloced)
        {
            tensors[off_A].tensor->scale(alpha, idx_A);
        }

        for (int i = 0;i < nA;i++)
        {
            iA[i]++;
            off_A += stride_A[i];

            if (iA[i] == n)
            {
                iA[i] = 0;
                off_A -= stride_A[i]*n;
                if (i == nA-1) doneA = true;
            }
            else
            {
                break;
            }
        }

        if (nA == 0) doneA = true;
    }
}

template <class T>
T SymmetryBlockedTensor<T>::dot(bool conja, const SymmetryBlockedTensor<T>& A, const string& idx_A,
                                bool conjb,                                    const string& idx_B) const
{
    SymmetryBlockedTensor<T>& sodt = scalar();
    sodt.mult(1, conja,     A, idx_A,
                 conjb, *this, idx_B,
              0,                  "");
    vector<T> vals;
    sodt(0).getAllData(vals);
    assert(vals.size() == 1);
    return vals[0];
}

template <class T>
void SymmetryBlockedTensor<T>::weight(const vector<const vector<vector<T> >*>& d)
{
    int n = group.getNumIrreps();

    vector<int> stride(ndim,1);
    for (int i = 1;i < ndim;i++) stride[i] = stride[i-1]*n;

    int off_A = 0;
    vector<int> iA(this->ndim, 0);
    vector<const vector<T>*> dsub(this->ndim);
    for (bool doneA = false;!doneA;)
    {
        if (tensors[off_A] != NULL && tensors[off_A].isAlloced)
        {
            for (int i = 0;i < this->ndim;i++) dsub[i] = &((*d[i])[iA[i]]);
            tensors[off_A].tensor->weight(dsub);
        }

        for (int i = 0;i < this->ndim;i++)
        {
            iA[i]++;
            off_A += stride[i];

            if (iA[i] == n)
            {
                iA[i] = 0;
                off_A -= stride[i]*n;
                if (i == this->ndim-1) doneA = true;
            }
            else
            {
                break;
            }
        }

        if (ndim == 0) doneA = true;
    }
}

template <class T>
typename std::real_type<T>::type SymmetryBlockedTensor<T>::norm(int p) const
{
    typename std::real_type<T>::type nrm = 0;

    int n = group.getNumIrreps();

    vector<int> stride(ndim,1);
    for (int i = 1;i < ndim;i++) stride[i] = stride[i-1]*n;

    int off_A = 0;
    vector<int> iA(this->ndim, 0);
    vector<const vector<T>*> dsub(this->ndim);
    for (bool doneA = false;!doneA;)
    {
        if (tensors[off_A] != NULL && tensors[off_A].isAlloced)
        {
            double factor = 1;
            const vector<int>& subsym = tensors[off_A].tensor->getSymmetry();
            for (int i = 0;i < ndim;)
            {
                int j; for (j = i;sym[j] != NS;j++); j++;

                int m = j-i;
                for (int k = i;k < j;)
                {
                    int l; for (l = k;subsym[l] != NS;l++); l++;
                    int o = l-k;
                    factor *= binom(m,o);
                    m -= o;
                    k = l;
                }

                i = j;
            }

            typename std::real_type<T>::type subnrm = tensors[off_A].tensor->norm(p);

            if (p == 2)
            {
                nrm += factor*subnrm*subnrm;
            }
            else if (p == 0)
            {
                nrm = max(nrm,subnrm);
            }
            else if (p == 1)
            {
                nrm += factor*subnrm;
            }
        }

        for (int i = 0;i < this->ndim;i++)
        {
            iA[i]++;
            off_A += stride[i];

            if (iA[i] == n)
            {
                iA[i] = 0;
                off_A -= stride[i]*n;
                if (i == this->ndim-1) doneA = true;
            }
            else
            {
                break;
            }
        }

        if (ndim == 0) doneA = true;
    }

    if (p == 2) nrm = sqrt(nrm);

    return nrm;
}

template<class T>
map<const tCTF_World<T>*,map<const PointGroup*,pair<int,SymmetryBlockedTensor<T>*> > > SymmetryBlockedTensor<T>::scalars;

template <typename T>
void SymmetryBlockedTensor<T>::register_scalar()
{
    if (scalars.find(&arena.ctf<T>()) == scalars.end() ||
        scalars[&arena.ctf<T>()].find(&group) == scalars[&arena.ctf<T>()].end())
    {
        /*
         * If we are the first, make a new entry and put a new
         * scalar in it. The entry in scalars must be made FIRST,
         * since the new scalar will call this constructor too.
         */
        scalars[&arena.ctf<T>()][&group].first = -1;
        scalars[&arena.ctf<T>()][&group].second = new SymmetryBlockedTensor<T>("scalar", *this, (T)0);
    }

    scalars[&arena.ctf<T>()][&group].first++;
}

template <typename T>
void SymmetryBlockedTensor<T>::unregister_scalar()
{
    /*
     * The last tensor (besides the scalar in scalars)
     * will delete the entry, so if it does not exist
     * then we must be that scalar and nothing needs to be done.
     */
    if (scalars.find(&arena.ctf<T>()) == scalars.end() ||
        scalars[&arena.ctf<T>()].find(&group) == scalars[&arena.ctf<T>()].end()) return;

    if (--scalars[&arena.ctf<T>()][&group].first == 0)
    {
        /*
         * The entry must be deleted FIRST, so that the scalar
         * knows to do nothing.
         */
        SymmetryBlockedTensor<T>* scalar = scalars[&arena.ctf<T>()][&group].second;
        scalars[&arena.ctf<T>()].erase(&group);
        if (scalars[&arena.ctf<T>()].empty()) scalars.erase(&arena.ctf<T>());
        delete scalar;
    }
}

template <typename T>
SymmetryBlockedTensor<T>& SymmetryBlockedTensor<T>::scalar() const
{
    return *scalars[&arena.ctf<T>()][&group].second;
}

INSTANTIATE_SPECIALIZATIONS(SymmetryBlockedTensor);

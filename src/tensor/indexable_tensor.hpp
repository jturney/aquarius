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

#ifndef _AQUARIUS_INDEXABLE_TENSOR_HPP_
#define _AQUARIUS_INDEXABLE_TENSOR_HPP_

#include <vector>
#include <string>
#include <algorithm>

#include "stl_ext/stl_ext.hpp"

#include "tensor.hpp"

namespace aquarius
{
namespace tensor
{

template <class Derived, class T> class IndexableTensor;
template <class Derived, class T> class IndexedTensor;
template <class Derived, class T> class IndexedTensorMult;

#define INHERIT_FROM_INDEXABLE_TENSOR(Derived,T) \
    protected: \
        using aquarius::tensor::IndexableTensor< Derived, T >::ndim; \
    public: \
        using aquarius::tensor::IndexableTensor< Derived, T >::mult; \
        using aquarius::tensor::IndexableTensor< Derived, T >::sum; \
        using aquarius::tensor::IndexableTensor< Derived, T >::scale; \
        using aquarius::tensor::IndexableTensor< Derived, T >::dot; \
        using aquarius::tensor::IndexableTensor< Derived, T >::operator=; \
        using aquarius::tensor::IndexableTensor< Derived, T >::operator+=; \
        using aquarius::tensor::IndexableTensor< Derived, T >::operator-=; \
        using aquarius::tensor::IndexableTensor< Derived, T >::operator[]; \
        using aquarius::tensor::Tensor< Derived,T >::getDerived; \
        using aquarius::tensor::Tensor< Derived,T >::operator*=; \
        using aquarius::tensor::Tensor< Derived,T >::operator/=; \
        using aquarius::tensor::Tensor< Derived,T >::operator*; \
        using aquarius::tensor::Tensor< Derived,T >::operator/; \
        Derived & operator=(const Derived & other) \
        { \
            sum((T)1, false, other, (T)0); \
            return *this; \
        } \
    private:

template <class Derived, typename T>
class IndexableTensorBase
{
    protected:
        int ndim;

    public:
        IndexableTensorBase(const int ndim = 0) : ndim(ndim) {}

        virtual ~IndexableTensorBase() {}

        Derived& getDerived() { return static_cast<Derived&>(*this); }

        const Derived& getDerived() const { return static_cast<const Derived&>(*this); }

        int getDimension() const { return ndim; }

        std::string implicit() const
        {
            std::string inds(ndim, ' ');
            for (int i = 0;i < ndim;i++) inds[i] = (char)(i+1);
            return inds;
        }

        /**********************************************************************
         *
         * Explicit indexing operations
         *
         *********************************************************************/
        IndexedTensor<Derived,T> operator[](const std::string& idx)
        {
            return IndexedTensor<Derived,T>(getDerived(), idx);
        }

        IndexedTensor<const Derived,T> operator[](const std::string& idx) const
        {
            return IndexedTensor<const Derived,T>(getDerived(), idx);
        }

        /**********************************************************************
         *
         * Implicitly indexed binary operations (inner product, trace, and weighting)
         *
         *********************************************************************/
        ENABLE_IF_SAME(Derived,cvDerived,Derived&)
        operator=(const IndexedTensorMult<cvDerived,T>& other)
        {
            (*this)[implicit()] = other;
            return getDerived();
        }

        ENABLE_IF_SAME(Derived,cvDerived,Derived&)
        operator+=(const IndexedTensorMult<cvDerived,T>& other)
        {
            (*this)[implicit()] += other;
            return getDerived();
        }

        ENABLE_IF_SAME(Derived,cvDerived,Derived&)
        operator-=(const IndexedTensorMult<cvDerived,T>& other)
        {
            (*this)[implicit()] -= other;
            return getDerived();
        }

        /**********************************************************************
         *
         * Implicitly indexed unary operations (assignment and summation)
         *
         *********************************************************************/
        ENABLE_IF_SAME(Derived,cvDerived,Derived&)
        operator=(const IndexedTensor<cvDerived,T>& other)
        {
            (*this)[implicit()] = other;
            return getDerived();
        }

        ENABLE_IF_SAME(Derived,cvDerived,Derived&)
        operator+=(const IndexedTensor<cvDerived,T>& other)
        {
            (*this)[implicit()] += other;
            return getDerived();
        }

        ENABLE_IF_SAME(Derived,cvDerived,Derived&)
        operator-=(const IndexedTensor<cvDerived,T>& other)
        {
            (*this)[implicit()] -= other;
            return getDerived();
        }

        /**********************************************************************
         *
         * Binary tensor operations (multiplication)
         *
         *********************************************************************/
        void mult(const T alpha, bool conja, const Derived& A, const std::string& idx_A,
                                 bool conjb, const Derived& B, const std::string& idx_B,
                  const T beta,                                const std::string& idx_C)
        {
            std::vector<int> idx_A_(A.getDimension());
            std::vector<int> idx_B_(B.getDimension());
            std::vector<int> idx_C_(ndim);

            for (int i = 0;i < A.getDimension();i++) idx_A_[i] = idx_A[i];
            for (int i = 0;i < B.getDimension();i++) idx_B_[i] = idx_B[i];
            for (int i = 0;i < ndim;i++)   idx_C_[i] = idx_C[i];

            mult(alpha, conja, A, idx_A_.data(),
                        conjb, B, idx_B_.data(),
                  beta,           idx_C_.data());
        }

        virtual void mult(const T alpha,  bool conja, const Derived& A, const int* idx_A,
                                          bool conjb, const Derived& B, const int* idx_B,
                          const T beta,                                 const int* idx_C) = 0;


        /**********************************************************************
         *
         * Unary tensor operations (summation)
         *
         *********************************************************************/
        void sum(const T alpha, bool conja, const Derived& A, const std::string& idx_A,
                 const T beta,                                const std::string& idx_B)
        {
            std::vector<int> idx_A_(A.getDimension());
            std::vector<int> idx_B_(ndim);

            for (int i = 0;i < A.getDimension();i++) idx_A_[i] = idx_A[i];
            for (int i = 0;i < ndim;i++) idx_B_[i] = idx_B[i];

            sum(alpha, conja, A, idx_A_.data(),
                 beta,           idx_B_.data());
        }

        virtual void sum(const T alpha, bool conja, const Derived& A, const int* idx_A,
                         const T beta,                                const int* idx_B) = 0;


        /**********************************************************************
         *
         * Scalar operations
         *
         *********************************************************************/
        void scale(const T alpha, const std::string& idx_A)
        {
            std::vector<int> idx_A_(ndim);
            for (int i = 0;i < ndim;i++) idx_A_[i] = idx_A[i];
            scale(alpha, idx_A_.data());
        }

        virtual void scale(const T alpha, const int* idx_A) = 0;

        T dot(bool conja, const Derived& A, const std::string& idx_A,
              bool conjb,                   const std::string& idx_B) const
        {
            std::vector<int> idx_A_(A.getDimension());
            std::vector<int> idx_B_(ndim);

            for (int i = 0;i < A.getDimension();i++) idx_A_[i] = idx_A[i];
            for (int i = 0;i < ndim;i++) idx_B_[i] = idx_B[i];

            return dot(conja, A, idx_A_.data(),
                       conjb,    idx_B_.data());
        }

        virtual T dot(bool conja, const Derived& A, const int* idx_A,
                      bool conjb,                   const int* idx_B) const = 0;
};

template <class Derived, typename T>
class IndexableTensor : public IndexableTensorBase<Derived,T>, public Tensor<Derived,T>
{
    INHERIT_FROM_TENSOR(Derived,T)

    protected:
        using IndexableTensorBase<Derived,T>::ndim;

    public:
        using IndexableTensorBase<Derived,T>::scale;
        using IndexableTensorBase<Derived,T>::dot;
        using IndexableTensorBase<Derived,T>::mult;
        using IndexableTensorBase<Derived,T>::sum;
        using IndexableTensorBase<Derived,T>::implicit;

        IndexableTensor(const int ndim = 0)
        : IndexableTensorBase<Derived,T>(ndim) {}

        virtual ~IndexableTensor() {}

        /**********************************************************************
         *
         * Binary tensor operations (multiplication)
         *
         *********************************************************************/
        void mult(const T alpha)
        {
            scale(alpha);
        }

        void mult(const T alpha, bool conja, const Derived& A,
                                 bool conjb, const Derived& B,
                  const T beta)
        {
            #ifdef VALIDATE_INPUTS
            if (ndim != A.getDimension() || ndim != B_.getDimension()) throw InvalidNdimError();
            #endif //VALIDATE_INPUTS

            mult(alpha, conja, A, A.implicit(),
                        conjb, B, B.implicit(),
                  beta,             implicit());
        }

        /**********************************************************************
         *
         * Unary tensor operations (summation)
         *
         *********************************************************************/
        void sum(const T alpha, const T beta)
        {
            Derived tensor(getDerived(), alpha);
            beta*(*this)[implicit()] = tensor[""];
        }

        void sum(const T alpha, bool conja, const Derived& A, const T beta)
        {
            #ifdef VALIDATE_INPUTS
            if (ndim != A.getDimension()) throw InvalidNdimError();
            #endif //VALIDATE_INPUTS

            sum(alpha, conja, A, A.implicit(),
                 beta,             implicit());
        }

        /**********************************************************************
         *
         * Scalar operations
         *
         *********************************************************************/
        void scale(const T alpha)
        {
            scale(alpha, implicit());
        }

        T dot(bool conja, const Derived& A, bool conjb) const
        {
            #ifdef VALIDATE_INPUTS
            if (ndim != A.getDimension()) throw InvalidNdimError();
            #endif //VALIDATE_INPUTS

            return dot(conja, A, A.implicit(),
                       conjb,      implicit());
        }
};

template <class Derived, typename T>
class IndexedTensor
{
    public:
        Derived& tensor_;
        std::string idx_;
        T factor_;
        bool conj_;

        template <typename cvDerived>
        IndexedTensor(const IndexedTensor<cvDerived,T>& other)
        : tensor_(other.tensor_), idx_(other.idx_), factor_(other.factor_), conj_(other.conj_) {}

        IndexedTensor(Derived& tensor, const std::string& idx, const T factor=(T)1, const bool conj=false)
        : tensor_(tensor), idx_(idx), factor_(factor), conj_(conj)
        {
            if (idx.size() != tensor.getDimension()) throw InvalidNdimError();
        }
      
        void set_name(char const * name_) { tensor_.set_name(name_); }

        /**********************************************************************
         *
         * Unary negation, conjugation
         *
         *********************************************************************/
        IndexedTensor<Derived,T> operator-() const
        {
            IndexedTensor<Derived,T> ret(*this);
            ret.factor_ = -ret.factor_;
            return ret;
        }

        friend IndexedTensor<const Derived,T> conj(const IndexedTensor<Derived,T>& other)
        {
            IndexedTensor<Derived,T> ret(other);
            ret.conj_ = !ret.conj_;
            return ret;
        }

        /**********************************************************************
         *
         * Unary tensor operations (summation)
         *
         *********************************************************************/
        IndexedTensor<Derived,T>& operator=(const IndexedTensor<Derived,T>& other)
        {
            tensor_.sum(other.factor_, other.conj_, other.tensor_, other.idx_, (T)0, idx_);
            return *this;
        }

        ENABLE_IF_SAME(Derived,cvDerived,CONCAT(IndexedTensor<Derived,T>&))
        operator=(const IndexedTensor<cvDerived,T>& other)
        {
            tensor_.sum(other.factor_, other.conj_, other.tensor_, other.idx_, (T)0, idx_);
            return *this;
        }

        ENABLE_IF_SAME(Derived,cvDerived,CONCAT(IndexedTensor<Derived,T>&))
        operator+=(const IndexedTensor<cvDerived,T>& other)
        {
            tensor_.sum(other.factor_, other.conj_, other.tensor_, other.idx_, factor_, idx_);
            return *this;
        }

        ENABLE_IF_SAME(Derived,cvDerived,CONCAT(IndexedTensor<Derived,T>&))
        operator-=(const IndexedTensor<cvDerived,T>& other)
        {
            tensor_.sum(-other.factor_, other.conj_, other.tensor_, other.idx_, factor_, idx_);
            return *this;
        }

        /**********************************************************************
         *
         * Binary tensor operations (multiplication)
         *
         *********************************************************************/
        ENABLE_IF_SAME(Derived,cvDerived,CONCAT(IndexedTensor<Derived,T>&))
        operator=(const IndexedTensorMult<cvDerived,T>& other)
        {
            tensor_.mult(other.factor_, other.A_.conj_, other.A_.tensor_, other.A_.idx_,
                                        other.B_.conj_, other.B_.tensor_, other.B_.idx_,
                                  (T)0,                                            idx_);
            return *this;
        }

        ENABLE_IF_SAME(Derived,cvDerived,CONCAT(IndexedTensor<Derived,T>&))
        operator+=(const IndexedTensorMult<cvDerived,T>& other)
        {
            tensor_.mult(other.factor_, other.A_.conj_, other.A_.tensor_, other.A_.idx_,
                                        other.B_.conj_, other.B_.tensor_, other.B_.idx_,
                               factor_,                                            idx_);
            return *this;
        }

        ENABLE_IF_SAME(Derived,cvDerived,CONCAT(IndexedTensor<Derived,T>&))
        operator-=(const IndexedTensorMult<cvDerived,T>& other)
        {
            tensor_.mult(-other.factor_, other.A_.conj_, other.A_.tensor_, other.A_.idx_,
                                         other.B_.conj_, other.B_.tensor_, other.B_.idx_,
                                factor_,                                            idx_);
            return *this;
        }

        ENABLE_IF_SAME(Derived,cvDerived,CONCAT(IndexedTensorMult<Derived,T>))
        operator*(const IndexedTensor<cvDerived,T>& other) const
        {
            return IndexedTensorMult<Derived,T>(*this, other);
        }

        ENABLE_IF_SAME(Derived,cvDerived,CONCAT(IndexedTensorMult<Derived,T>))
        operator*(const ScaledTensor<cvDerived,T>& other) const
        {
            cvDerived& B = other.tensor_.getDerived();

            if (other.conj_)
            {
                return IndexedTensorMult<Derived,T>(*this, B[B.implicit()]*other.factor_);
            }
            else
            {
                return IndexedTensorMult<Derived,T>(*this, conj(B[B.implicit()])*other.factor_);
            }
        }

        ENABLE_IF_SAME(Derived,cvDerived,CONCAT(IndexedTensorMult<Derived,T>))
        operator*(const IndexableTensor<cvDerived,T>& other) const
        {
            return IndexedTensorMult<Derived,T>(*this, other[other.implicit()]);
        }

        /**********************************************************************
         *
         * Operations with scalars
         *
         *********************************************************************/
        IndexedTensor<Derived,T> operator*(const T factor) const
        {
            IndexedTensor<Derived,T> it(*this);
            it.factor_ *= factor;
            return it;
        }

        friend IndexedTensor<Derived,T> operator*(const T factor, const IndexedTensor<Derived,T>& other)
        {
            return other*factor;
        }

        IndexedTensor<Derived,T>& operator*=(const T factor)
        {
            tensor_.scale(factor, idx_);
            return *this;
        }

        IndexedTensor<Derived,T>& operator=(const T val)
        {
            Derived tensor(tensor_, val);
            *this = tensor[""];
            return *this;
        }

        IndexedTensor<Derived,T>& operator+=(const T val)
        {
            Derived tensor(tensor_, val);
            *this += tensor[""];
            return *this;
        }

        IndexedTensor<Derived,T>& operator-=(const T val)
        {
            Derived tensor(tensor_, val);
            *this -= tensor[""];
            return *this;
        }
};

template <class Derived1, class Derived2, class T>
//typename std::enable_if<std::is_same<const Derived1, const Derived2>::value,IndexedTensorMult<Derived1,T> >::type
IndexedTensorMult<Derived1,T>
operator*(const IndexableTensorBase<Derived1,T>& t1, const IndexedTensor<Derived2,T>& t2)
{
    return IndexedTensorMult<Derived1,T>(t1[t1.implicit()], t2);
}

template <class Derived1, class Derived2, class T>
//typename std::enable_if<std::is_same<const Derived1, const Derived2>::value,IndexedTensorMult<Derived1,T> >::type
IndexedTensorMult<Derived1,T>
operator*(const ScaledTensor<Derived1,T>& t1, const IndexedTensor<Derived2,T>& t2)
{
    Derived1& A = t1.tensor_.getDerived();

    if (t1.conj_)
    {
        return IndexedTensorMult<Derived1,T>(conj(A[A.implicit()])*t1.factor_, t2);
    }
    else
    {
        return IndexedTensorMult<Derived1,T>(A[A.implicit()]*t1.factor_, t2);
    }
}

template <class Derived, typename T>
class IndexedTensorMult
{
    private:
        const IndexedTensorMult& operator=(const IndexedTensorMult<Derived,T>& other);

    public:
        IndexedTensor<const Derived,T> A_;
        IndexedTensor<const Derived,T> B_;
        T factor_;

        template <class Derived1, class Derived2>
        IndexedTensorMult(const IndexedTensor<Derived1,T>& A, const IndexedTensor<Derived2,T>& B)
        : A_(A), B_(B), factor_(A.factor_*B.factor_) {}

        /**********************************************************************
         *
         * Unary negation, conjugation
         *
         *********************************************************************/
        IndexedTensorMult<Derived,T> operator-() const
        {
            IndexedTensorMult<Derived,T> ret(*this);
            ret.factor_ = -ret.factor_;
            return ret;
        }

        friend IndexedTensorMult<Derived,T> conj(const IndexedTensorMult<Derived,T>& other)
        {
            IndexedTensorMult<Derived,T> ret(other);
            ret.A_.conj_ = !ret.A_.conj_;
            ret.B_.conj_ = !ret.B_.conj_;
            return ret;
        }

        /**********************************************************************
         *
         * Operations with scalars
         *
         *********************************************************************/
        IndexedTensorMult<Derived,T> operator*(const T factor) const
        {
            IndexedTensorMult<Derived,T> ret(*this);
            ret.factor_ *= factor;
            return ret;
        }

        IndexedTensorMult<Derived,T> operator/(const T factor) const
        {
            IndexedTensorMult<Derived,T> ret(*this);
            ret.factor_ /= factor;
            return ret;
        }

        friend IndexedTensorMult<Derived,T> operator*(const T factor, const IndexedTensorMult<Derived,T>& other)
        {
            return other*factor;
        }
};

}

/**************************************************************************
 *
 * Tensor to scalar operations
 *
 *************************************************************************/
template <class Derived, typename T>
T scalar(const tensor::IndexedTensorMult<Derived,T>& itm)
{
    return itm.factor_*itm.B_.tensor_.dot(itm.A_.conj_, itm.A_.tensor_, itm.A_.idx_,
                                          itm.B_.conj_,                 itm.B_.idx_);
}

}

#endif

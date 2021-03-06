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

#include <stdexcept>
#include <iomanip>
#include <ios>
#include <iostream>
#include <cassert>

#include "lapack.h"
#include "math_ext.h"

extern "C"
{

int roundup(int x, int y)
{
    return ((x+y-1)/y)*y;
}

double dist2(const double* a, const double* b)
{
    return (a[0]-b[0])*(a[0]-b[0]) +
           (a[1]-b[1])*(a[1]-b[1]) +
           (a[2]-b[2])*(a[2]-b[2]);
}

double dist(const double* a, const double* b)
{
    return sqrt(dist2(a, b));
}

int binom(int a, int b)
{
    int i, j;

    if (b < 0 || b > a) return 0;

    j = 1;
    for (i = 1;i <= MIN(b,a-b);i++)
    {
        j = (j*(a-i+1))/i;
    }

    return j;
}

int64_t fact(int n)
{
    int i;
    int64_t j;

    j = 1;

    for (i = n;i > 1;i--)
    {
        j = j*i;
    }

    return j;
}

int64_t dfact(int n)
{
    int i;
    int64_t j;

    j = 1;

    for (i = n;i > 1;i -= 2)
    {
        j = j*i;
    }

    return j;
}

}

using namespace std;

namespace aquarius
{

template <typename T>
void transpose(const size_t m, const size_t n, const T alpha, const T* A, const size_t lda,
                                               const T  beta,       T* B, const size_t ldb)
{
    size_t i, j;

    if (alpha == 1.0)
    {
        if (beta == 0.0)
        {
            for (i = 0;i < m;i++)
            {
                for (j = 0;j < n;j++)
                {
                    B[i*ldb + j] = A[j*lda + i];
                }
            }
        }
        else if (beta == 1.0)
        {
            for (i = 0;i < m;i++)
            {
                for (j = 0;j < n;j++)
                {
                    B[i*ldb + j] += A[j*lda + i];
                }
            }
        }
        else
        {
            for (i = 0;i < m;i++)
            {
                for (j = 0;j < n;j++)
                {
                    B[i*ldb + j] = beta*B[i*ldb + j] + A[j*lda + i];
                }
            }
        }
    }
    else
    {
        if (beta == 0.0)
        {
            for (i = 0;i < m;i++)
            {
                for (j = 0;j < n;j++)
                {
                    B[i*ldb + j] = alpha*A[j*lda + i];
                }
            }
        }
        else if (beta == 1.0)
        {
            for (i = 0;i < m;i++)
            {
                for (j = 0;j < n;j++)
                {
                    B[i*ldb + j] += alpha*A[j*lda + i];
                }
            }
        }
        else
        {
            for (i = 0;i < m;i++)
            {
                for (j = 0;j < n;j++)
                {
                    B[i*ldb + j] = beta*B[i*ldb + j] + alpha*A[j*lda + i];
                }
            }
        }
    }
}

template void transpose(const size_t m, const size_t n, const double alpha, const double* A, const size_t lda,
                                                        const double  beta,       double* B, const size_t ldb);

vec3::vec3()
{
    v[0] = 0;
    v[1] = 0;
    v[2] = 0;
};

vec3::vec3(double pos[3])
{
    v[0] = pos[0];
    v[1] = pos[1];
    v[2] = pos[2];
};

vec3::vec3(double x, double y, double z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
};

vec3::vec3(const vec3& other)
{
    v[0] = other[0];
    v[1] = other[1];
    v[2] = other[2];
};

bool vec3::operator==(const vec3& other) const
{
    return v[0] == other[0] &&
           v[1] == other[1] &&
           v[2] == other[2];
}

vec3& vec3::operator=(double other)
{
    v[0] = other;
    v[1] = other;
    v[2] = other;
    return *this;
}

vec3& vec3::operator+=(const vec3& other)
{
    v[0] += other[0];
    v[1] += other[1];
    v[2] += other[2];
    return *this;
}

vec3& vec3::operator-=(const vec3& other)
{
    v[0] -= other[0];
    v[1] -= other[1];
    v[2] -= other[2];
    return *this;
}

vec3 vec3::operator+(const vec3& other) const
{
    return vec3(*this) += other;
}

vec3 vec3::operator-(const vec3& other) const
{
    return vec3(*this) -= other;
}

vec3 vec3::operator^(const vec3& other) const
{
    return vec3(v[1]*other[2] - v[2]*other[1],
                v[2]*other[0] - v[0]*other[2],
                v[0]*other[1] - v[1]*other[0]);
}

vec3 vec3::operator%(const vec3& other) const
{
    return vec3(*this).orthogonalize(other);
}

vec3 vec3::operator-() const
{
    vec3 r;
    r[0] = -(*this)[0];
    r[1] = -(*this)[1];
    r[2] = -(*this)[2];
    return r;
}
double& vec3::operator[](int i)
{
    return v[i];
}

const double& vec3::operator[](int i) const
{
    return v[i];
}

vec3::operator double*()
{
    return v;
}

vec3::operator const double*() const
{
    return v;
}

double vec3::operator*(const vec3& other) const
{
    return v[0]*other[0] + v[1]*other[1] + v[2]*other[2];
}

vec3 vec3::operator*(const mat3x3& other) const
{
    vec3 r;
    r[0] = v[0]*other[0][0] + v[1]*other[1][0] + v[2]*other[2][0];
    r[1] = v[0]*other[0][1] + v[1]*other[1][1] + v[2]*other[2][1];
    r[2] = v[0]*other[0][2] + v[1]*other[1][2] + v[2]*other[2][2];
    return r;
}

vec3& vec3::operator*=(const double a)
{
    v[0] *= a;
    v[1] *= a;
    v[2] *= a;
    return *this;
}

vec3& vec3::operator/=(const double a)
{
    v[0] /= a;
    v[1] /= a;
    v[2] /= a;
    return *this;
}

vec3 vec3::operator*(const double a) const
{
    return vec3(*this) *= a;
}

vec3 vec3::operator/(const double a) const
{
    return vec3(*this) /= a;
}

vec3 operator*(const double a, const vec3& v)
{
    return vec3(v) *= a;
}

vec3 vec3::operator/(const vec3& other) const
{
    return vec3(*this).orthogonalize(other);
}

mat3x3 vec3::operator|(const vec3& other) const
{
    return mat3x3(v[0]*other[0],v[0]*other[1],v[0]*other[2],
                  v[1]*other[0],v[1]*other[1],v[1]*other[2],
                  v[2]*other[0],v[2]*other[1],v[2]*other[2]);
}

vec3 unit(const vec3& v)
{
    return vec3(v).normalize();
}

double norm(const vec3& v)
{
    return v.norm();
}

double vec3::norm() const
{
    return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

vec3& vec3::normalize()
{
    *this /= norm();
    return *this;
}

vec3& vec3::orthogonalize(const vec3& other)
{
    double n = other.norm();
    *this -= other*(*this*other)/n;
    return *this;
}

ostream& operator<<(ostream& os, const vec3& v)
{
    os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
    return os;
}

double& mat3x3::column::operator[](int i)
{
    switch (i)
    {
        case 0:
            return r1;
        case 1:
            return r2;
        case 2:
            return r3;
        default:
            throw out_of_range("out of range");
    }
}

const double& mat3x3::const_column::operator[](int i) const
{
    switch (i)
    {
        case 0:
            return r1;
        case 1:
            return r2;
        case 2:
            return r3;
        default:
            throw out_of_range("out of range");
    }
}

mat3x3::mat3x3()
{
    for (int i = 0;i < 3;i++)
    {
        for (int j = 0;j < 3;j++) m[i][j] = 0;
    }
}

mat3x3::mat3x3(double m00, double m01, double m02,
               double m10, double m11, double m12,
               double m20, double m21, double m22)
{
    m[0][0] = m00; m[0][1] = m01; m[0][2] = m02;
    m[1][0] = m10; m[1][1] = m11; m[1][2] = m12;
    m[2][0] = m20; m[2][1] = m21; m[2][2] = m22;
}

const mat3x3 mat3x3::operator^(int p) const
{
    assert(p >= 0);

    mat3x3 ret(1,0,0,
               0,1,0,
               0,0,1);

    for (;p > 0;p--) ret = ret*(*this);

    return ret;
}

mat3x3::column mat3x3::operator[](int i)
{
    return column(m[0][i], m[1][i], m[2][i]);
}

mat3x3::const_column mat3x3::operator[](int i) const
{
    return const_column(m[0][i], m[1][i], m[2][i]);
}

bool mat3x3::operator==(const mat3x3& other) const
{
    return m[0][0] == other[0][0] && m[0][1] == other[0][1] && m[0][2] == other[0][2] &&
           m[1][0] == other[1][0] && m[1][1] == other[1][1] && m[1][2] == other[1][2] &&
           m[2][0] == other[2][0] && m[2][1] == other[2][1] && m[2][2] == other[2][2];
}

mat3x3& mat3x3::operator=(double other)
{
    m[0][0] = other;
    m[1][0] = other;
    m[2][0] = other;
    m[0][1] = other;
    m[1][1] = other;
    m[2][1] = other;
    m[0][2] = other;
    m[1][2] = other;
    m[2][2] = other;
    return *this;
}

mat3x3 mat3x3::operator*(const mat3x3& other) const
{
    mat3x3 r;
    r[0][0] = (*this)[0][0]*other[0][0] + (*this)[0][1]*other[1][0] + (*this)[0][2]*other[2][0];
    r[0][1] = (*this)[0][0]*other[0][1] + (*this)[0][1]*other[1][1] + (*this)[0][2]*other[2][1];
    r[0][2] = (*this)[0][0]*other[0][2] + (*this)[0][1]*other[1][2] + (*this)[0][2]*other[2][2];
    r[1][0] = (*this)[1][0]*other[0][0] + (*this)[1][1]*other[1][0] + (*this)[1][2]*other[2][0];
    r[1][1] = (*this)[1][0]*other[0][1] + (*this)[1][1]*other[1][1] + (*this)[1][2]*other[2][1];
    r[1][2] = (*this)[1][0]*other[0][2] + (*this)[1][1]*other[1][2] + (*this)[1][2]*other[2][2];
    r[2][0] = (*this)[2][0]*other[0][0] + (*this)[2][1]*other[1][0] + (*this)[2][2]*other[2][0];
    r[2][1] = (*this)[2][0]*other[0][1] + (*this)[2][1]*other[1][1] + (*this)[2][2]*other[2][1];
    r[2][2] = (*this)[2][0]*other[0][2] + (*this)[2][1]*other[1][2] + (*this)[2][2]*other[2][2];
    return r;
}

mat3x3 mat3x3::operator+(const mat3x3& other) const
{
    mat3x3 r;
    r[0][0] = (*this)[0][0] + other[0][0];
    r[0][1] = (*this)[0][1] + other[0][1];
    r[0][2] = (*this)[0][2] + other[0][2];
    r[1][0] = (*this)[1][0] + other[1][0];
    r[1][1] = (*this)[1][1] + other[1][1];
    r[1][2] = (*this)[1][2] + other[1][2];
    r[2][0] = (*this)[2][0] + other[2][0];
    r[2][1] = (*this)[2][1] + other[2][1];
    r[2][2] = (*this)[2][2] + other[2][2];
    return r;
}

mat3x3 mat3x3::operator-(const mat3x3& other) const
{
    mat3x3 r;
    r[0][0] = (*this)[0][0] - other[0][0];
    r[0][1] = (*this)[0][1] - other[0][1];
    r[0][2] = (*this)[0][2] - other[0][2];
    r[1][0] = (*this)[1][0] - other[1][0];
    r[1][1] = (*this)[1][1] - other[1][1];
    r[1][2] = (*this)[1][2] - other[1][2];
    r[2][0] = (*this)[2][0] - other[2][0];
    r[2][1] = (*this)[2][1] - other[2][1];
    r[2][2] = (*this)[2][2] - other[2][2];
    return r;
}

mat3x3& mat3x3::operator+=(const mat3x3& other)
{
    m[0][0] += other[0][0];
    m[0][1] += other[0][1];
    m[0][2] += other[0][2];
    m[1][0] += other[1][0];
    m[1][1] += other[1][1];
    m[1][2] += other[1][2];
    m[2][0] += other[2][0];
    m[2][1] += other[2][1];
    m[2][2] += other[2][2];
    return *this;
}

mat3x3& mat3x3::operator-=(const mat3x3& other)
{
    m[0][0] -= other[0][0];
    m[0][1] -= other[0][1];
    m[0][2] -= other[0][2];
    m[1][0] -= other[1][0];
    m[1][1] -= other[1][1];
    m[1][2] -= other[1][2];
    m[2][0] -= other[2][0];
    m[2][1] -= other[2][1];
    m[2][2] -= other[2][2];
    return *this;
}

mat3x3 operator+(double other, const mat3x3& m)
{
    return m+other;
}

mat3x3 operator-(double other, const mat3x3& m)
{
    return -(m-other);
}

mat3x3 operator*(double other, const mat3x3& m)
{
    return m*other;
}

mat3x3 mat3x3::operator+(double other) const
{
    mat3x3 r;
    r[0][0] = (*this)[0][0] + other;
    r[0][1] = (*this)[0][1];
    r[0][2] = (*this)[0][2];
    r[1][0] = (*this)[1][0];
    r[1][1] = (*this)[1][1] + other;
    r[1][2] = (*this)[1][2];
    r[2][0] = (*this)[2][0];
    r[2][1] = (*this)[2][1];
    r[2][2] = (*this)[2][2] + other;
    return r;
}

mat3x3 mat3x3::operator-(double other) const
{
    mat3x3 r;
    r[0][0] = (*this)[0][0] - other;
    r[0][1] = (*this)[0][1];
    r[0][2] = (*this)[0][2];
    r[1][0] = (*this)[1][0];
    r[1][1] = (*this)[1][1] - other;
    r[1][2] = (*this)[1][2];
    r[2][0] = (*this)[2][0];
    r[2][1] = (*this)[2][1];
    r[2][2] = (*this)[2][2] - other;
    return r;
}

mat3x3 mat3x3::operator*(double other) const
{
    mat3x3 r;
    r[0][0] = (*this)[0][0]*other;
    r[0][1] = (*this)[0][1]*other;
    r[0][2] = (*this)[0][2]*other;
    r[1][0] = (*this)[1][0]*other;
    r[1][1] = (*this)[1][1]*other;
    r[1][2] = (*this)[1][2]*other;
    r[2][0] = (*this)[2][0]*other;
    r[2][1] = (*this)[2][1]*other;
    r[2][2] = (*this)[2][2]*other;
    return r;
}

mat3x3 mat3x3::operator/(double other) const
{
    mat3x3 r;
    r[0][0] = (*this)[0][0]/other;
    r[0][1] = (*this)[0][1]/other;
    r[0][2] = (*this)[0][2]/other;
    r[1][0] = (*this)[1][0]/other;
    r[1][1] = (*this)[1][1]/other;
    r[1][2] = (*this)[1][2]/other;
    r[2][0] = (*this)[2][0]/other;
    r[2][1] = (*this)[2][1]/other;
    r[2][2] = (*this)[2][2]/other;
    return r;
}

mat3x3 mat3x3::operator-() const
{
    mat3x3 r;
    r[0][0] = -(*this)[0][0];
    r[0][1] = -(*this)[0][1];
    r[0][2] = -(*this)[0][2];
    r[1][0] = -(*this)[1][0];
    r[1][1] = -(*this)[1][1];
    r[1][2] = -(*this)[1][2];
    r[2][0] = -(*this)[2][0];
    r[2][1] = -(*this)[2][1];
    r[2][2] = -(*this)[2][2];
    return r;
}

vec3 mat3x3::operator*(const vec3& other) const
{
    vec3 r;
    r[0] = (*this)[0][0]*other[0] + (*this)[0][1]*other[1] + (*this)[0][2]*other[2];
    r[1] = (*this)[1][0]*other[0] + (*this)[1][1]*other[1] + (*this)[1][2]*other[2];
    r[2] = (*this)[2][0]*other[0] + (*this)[2][1]*other[1] + (*this)[2][2]*other[2];
    return r;
}

mat3x3::operator double*()
{
    return &m[0][0];
}

mat3x3::operator const double*() const
{
    return &m[0][0];
}

void mat3x3::diagonalize(vec3& eigenvalues, mat3x3& eigenvectors) const
{
    eigenvectors = *this;
    dsyev('V','U',3,(double*)eigenvectors.m,3,eigenvalues.v);
}

mat3x3 mat3x3::identity()
{
    mat3x3 i;
    i[0][0] = 1;
    i[1][1] = 1;
    i[2][2] = 1;
    return i;
}

double mat3x3::norm() const
{
    return sqrt(m[0][0]*m[0][0] + m[0][1]*m[0][1] + m[0][2]*m[0][2] +
                m[1][0]*m[1][0] + m[1][1]*m[1][1] + m[1][2]*m[1][2] +
                m[2][0]*m[2][0] + m[2][1]*m[2][1] + m[2][2]*m[2][2]);
}

double norm(const mat3x3& m)
{
    return m.norm();
}

ostream& operator<<(ostream& os, const mat3x3& m)
{
    ios_base::fmtflags oldfmt = os.flags();
    os << scientific << showpos << setprecision(12) << setw(20);
    os <<  "/" << m[0][0] << " " << m[0][1] << " " << m[0][2] << "\\\n";
    os <<  "|" << m[1][0] << " " << m[1][1] << " " << m[1][2] << "|\n";
    os << "\\" << m[2][0] << " " << m[2][1] << " " << m[2][2] << "/\n";
    os.flags(oldfmt);
    return os;
}

}

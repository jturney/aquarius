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

#ifndef _AQUARIUS_SCF_HPP_
#define _AQUARIUS_SCF_HPP_

#include <vector>
#include <cmath>
#include <complex>
#include <iomanip>

#include "tensor/symblocked_tensor.hpp"
#include "integrals/1eints.hpp"
#include "input/molecule.hpp"
#include "input/config.hpp"
#include "util/util.h"
#include "util/blas.h"
#include "util/lapack.h"
#include "util/distributed.hpp"
#include "util/iterative.hpp"
#include "convergence/diis.hpp"
#include "task/task.hpp"
#include "operator/space.hpp"

namespace aquarius
{
namespace scf
{

template <typename T>
class UHF : public Iterative
{
    protected:
        bool frozen_core;
        T damping;
        std::vector<int> occ_alpha, occ_beta;
        std::vector<std::vector<typename std::real_type<T>::type> > E_alpha, E_beta;
        aquarius::convergence::DIIS< tensor::SymmetryBlockedTensor<T> > diis;

    public:
        UHF(const std::string& type, const std::string& name, const input::Config& config);

        void iterate();

        void run(task::TaskDAG& dag, const Arena& arena);

    protected:
        void calcSMinusHalf();

        void calcS2();

        void diagonalizeFock();

        virtual void buildFock() = 0;

        void calcEnergy();

        void calcDensity();

        void DIISExtrap();
};

}
}

#endif

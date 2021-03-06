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

#include "input/config.hpp"
#include "input/molecule.hpp"
#include "scf/aoscf.hpp"
#include "operator/aomoints.hpp"
#include "cc/ccsdt.hpp"
#include "time/time.hpp"

#ifdef USE_ELEMENTAL
#include "elemental.hpp"
using namespace elem;
#endif

using namespace std;
using namespace MPI;
using namespace aquarius;
using namespace aquarius::integrals;
using namespace aquarius::input;
using namespace aquarius::scf;
using namespace aquarius::cc;
using namespace aquarius::op;
using namespace aquarius::time;
using namespace aquarius::tensor;

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
#ifdef USE_ELEMENTAL
    elem::Initialize(argc, argv);
#endif

    {
        int i;
        double dt;
        Arena<double> world(argc, argv);

        assert(argc > 1);
        Schema schema(TOPDIR "/input_schema");
        Config config(argv[1]);
        schema.apply(config);

        Molecule mol(config);

        PRINT("nA: %d\n", mol.getNumOrbitals()-mol.getNumAlphaElectrons());
        PRINT("na: %d\n", mol.getNumOrbitals()-mol.getNumBetaElectrons());
        PRINT("nI: %d\n", mol.getNumAlphaElectrons());
        PRINT("ni: %d\n", mol.getNumBetaElectrons());

        tic();
        ERI<double> ints(world, Context(), mol);
        dt = todouble(toc());
        PRINT("\nAO integrals took: %8.3f s\n", dt);

        tic();
        AOUHF<double> scf(config.get("scf"), ints);

        CTF_Timer t_scf("SCF");
        t_scf.start();
        PRINT("\nUHF-SCF\n\n");
        PRINT("It.            SCF Energy     Residual Walltime\n");
        tic();
        for (i = 0;scf.iterate();i++)
        {
            dt = todouble(toc());
            PRINT("%3d % 21.15f %12.6e %8.3f\n", i+1, scf.getEnergy(), scf.getConvergence(), dt);
            tic();
        }
        toc();

        PRINT("\nUHF Orbital Energies\n\n");
        for (int i = 0;i < mol.getNumOrbitals();i++)
        {
            PRINT("%4d ", i+1);

            if (i < mol.getNumAlphaElectrons())
            {
                PRINT("%21.15f a ", scf.getAlphaEigenvalues()[i]);
            }
            else
            {
                PRINT("%21.15f   ", scf.getAlphaEigenvalues()[i]);
            }

            if (i < mol.getNumBetaElectrons())
            {
                PRINT("%21.15f b ", scf.getBetaEigenvalues()[i]);
            }
            else
            {
                PRINT("%21.15f   ", scf.getBetaEigenvalues()[i]);
            }

            PRINT("\n");
        }

        dt = todouble(toc());
        PRINT("\nAO SCF took: %8.3f s (%8.3f s/it.)\n", dt, dt/i);
        t_scf.stop();

        double s2 = scf.getS2();
        double mult = scf.getMultiplicity();
        double na = scf.getAvgNumAlpha();
        double nb = scf.getAvgNumBeta();

        PRINT("\n");
        PRINT("<0|S^2|0>     = %f\n", s2);
        PRINT("<0|2S+1|0>    = %f\n", mult);
        PRINT("<0|n_alpha|0> = %f\n", na);
        PRINT("<0|n_beta|0>  = %f\n", nb);

        tic();
        AOMOIntegrals<double> moints(scf);
        dt = todouble(toc());
        PRINT("\nAO MO took: %8.3f s\n", dt);

        CTF_Timer t_mp2("MP2");
        t_mp2.start();
        CCSDT<double> ccsdt(config.get("cc"), moints);
        t_mp2.stop();

        PRINT("\nUHF-MP2 Energy: %.15f\n", ccsdt.getEnergy());

        s2 = ccsdt.getProjectedS2();
        mult = ccsdt.getProjectedMultiplicity();

        PRINT("\n");
        PRINT("<0|S^2|MP2>  = %f\n", s2);
        PRINT("<0|2S+1|MP2> = %f\n", mult);

        CTF_Timer t_ccsdt("CCSD");
        t_ccsdt.start();
        PRINT("\nUHF-CCSDT\n\n");
        PRINT("It.   Correlation Energy     Residual Walltime\n");
        tic();
        tic();
        for (i = 0;ccsdt.iterate();i++)
        {
            dt = todouble(toc());
            PRINT("%3d % 20.15f %12.6e %8.3f\n", i+1, ccsdt.getEnergy(), ccsdt.getConvergence(), dt);
            tic();
        }
        toc();
        t_ccsdt.stop();

        dt = todouble(toc());
        PRINT("\nCCSDT took: %8.3f s (%8.3f s/it.)\n", dt, dt/i);

        s2 = ccsdt.getProjectedS2();
        mult = ccsdt.getProjectedMultiplicity();

        PRINT("\n");
        PRINT("<0|S^2|CC>  = %f\n", s2);
        PRINT("<0|2S+1|CC> = %f\n", mult);
        PRINT("\n");

        PRINT("Final Energy: %.15f\n\n", scf.getEnergy()+ccsdt.getEnergy());

        print_timers();
    }

#ifdef USE_ELEMENTAL
    elem::Finalize();
#endif
    MPI_Finalize();
}

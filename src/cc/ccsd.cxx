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

#include "cc.hpp"

#include <cfloat>
#include <iostream>

using namespace std;
using namespace aquarius::tensor;
using namespace aquarius::time;
using namespace aquarius::input;
using namespace aquarius::scf;

namespace aquarius
{
namespace cc
{

CCSD::CCSD(const Config& config, MOIntegrals& moints)
: Distributed<double>(moints.ctf), Iterative(config), moints(moints),
  T1("a,i"), E1("a,i"), D1("a,i"), Z1("a,i"),
  T2("ab,ij"), E2("ab,ij"), D2("ab,ij"), Z2("ab,ij"),
  diis(config.get("diis"), 2, 2)
{
    int N = moints.getSCF().getMolecule().getNumOrbitals();
    int nI = moints.getSCF().getMolecule().getNumAlphaElectrons();
    int ni = moints.getSCF().getMolecule().getNumBetaElectrons();
    int nA = N-nI;
    int na = N-ni;

    int sizeAI[] = {nA, nI};
    int sizeai[] = {na, ni};
    int sizeAAII[] = {nA, nA, nI, nI};
    int sizeAaIi[] = {nA, na, nI, ni};
    int sizeaaii[] = {na, na, ni, ni};

    int shapeNN[] = {NS, NS};
    int shapeNNNN[] = {NS, NS, NS, NS};
    int shapeANAN[] = {AS, NS, AS, NS};

    T1.addSpinCase(new DistTensor<double>(ctf, 2, sizeAI, shapeNN, false), "A,I", "AI");
    T1.addSpinCase(new DistTensor<double>(ctf, 2, sizeai, shapeNN, false), "a,i", "ai");

    E1.addSpinCase(new DistTensor<double>(ctf, 2, sizeAI, shapeNN, false), "A,I", "AI");
    E1.addSpinCase(new DistTensor<double>(ctf, 2, sizeai, shapeNN, false), "a,i", "ai");

    D1.addSpinCase(new DistTensor<double>(ctf, 2, sizeAI, shapeNN, false), "A,I", "AI");
    D1.addSpinCase(new DistTensor<double>(ctf, 2, sizeai, shapeNN, false), "a,i", "ai");

    Z1.addSpinCase(new DistTensor<double>(ctf, 2, sizeAI, shapeNN, false), "A,I", "AI");
    Z1.addSpinCase(new DistTensor<double>(ctf, 2, sizeai, shapeNN, false), "a,i", "ai");

    T2.addSpinCase(new DistTensor<double>(ctf, 4, sizeAAII, shapeANAN, false), "AB,IJ", "ABIJ");
    T2.addSpinCase(new DistTensor<double>(ctf, 4, sizeAaIi, shapeNNNN, false), "Ab,Ij", "AbIj");
    T2.addSpinCase(new DistTensor<double>(ctf, 4, sizeaaii, shapeANAN, false), "ab,ij", "abij");

    E2.addSpinCase(new DistTensor<double>(ctf, 4, sizeAAII, shapeANAN, false), "AB,IJ", "ABIJ");
    E2.addSpinCase(new DistTensor<double>(ctf, 4, sizeAaIi, shapeNNNN, false), "Ab,Ij", "AbIj");
    E2.addSpinCase(new DistTensor<double>(ctf, 4, sizeaaii, shapeANAN, false), "ab,ij", "abij");

    D2.addSpinCase(new DistTensor<double>(ctf, 4, sizeAAII, shapeANAN, false), "AB,IJ", "ABIJ");
    D2.addSpinCase(new DistTensor<double>(ctf, 4, sizeAaIi, shapeNNNN, false), "Ab,Ij", "AbIj");
    D2.addSpinCase(new DistTensor<double>(ctf, 4, sizeaaii, shapeANAN, false), "ab,ij", "abij");

    Z2.addSpinCase(new DistTensor<double>(ctf, 4, sizeAAII, shapeANAN, false), "AB,IJ", "ABIJ");
    Z2.addSpinCase(new DistTensor<double>(ctf, 4, sizeAaIi, shapeNNNN, false), "Ab,Ij", "AbIj");
    Z2.addSpinCase(new DistTensor<double>(ctf, 4, sizeaaii, shapeANAN, false), "ab,ij", "abij");

    D1["ai"]  = moints.getFIJ()["ii"];
    D1["ai"] -= moints.getFAB()["aa"];

    D2["abij"]  = moints.getFIJ()["ii"];
    D2["abij"] += moints.getFIJ()["jj"];
    D2["abij"] -= moints.getFAB()["aa"];
    D2["abij"] -= moints.getFAB()["bb"];

    int64_t size;
    double * data;
    data = D1.getSpinCase(0).getRawData(size);
    for (int i=0; i<size; i++){
      if (fabs(data[i]) > DBL_MIN)
        data[i] = 1./data[i];
    }
    data = D1.getSpinCase(1).getRawData(size);
    for (int i=0; i<size; i++){
      if (fabs(data[i]) > DBL_MIN)
        data[i] = 1./data[i];
    }
    data = D2.getSpinCase(0).getRawData(size);
    for (int i=0; i<size; i++){
      if (fabs(data[i]) > DBL_MIN)
        data[i] = 1./data[i];
    }
    data = D2.getSpinCase(1).getRawData(size);
    for (int i=0; i<size; i++){
      if (fabs(data[i]) > DBL_MIN)
        data[i] = 1./data[i];
    }
    data = D2.getSpinCase(2).getRawData(size);
    for (int i=0; i<size; i++){
      if (fabs(data[i]) > DBL_MIN)
        data[i] = 1./data[i];
    }

    T1["ai"] = moints.getFAI()["ai"]*D1["ai"];
    T2["abij"] = moints.getVABIJ()["abij"]*D2["abij"];

    SpinorbitalTensor< DistTensor<double> > Tau(T2);
    Tau["abij"] += 0.5*T1["ai"]*T1["bj"];

    energy = 0.25*scalar(moints.getVABIJ()["efmn"]*Tau["efmn"]);

    conv =          T1.getSpinCase(0).reduce(CTF_OP_MAXABS);
    conv = max(conv,T1.getSpinCase(1).reduce(CTF_OP_MAXABS));
    conv = max(conv,T2.getSpinCase(0).reduce(CTF_OP_MAXABS));
    conv = max(conv,T2.getSpinCase(1).reduce(CTF_OP_MAXABS));
    conv = max(conv,T2.getSpinCase(2).reduce(CTF_OP_MAXABS));
}

void CCSD::_iterate()
{
    Hamiltonian H(moints, Hamiltonian::FAE|
                          Hamiltonian::FMI|
                          Hamiltonian::FME|
                          Hamiltonian::WMNIJ|
                          Hamiltonian::WMNIE|
                          Hamiltonian::WMBIJ|
                          Hamiltonian::WMBEJ);

    SpinorbitalTensor< DistTensor<double> >& FME = H.getFME();
    SpinorbitalTensor< DistTensor<double> >& FAE = H.getFAE();
    SpinorbitalTensor< DistTensor<double> >& FMI = H.getFMI();
    SpinorbitalTensor< DistTensor<double> >& WMNEF = H.getWMNEF();
    SpinorbitalTensor< DistTensor<double> >& WAMEF = H.getWAMEF();
    SpinorbitalTensor< DistTensor<double> >& WABEJ = H.getWABEJ();
    SpinorbitalTensor< DistTensor<double> >& WABEF = H.getWABEF();
    SpinorbitalTensor< DistTensor<double> >& WMNIJ = H.getWMNIJ();
    SpinorbitalTensor< DistTensor<double> >& WMNIE = H.getWMNIE();
    SpinorbitalTensor< DistTensor<double> >& WMBIJ = H.getWMBIJ();
    SpinorbitalTensor< DistTensor<double> >& WMBEJ = H.getWMBEJ();

    //FAE["aa"] = 0.0;
    //FMI["ii"] = 0.0;

    FAE = 0.0;
    FMI = 0.0;

    SpinorbitalTensor< DistTensor<double> > Tau(T2);
    Tau["abij"] += 0.5*T1["ai"]*T1["bj"];

    /**************************************************************************
     *
     * Intermediates for T1->T1 and T2->T1
     */
    PROFILE_SECTION(calc_FEM)
    FME["me"] += WMNEF["mnef"]*T1["fn"];
    PROFILE_STOP

    PROFILE_SECTION(calc_FMI)
    FMI["mi"] += 0.5*WMNEF["mnef"]*T2["efin"];
    FMI["mi"] += FME["me"]*T1["ei"];
    FMI["mi"] += WMNIE["mnif"]*T1["fn"];
    PROFILE_STOP

    PROFILE_SECTION(calc_WIJKL)
    WMNIJ["mnij"] += 0.5*WMNEF["mnef"]*Tau["efij"];
    WMNIJ["mnij"] += WMNIE["mnie"]*T1["ej"];
    PROFILE_STOP

    PROFILE_SECTION(calc_WMNIE)
    WMNIE["mnie"] += WMNEF["mnfe"]*T1["fi"];
    PROFILE_STOP
    /*
     *************************************************************************/

    /**************************************************************************
     *
     * T1->T1 and T2->T1
     */
    PROFILE_SECTION(calc_FAI)
    Z1["ai"] = moints.getFAI()["ai"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T1_IN_T1_RING)
    Z1["ai"] += T1["em"]*WMBEJ["maei"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T2_IN_T1_ABCI)
    Z1["ai"] += 0.5*WAMEF["amef"]*Tau["efim"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T2_IN_T1_IJKA)
    Z1["ai"] -= 0.5*WMNIE["mnie"]*T2["aemn"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T2_IN_T1_FME)
    Z1["ai"] += T2["aeim"]*FME["me"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T1_IN_T1_FAE)
    Z1["ai"] += T1["ei"]*FAE["ae"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T1_IN_T1_FMI)
    Z1["ai"] -= T1["am"]*FMI["mi"];
    PROFILE_STOP
    /*
     *************************************************************************/

    /**************************************************************************
     *
     * Intermediates for T1->T2 and T2->T2
     */
    PROFILE_SECTION(calc_FAE)
    FAE["ae"] -= 0.5*WMNEF["mnef"]*T2["afmn"];
    FAE["ae"] -= FME["me"]*T1["am"];
    FAE["ae"] += WAMEF["amef"]*T1["fm"];
    PROFILE_STOP

    PROFILE_SECTION(calc_WAIJK)
    WMBIJ["mbij"] += 0.5*WAMEF["bmfe"]*Tau["efij"];
    WMBIJ["mbij"] += WMBEJ["mbej"]*T1["ei"];
    PROFILE_STOP

    PROFILE_SECTION(calc_WMBEJ)
    WMBEJ["maei"] += 0.5*WMNEF["mnef"]*T2["afin"];
    WMBEJ["maei"] += WAMEF["amfe"]*T1["fi"];
    WMBEJ["maei"] -= WMNIE["nmie"]*T1["an"];
    PROFILE_STOP
    /*
     *************************************************************************/

    /**************************************************************************
     *
     * T1->T2 and T2->T2
     */
    PROFILE_SECTION(calc_WMNEF)
    Z2["abij"] = WMNEF["ijab"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T2_IN_T2_FAE)
    Z2["abij"] += FAE["af"]*T2["fbij"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T2_IN_T2_FMI)
    Z2["abij"] -= FMI["ni"]*T2["abnj"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T1_IN_T2_ABCI)
    Z2["abij"] += WABEJ["abej"]*T1["ei"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T1_IN_T2_IJKA)
    Z2["abij"] -= WMBIJ["mbij"]*T1["am"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T2_IN_T2_ABCD)
    Z2["abij"] += 0.5*WABEF["abef"]*Tau["efij"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T2_IN_T2_IJKL)
    Z2["abij"] += 0.5*WMNIJ["mnij"]*Tau["abmn"];
    PROFILE_STOP

    PROFILE_SECTION(calc_T2_IN_T2_RING)
    Z2["abij"] += WMBEJ["maei"]*T2["ebmj"];
    PROFILE_STOP
    /*
     *************************************************************************/

    PROFILE_SECTION(calc_EN)

    E1["ai"]     = Z1["ai"]*D1["ai"];
    E1["ai"]    -= T1["ai"];
    T1["ai"]    += E1["ai"];
    E2["abij"]   = Z2["abij"]*D2["abij"];
    E2["abij"]  -= T2["abij"];
    T2["abij"]  += E2["abij"];

    Tau["abij"]  = T2["abij"];
    Tau["abij"] += 0.5*T1["ai"]*T1["bj"];
    energy = 0.25*scalar(WMNEF["mnef"]*Tau["efmn"]);

    conv =          E1.getSpinCase(0).reduce(CTF_OP_MAXABS);
    conv = max(conv,E1.getSpinCase(1).reduce(CTF_OP_MAXABS));
    conv = max(conv,E2.getSpinCase(0).reduce(CTF_OP_MAXABS));
    conv = max(conv,E2.getSpinCase(1).reduce(CTF_OP_MAXABS));
    conv = max(conv,E2.getSpinCase(2).reduce(CTF_OP_MAXABS));

    E2 *= 0.5;

    vector<SpinorbitalTensor< DistTensor<double> >*> T(2);
    T[0] = &T1;
    T[1] = &T2;
    vector<SpinorbitalTensor< DistTensor<double> >*> E(2);
    E[0] = &E1;
    E[1] = &E2;
    diis.extrapolate(T, E);

    PROFILE_STOP
}

}
}

#ifndef MEDIUMRESPONSE_HH
#define MEDIUMRESPONSE_HH

#include <random>
#include <string>
#include <tuple>

#include "G4Step.hh"
#include "G4SystemOfUnits.hh"

#include <CLHEP/Random/RandExponential.h>
#include <CLHEP/Random/RandPoisson.h>
#include <CLHEP/Random/RandGauss.h>
#include <CLHEP/Random/RandBinomial.h>

namespace G4LArBox
{
    class MediumResponse
    {
    public:
        MediumResponse();
        ~MediumResponse();

        void Excitation(const G4Step* step, int nexc, int nion);
        void Recombination(const G4Step* step, int nexc, int nion, int nopt, int ntherm, double r);
        void ExcitationQuenching(int nopt, int ntherm, double r, double Qopt);
        void RunProcesses(const G4Step* step, int nexc, int nion, int nopt, int ntherm, double r, double Qopt);
        void GenerateResponse(const G4Step* step, int nexc, int nion, int nopt, int ntherm, double r);
    };
}

#endif // MEDIUMRESPONSE_HH

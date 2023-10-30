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
        MediumResponse(int& nexc, int& nion, int& nopt, int& ntherm, double& r);
        ~MediumResponse();

        void Excitation(const G4Step* step);
        void Recombination(const G4Step* step);
        void ExcitationQuenching(double Q);
        void RunProcesses(const G4Step* step, double Q);
        void GenerateResponse(const G4Step* step);

    private: 
        int& nexc_;
        int& nion_;
        int& nopt_;
        int& ntherm_;
        double& r_;
    };
}

#endif // MEDIUMRESPONSE_HH

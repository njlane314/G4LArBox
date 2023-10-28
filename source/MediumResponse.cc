#include "MediumResponse.hh"

namespace G4LArBox
{
    void MediumResponse::Excitation(const G4Step* step, int nexc, int nion)
    {    
        double Wion = 23.6 * eV;
        double ionth = 15.8 * eV;
        double excth = 19.5 * eV;
        double thermls = 0.057 * eV;
        double fano = 0.106;

        double npairs = step->GetTotalEnergyDeposit() / Wion;

        if (npairs > 30) 
        {
            double res = std::sqrt(fano * npairs);
            nion = static_cast<int>(std::round(CLHEP::RandGauss::shoot(npairs, res)));
        }
        else 
        {
            nion = static_cast<int>(std::round(CLHEP::RandPoisson::shoot(npairs)));
        }

        double eexc = step->GetTotalEnergyDeposit() - nion * (ionth + thermls);
        if (eexc > excth) 
        {
            nexc = static_cast<int>(std::round(eexc / excth));
        }
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void MediumResponse::Recombination(const G4Step* step, int nexc, int nion, int nopt, int ntherm, double r) 
    {
        double Efld = 0.5;
        double dEdx = step->GetTotalEnergyDeposit() / step->GetStepLength();
        if (dEdx < 1) 
        {
            dEdx = 1;
        }

        auto ChargeRecombination = [](double dEdx, double Efld) 
        {
            auto BirksRecombination = [](double dEdx, double Efld) 
            {
                double ARecomb = 0.800;
                double kRecomb = 0.0486;

                return ARecomb / (1. + dEdx * kRecomb / Efld);
            };

            auto EscapeRecombination = [](double dEdx, double Efld) 
            {
                double larqlChi0A = 0.00338427;
                double larqlChi0B = -6.57037;
                double larqlChi0C = 1.88418;
                double larqlChi0D = 0.000129379;

                double escaping_fraction = larqlChi0A / (larqlChi0B + exp(larqlChi0C + larqlChi0D * dEdx));
                
                if (dEdx < 1) 
                {
                    dEdx = 1;
                }

                double larqlAlpha = 0.0372;
                double larqlBeta = 0.0124;

                double field_correction = std::exp(-Efld  / (larqlAlpha * std::log(dEdx) + larqlBeta));

                return escaping_fraction * field_correction;
            };

            return BirksRecombination(dEdx, Efld) + EscapeRecombination(dEdx, Efld);
        };

        double r = ChargeRecombination(0.5, Efld);

        ntherm = static_cast<int>(std::round(CLHEP::RandBinomial::shoot(ntherm, 1 - r)));
        if (ntherm < 0) 
        {
            ntherm = 0;
        }

        nopt = nexc + (nion - ntherm);
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void MediumResponse::ExcitationQuenching(int nopt, int ntherm, double r, double Qopt) 
    [
        nopt = CLHEP::RandBinomial::shoot(nopt, Qopt);
    ]

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void MediumResponse::RunProcesses(const G4Step* step, int nexc, int nion, int nopt, int ntherm, double r) 
    {
        Excitation(step, nexc, nion);
        Recombination(step, nexc, nion, nopt, ntherm, r);
        ExcitationQuenching(nopt, ntherm, r, Qopt)
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void MediumResponse::GenerateResponse(const G4Step* step, int nexc, int nion, int nopt, int ntherm, double r)
    {
        nexc, nion, nopt, ntherm = 0;
        r = 0;

        double Qopt = 1; // Scintillation quenching factor

        if (step->GetTotalEnergyDeposit() > 0) {
            const std::string p = step->GetTrack()->GetDefinition()->GetParticleName();
            if (p == "e+" || 
                p == "e-" || 
                p == "mu+" || 
                p == "mu-" || 
                p == "proton" || 
                p == "anti_proton" || 
                p == "pi+" || 
                p == "pi-" || 
                p == "kaon+" || 
                p == "kaon-" || 
                p == "alpha") 
            {
                if (p == "alpha")
                {
                    Qopt = 0.7;
                }
                RunProcesses(step, nexc, nion, nopt, ntherm, r, Qopt);
            }
            else 
            {
                nexc = 0;
                nion = 0;
                nopt = 0;
                ntherm = 0;
                r = 0;
            }
        }
    }
}
#include "MediumResponse.hh"

namespace G4LArBox
{
    MediumResponse::MediumResponse(int& nexc, int& nion, int& nopt, int& ntherm, double& r) 
    : nexc_(nexc), nion_(nion), nopt_(nopt), ntherm_(ntherm), r_(r)
    {}

    MediumResponse::~MediumResponse()
    {}

    void MediumResponse::Excitation(const G4Step* step)
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
            nion_ = static_cast<int>(std::round(CLHEP::RandGauss::shoot(npairs, res)));
        }
        else 
        {
            nion_ = static_cast<int>(std::round(CLHEP::RandPoisson::shoot(npairs)));
        }

        double eexc = step->GetTotalEnergyDeposit() - nion_ * (ionth + thermls);
        if (eexc > excth) 
        {
            nexc_ = static_cast<int>(std::round(eexc / excth));
        }
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void MediumResponse::Recombination(const G4Step* step) 
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

        r_ = ChargeRecombination(dEdx, Efld);

        ntherm_ = static_cast<int>(std::round(CLHEP::RandBinomial::shoot(nion_, r_)));

        if (ntherm_ < 0) 
        {
            ntherm_ = 0;
        }

        nopt_ = nexc_ + (nion_ - ntherm_);
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void MediumResponse::ExcitationQuenching(double Q) 
    {
        if (Q < 1) 
        {
            nopt_ = CLHEP::RandBinomial::shoot(nopt_, Q);
        }
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void MediumResponse::RunProcesses(const G4Step* step, double Q) 
    {
        Excitation(step);
        Recombination(step);
        ExcitationQuenching(Q);
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void MediumResponse::GenerateResponse(const G4Step* step)
    {
        nexc_ = 0;
        nion_ = 0;
        nopt_ = 0;
        ntherm_ = 0;
        r_ = 0;

        double Q = 1; // Scintillation quenching factor

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
                    Q = 0.7;
                }
                RunProcesses(step, Q);
            }
            else 
            {
                nexc_ = 0;
                nion_ = 0;
                nopt_ = 0;
                ntherm_ = 0;
                r_ = 0;
            }
        }
    }
}
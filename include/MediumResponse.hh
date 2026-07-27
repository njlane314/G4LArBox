#ifndef MEDIUMRESPONSE_HH
#define MEDIUMRESPONSE_HH

class G4Step;

namespace G4LArBox
{
    class MediumResponse
    {
    public:
        MediumResponse(int& nexc, int& nion, int& nopt, int& ntherm, double& r);
        void GenerateResponse(const G4Step* step);

    private:
        void Excitation(const G4Step* step);
        void Recombination(const G4Step* step);
        void ExcitationQuenching(double Q);
        void RunProcesses(const G4Step* step, double Q);

        int& nexc_;
        int& nion_;
        int& nopt_;
        int& ntherm_;
        double& r_;
    };
}

#endif

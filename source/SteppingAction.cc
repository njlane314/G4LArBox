#include "SteppingAction.hh"

namespace G4LArBox
{
    SteppingAction::SteppingAction()
    {}

    SteppingAction::~SteppingAction()
    {}

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void SteppingAction::UserSteppingAction(const G4Step* step)
    {
        int nexc, nion, nopt, ntherm; 
        double r;
        
        MediumResponse* ResponseModel = new MediumResponse(nexc, nion, nopt, ntherm, r);
        ResponseModel->GenerateResponse(step);

        std::cout << "----- SteppingAction Debug Info -----\n"
                  << "nexc: " << nexc << '\n'
                  << "nion: " << nion << '\n'
                  << "nopt: " << nopt << '\n'
                  << "ntherm: " << ntherm << '\n'
                  << "r: " << r << '\n'
                  << "-------------------------------------\n";
        
        DataHandler::Instance()->AddStep(step);
        
        delete ResponseModel;
    }
}
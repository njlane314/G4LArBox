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
        int nexc = 0, nion = 0, nopt = 0, ntherm = 0;
        double r = 0;
        
        MediumResponse* ResponseModel = new MediumResponse();
        ResponseModel->GenerateResponse(step, nexc, nion, nopt, ntherm, r);
        
        DataHandler::Instance()->AddStep(step);
    }
}
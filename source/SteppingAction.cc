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
        
        DataHandler::Instance()->AddStep(step, nexc, nion, nopt, ntherm);
        
        delete ResponseModel;
    }
}
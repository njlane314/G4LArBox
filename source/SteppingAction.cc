#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"

namespace G4LArBox
{
    SteppingAction::SteppingAction(EventAction* eventAction)
    : fEventAction(eventAction)
    {}

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void SteppingAction::UserSteppingAction(const G4Step* step)
    {
        int nexc = 0, nion = 0;
        G4LArBox::MediumResponse::GenerateResponse(step, nexc, nion);
        
        G4LArBox::DataHandler* datahandler = G4LArBox::DataHandler::GetInstance();
        datahandler->AddHit(step, nexc, nion);
    }
}
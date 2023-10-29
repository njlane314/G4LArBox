#include "RunAction.hh"

namespace G4LArBox
{
    RunAction::RunAction()
    {}

    RunAction::~RunAction()
    {}

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void RunAction::BeginOfRunAction(const G4Run*)
    {}

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void RunAction::EndOfRunAction(const G4Run* run)
    {
        DataHandler::Instance()->WriteFile();
    }
}

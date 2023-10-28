#include "RunAction.hh"
#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4AccumulableManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

namespace G4LArBox
{
    RunAction::RunAction()
    {}

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void RunAction::BeginOfRunAction(const G4Run*)
    {}

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void RunAction::EndOfRunAction(const G4Run* run)
    {
        G4int nofEvents = run->GetNumberOfEvent();
        if (nofEvents == 0) return;

        // Print
        //
        if (IsMaster()) {
        G4cout
        << G4endl
        << "--------------------End of Global Run-----------------------";
        }
        else {
        G4cout
        << G4endl
        << "--------------------End of Local Run------------------------";
        }

        G4cout
        << G4endl
        << "------------------------------------------------------------"
        << G4endl
        << G4endl;
    }
}

#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"

namespace G4LArBox 
{
    void ActionInitialization::BuildForMaster() const
    {
      auto runact = new RunAction;
      SetUserAction(runact);
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void ActionInitialization::Build() const
    {
      SetUserAction(new PrimaryGeneratorAction);

      auto runact = new RunAction;
      SetUserAction(runact);

      auto evtact = new EventAction(runact);
      SetUserAction(evtact);

      auto trackact = new TrackingAction(evtact);

      SetUserAction(new SteppingAction(evtact));
    }
}

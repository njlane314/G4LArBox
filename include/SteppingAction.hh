#ifndef STEPPINGACTION_HH
#define STEPPINGACTION_HH

#include "MediumResponse.hh"

#include "G4UserSteppingAction.hh"
#include "globals.hh"

#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"

namespace G4LArBox
{
  class SteppingAction : public G4UserSteppingAction
  {
    public:
      SteppingAction();
      ~SteppingAction();

      void UserSteppingAction(const G4Step*);
  };
}

#endif // STEPPINGACTION_HH
#ifndef STEPPINGACTION_HH
#define STEPPINGACTION_HH

#include "G4UserSteppingAction.hh"
#include "globals.hh"

namespace G4LArBox
{
  class SteppingAction : public G4UserSteppingAction
  {
    public:
      SteppingAction(EventAction* eventAction);
      ~SteppingAction();

      void UserSteppingAction(const G4Step*);
  };
}

#endif // STEPPINGACTION_HH
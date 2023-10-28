#ifndef EVENT_ACTION_HH
#define EVENT_ACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"

namespace G4LArBox
{
  class EventAction : public G4UserEventAction
  {
    public:
      EventAction(RunAction* runAction);
      ~EventAction();

      void BeginOfEventAction(const G4Event* event);
      void EndOfEventAction(const G4Event* event);
  };
}

#endif // EVENT_ACTION_HH
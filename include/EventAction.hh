#ifndef EVENT_ACTION_HH
#define EVENT_ACTION_HH

#include "G4UserEventAction.hh"

class G4Event;

namespace G4LArBox
{
    class EventAction final : public G4UserEventAction
    {
    public:
        void EndOfEventAction(const G4Event*) override;
    };
}

#endif

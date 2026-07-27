#ifndef STEPPINGACTION_HH
#define STEPPINGACTION_HH

#include "G4UserSteppingAction.hh"

namespace G4LArBox
{
    class SteppingAction final : public G4UserSteppingAction
    {
    public:
        void UserSteppingAction(const G4Step* step) override;
    };
}

#endif

#ifndef ACTION_INITIALISATION_HH
#define ACTION_INITIALISATION_HH

#include "G4VUserActionInitialization.hh"

namespace G4LArBox
{
    class ActionInitialisation final : public G4VUserActionInitialization
    {
    public:
        void Build() const override;
    };
}

#endif

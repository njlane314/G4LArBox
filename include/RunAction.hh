#ifndef RUNACTION_HH
#define RUNACTION_HH

#include "G4UserRunAction.hh"

class G4Run;

namespace G4LArBox
{
    class RunAction final : public G4UserRunAction
    {
    public:
        void EndOfRunAction(const G4Run*) override;
    };
}

#endif

#ifndef TRACKINGACTION_HH
#define TRACKINGACTION_HH

#include "G4UserTrackingAction.hh"

class G4Track;

namespace G4LArBox 
{
    class TrackingAction final : public G4UserTrackingAction
    {
    public:
        void PostUserTrackingAction(const G4Track* track) override;
    };
}

#endif

#ifndef TRACKINGACTION_HH
#define TRACKINGACTION_HH

#include "G4Track.hh"
#include "G4UserTrackingAction.hh"

#include "DataHandler.hh"

namespace G4LArBox 
{
    class TrackingAction : public G4UserTrackingAction
    {
    public:
        TrackingAction();
        virtual ~TrackingAction();

        void PreUserTrackingAction(const G4Track* track);
        void PostUserTrackingAction(const G4Track* track);
    };
}

#endif // TRACKINGACTION_HH
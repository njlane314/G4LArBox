#include "TrackingAction.hh"

#include "DataHandler.hh"

namespace G4LArBox 
{
    void TrackingAction::PostUserTrackingAction(const G4Track* track) 
    {
        DataHandler::Instance().AddTrack(track);
    }
}

#include "TrackingAction.hh"

namespace G4LArBox 
{
    TrackingAction::TrackingAction() 
    {}

    TrackingAction::~TrackingAction() 
    {}

    void TrackingAction::PreUserTrackingAction(const G4Track* track) 
    {}

    void TrackingAction::PostUserTrackingAction(const G4Track* track) 
    {
        DataHandler::Instance()->AddTrack(track);
    }
}
#include "TrackingAction.hh"

#include "DataHandler.hh"

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
        if (track->GetParentID() != 0) 
        {
            DataHandler::GetInstance()->AddSecondary(track);
        }
    }
}
#include "EventAction.hh"

#include "DataHandler.hh"

namespace G4LArBox
{
    void EventAction::EndOfEventAction(const G4Event*)
    {
        DataHandler::Instance().AddEntry();
        DataHandler::Instance().Reset();
    }
}

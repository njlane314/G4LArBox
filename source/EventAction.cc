#include "EventAction.hh"

namespace G4LArBox
{
    EventAction::EventAction()
    {}

    EventAction::~EventAction()
    {}

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void EventAction::BeginOfEventAction(const G4Event* event) 
    {
        std::cout << "-- Starting new event..." << std::endl;
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void EventAction::EndOfEventAction(const G4Event*)
    {
        DataHandler::Instance()->AddEntry();
        DataHandler::Instance()->Reset();
    }
}

#include "EventAction.hh"
#include "RunAction.hh"

#include "G4Event.hh"
#include "G4RunManager.hh"

namespace G4LArBox
{
    EventAction::EventAction(RunAction* runAction)
    : fRunAction(runAction)
    {}

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void EventAction::BeginOfEventAction(const G4Event* event) 
    {
        DataHandler* datahandler = DataHandler::GetInstance();
        datahandler->Reset();

        for (int i = 0; i < event->GetNumberOfPrimaryVertex(); ++i) 
        {
            G4PrimaryVertex* vertex = event->GetPrimaryVertex(i);
            
            for (int j = 0; j < vertex->GetNumberOfParticle(); ++j) 
            {
                G4PrimaryParticle* particle = vertex->GetPrimary(j);
                
                PrimaryParticle primaryParticle(particle);
                datahandler->AddPrimary(primaryParticle);
            }
        }
    }

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void EventAction::EndOfEventAction(const G4Event*)
    {
        DataHandler* datahandler = DataHandler::GetInstance();
        dataHandler->WriteToFile();
    }
}

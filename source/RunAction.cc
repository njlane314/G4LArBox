#include "RunAction.hh"

#include "DataHandler.hh"

namespace G4LArBox
{
    void RunAction::EndOfRunAction(const G4Run*)
    {
        DataHandler::Instance().WriteFile();
    }
}

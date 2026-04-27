#include "SteppingAction.hh"

#include "DataHandler.hh"
#include "G4OpticalPhoton.hh"
#include "G4VPhysicalVolume.hh"

#include <cstdlib>
#include <string>

namespace G4LArBox
{
    namespace
    {
        bool FastOpticalResponseEnabled()
        {
            static const bool enabled = []() {
                const char* value = std::getenv("G4LARBOX_FAST_OPTICAL");
                if (value == nullptr)
                {
                    return false;
                }
                const std::string text(value);
                return text == "1" || text == "true" || text == "TRUE" ||
                       text == "on" || text == "ON" || text == "yes" || text == "YES";
            }();
            return enabled;
        }

        bool IsOpticalDetectorVolume(const G4VPhysicalVolume* volume)
        {
            if (volume == nullptr || volume->GetLogicalVolume() == nullptr)
            {
                return false;
            }

            const G4String logical_name = volume->GetLogicalVolume()->GetName();
            return logical_name == "volSiPM_Sens" ||
                   logical_name == "volSiPM" ||
                   logical_name.find("volOpDetSensitive_") == 0;
        }
    }

    SteppingAction::SteppingAction()
    {}

    SteppingAction::~SteppingAction()
    {}

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void SteppingAction::UserSteppingAction(const G4Step* step)
    {
        G4Track* track = step->GetTrack();
        if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition())
        {
            if (FastOpticalResponseEnabled())
            {
                track->SetTrackStatus(fStopAndKill);
                return;
            }

            const G4VPhysicalVolume* pre_volume = step->GetPreStepPoint()->GetPhysicalVolume();
            const G4VPhysicalVolume* post_volume = step->GetPostStepPoint()->GetPhysicalVolume();
            if (post_volume != pre_volume && IsOpticalDetectorVolume(post_volume))
            {
                DataHandler::Instance()->AddOpticalHit(step);
                track->SetTrackStatus(fStopAndKill);
            }
            return;
        }

        int nexc, nion, nopt, ntherm; 
        double r;
        
        MediumResponse* ResponseModel = new MediumResponse(nexc, nion, nopt, ntherm, r);
        ResponseModel->GenerateResponse(step);
        
        DataHandler::Instance()->AddStep(step, nexc, nion, nopt, ntherm);
        
        delete ResponseModel;
    }
}

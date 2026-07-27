#include "SteppingAction.hh"

#include "DataHandler.hh"
#include "MediumResponse.hh"

#include "G4Material.hh"
#include "G4Step.hh"

namespace G4LArBox
{
    void SteppingAction::UserSteppingAction(const G4Step* step)
    {
        int excitons = 0;
        int ions = 0;
        int photons = 0;
        int electrons = 0;
        double recombination = 0.0;

        const auto* material = step->GetPreStepPoint()->GetMaterial();
        if (material != nullptr && material->GetName() == "G4_lAr")
        {
            MediumResponse response(
                excitons, ions, photons, electrons, recombination);
            response.GenerateResponse(step);
        }

        DataHandler::Instance().AddStep(
            step, excitons, ions, photons, electrons);
    }
}

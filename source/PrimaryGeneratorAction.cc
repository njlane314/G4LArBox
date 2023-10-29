#include "PrimaryGeneratorAction.hh"

namespace G4LArBox
{
    PrimaryGeneratorAction::PrimaryGeneratorAction()
    {}

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    PrimaryGeneratorAction::~PrimaryGeneratorAction()
    {}

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

    void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
    {}

    void PrimaryGeneratorAction::BulkVertexGenerator(G4ThreeVector& vtx)
    {   
        // In your PrimaryGeneratorAction class or method:

        auto detectorConstruction = static_cast<const DetectorConstruction*>(G4RunManager::GetRunManager()->GetUserDetectorConstruction());

        G4VPhysicalVolume* pVolume = detectorConstruction->GetWorldVolume();
        G4Box* box = dynamic_cast<G4Box*>(pVolume->GetLogicalVolume()->GetSolid());
        if(box) 
        {
            double wbox = box->GetXHalfLength();
            double hbox = box->GetYHalfLength();
            double lbox = box->GetZHalfLength();

            double x0 = CLHEP::RandFlat::shoot(-wbox/2, wbox/2);
            double y0 = CLHEP::RandFlat::shoot(-hbox/2, hbox/2);
            double z0 = CLHEP::RandFlat::shoot(-lbox/2, lbox/2);

            vtx.setX(x0);
            vtx.setY(y0);
            vtx.setZ(z0);
        }
        else 
        {
            return;
        }
    }
}
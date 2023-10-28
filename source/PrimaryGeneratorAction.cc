#include "PrimaryGeneratorAction.hh"

#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4RunManager.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

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
        double x0 = CLHEP::RandFlat::shoot(-detector_width_/2, detector_width_/2);
        double y0 = CLHEP::RandFlat::shoot(-detector_height_/2, detector_height_/2);
        double z0 = CLHEP::RandFlat::shoot(-detector_depth_/2, detector_depth_/2);

        vtx.setX(x0);
        vtx.setY(y0);
        vtx.setZ(z0);
    }
}
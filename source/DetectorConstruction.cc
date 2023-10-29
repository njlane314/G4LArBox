#include "DetectorConstruction.hh"

namespace G4LArBox
{
    DetectorConstruction::DetectorConstruction() = default;
    DetectorConstruction::~DetectorConstruction() = default;
    
    G4VPhysicalVolume* DetectorConstruction::Construct()
    {
        std::string material = "G4_lAr";
        G4double wbox = 1*m, hbox = 1*m, lbox = 1*m;

        G4NistManager* nist = G4NistManager::Instance();
        G4Material* box_material = nist->FindOrBuildMaterial(material);

        G4Box* box = new G4Box("LArBox", wbox/2, hbox/2, lbox/2);
        G4LogicalVolume* box_logical = new G4LogicalVolume(box, box_material, "lArBox");
        box_physical = new G4PVPlacement(nullptr, G4ThreeVector(), box_logical, "lArBox.physical", nullptr, false, 0);

        G4VisAttributes* box_vis = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0));
        box_vis->SetForceSolid(true);
        box_logical->SetVisAttributes(box_vis);

        return box_physical;  
    }
}
#include "DetectorConstruction.hh"

namespace G4LArBox
{
    DetectorConstruction::DetectorConstruction(Messenger* messenger)
    : messenger_(messenger)
    {}
    DetectorConstruction::~DetectorConstruction() = default;
    
    G4VPhysicalVolume* DetectorConstruction::Construct()
    {
        std::string material = "G4_lAr";
        
        messenger_->GetBoxDimensions(wbox_, hbox_, lbox_);

        G4NistManager* nist = G4NistManager::Instance();
        G4Material* box_material = nist->FindOrBuildMaterial(material);

        G4Box* box = new G4Box("lArBox", wbox_/2, hbox_/2, lbox_/2);
        G4LogicalVolume* box_logical = new G4LogicalVolume(box, box_material, "lArBox");

        box_physical = new G4PVPlacement(nullptr, G4ThreeVector(), box_logical, "lArBox.physical", nullptr, false, 0);

        G4VisAttributes* box_vis = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0));
        box_vis->SetForceSolid(true);
        box_logical->SetVisAttributes(box_vis);

        return box_physical;  
    }
}
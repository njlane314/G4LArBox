#include "DetectorConstruction.hh"

#include "Messenger.hh"

#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"

#include <algorithm>
#include <stdexcept>

namespace G4LArBox
{
    DetectorConstruction::DetectorConstruction()
        : messenger_(std::make_unique<Messenger>())
    {}

    DetectorConstruction::~DetectorConstruction() = default;

    G4VPhysicalVolume* DetectorConstruction::Construct()
    {
        G4double width = 0.0;
        G4double height = 0.0;
        G4double length = 0.0;
        messenger_->GetBoxDimensions(width, height, length);

        auto* nist = G4NistManager::Instance();
        auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
        auto* liquid_argon = nist->FindOrBuildMaterial("G4_lAr");
        if (vacuum == nullptr || liquid_argon == nullptr)
        {
            throw std::runtime_error("Could not construct the box materials.");
        }

        const G4double world_margin =
            std::max(2.0 * m, std::max(width, std::max(height, length)));
        auto* world_solid = new G4Box("world",
                                     width / 2.0 + world_margin,
                                     height / 2.0 + world_margin,
                                     length / 2.0 + world_margin);
        auto* world_logical = new G4LogicalVolume(world_solid, vacuum, "world");
        world_volume_ = new G4PVPlacement(nullptr,
                                          {},
                                          world_logical,
                                          "world",
                                          nullptr,
                                          false,
                                          0);

        auto* box_solid =
            new G4Box("lar_box", width / 2.0, height / 2.0, length / 2.0);
        auto* box_logical =
            new G4LogicalVolume(box_solid, liquid_argon, "lar_box");
        active_volume_ = new G4PVPlacement(nullptr,
                                           {},
                                           box_logical,
                                           "lar_box",
                                           world_logical,
                                           false,
                                           0);

        world_logical->SetVisAttributes(G4VisAttributes::GetInvisible());
        auto* box_style = new G4VisAttributes(G4Colour(0.0, 0.2, 0.8, 0.25));
        box_style->SetForceSolid(true);
        box_logical->SetVisAttributes(box_style);

        return world_volume_;
    }
}

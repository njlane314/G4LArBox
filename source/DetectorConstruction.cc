#include "DetectorConstruction.hh"

#include "G4GDMLParser.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4PhysicalVolumeStore.hh"

#include <stdexcept>
#include <vector>

namespace G4LArBox
{
    DetectorConstruction::DetectorConstruction(Messenger* messenger)
    : messenger_(messenger),
      world_physical_(nullptr),
      active_physical_(nullptr)
    {}
    DetectorConstruction::~DetectorConstruction() = default;
    
    G4VPhysicalVolume* DetectorConstruction::Construct()
    {
        const G4String& gdml_file = messenger_->GetGDMLFile();
        if (!gdml_file.empty())
        {
            return ConstructGDMLGeometry(gdml_file);
        }

        return ConstructBoxGeometry();
    }

    G4VPhysicalVolume* DetectorConstruction::ConstructBoxGeometry()
    {
        messenger_->GetBoxDimensions(wbox_, hbox_, lbox_);

        G4NistManager* nist = G4NistManager::Instance();
        G4Material* world_material = nist->FindOrBuildMaterial("G4_Galactic");
        if (world_material == nullptr)
        {
            throw std::runtime_error("Failed to construct the world material G4_Galactic.");
        }

        G4Material* box_material = nist->FindOrBuildMaterial("G4_lAr");
        if (box_material == nullptr)
        {
            throw std::runtime_error("Failed to construct the liquid-argon material G4_lAr.");
        }

        constexpr double kWorldMargin = 1.0 * m;
        G4Box* world = new G4Box("worldBox",
                                 wbox_ / 2 + kWorldMargin,
                                 hbox_ / 2 + kWorldMargin,
                                 lbox_ / 2 + kWorldMargin);
        G4LogicalVolume* world_logical = new G4LogicalVolume(world, world_material, "worldBox");

        world_physical_ = new G4PVPlacement(nullptr,
                                            G4ThreeVector(),
                                            world_logical,
                                            "worldBox.physical",
                                            nullptr,
                                            false,
                                            0);

        G4Box* box = new G4Box("lArBox", wbox_/2, hbox_/2, lbox_/2);
        G4LogicalVolume* box_logical = new G4LogicalVolume(box, box_material, "lArBox");

        active_physical_ = new G4PVPlacement(nullptr,
                                             G4ThreeVector(),
                                             box_logical,
                                             "lArBox.physical",
                                             world_logical,
                                             false,
                                             0);

        G4VisAttributes* world_vis = new G4VisAttributes();
        world_vis->SetVisibility(false);
        world_logical->SetVisAttributes(world_vis);

        G4VisAttributes* box_vis = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0));
        box_vis->SetForceSolid(true);
        box_logical->SetVisAttributes(box_vis);

        ConfigureOpticalMaterials();

        return world_physical_;
    }

    G4VPhysicalVolume* DetectorConstruction::ConstructGDMLGeometry(const G4String& gdml_file)
    {
        gdml_parser_ = std::make_unique<G4GDMLParser>();
        gdml_parser_->Read(gdml_file, false);

        world_physical_ = gdml_parser_->GetWorldVolume();
        if (world_physical_ == nullptr)
        {
            throw std::runtime_error("Failed to construct GDML geometry from " + std::string(gdml_file));
        }

        active_physical_ = FindPhysicalVolume(messenger_->GetActiveVolumeName());
        if (active_physical_ == nullptr)
        {
            active_physical_ = FindPhysicalVolume("volTPCActive");
        }
        if (active_physical_ == nullptr)
        {
            active_physical_ = FindPhysicalVolume("volLAr");
        }
        if (active_physical_ == nullptr)
        {
            active_physical_ = FindPhysicalVolume("volArCLight");
        }
        if (active_physical_ == nullptr)
        {
            active_physical_ = world_physical_;
        }

        ConfigureOpticalMaterials();

        return world_physical_;
    }

    G4VPhysicalVolume* DetectorConstruction::FindPhysicalVolume(const G4String& name) const
    {
        if (name.empty())
        {
            return nullptr;
        }

        auto* store = G4PhysicalVolumeStore::GetInstance();
        for (auto* volume : *store)
        {
            if (volume == nullptr)
            {
                continue;
            }
            if (volume->GetName() == name)
            {
                return volume;
            }
            auto* logical = volume->GetLogicalVolume();
            if (logical != nullptr && logical->GetName() == name)
            {
                return volume;
            }
        }

        return nullptr;
    }

    void DetectorConstruction::ConfigureOpticalMaterials() const
    {
        G4double photon_energy[] = {2.0 * eV, 2.7 * eV, 3.4 * eV, 9.69 * eV};
        G4double lar_rindex[] = {1.23, 1.23, 1.24, 1.45};
        G4double lar_abslength[] = {20.0 * m, 20.0 * m, 20.0 * m, 20.0 * m};
        G4double gas_rindex[] = {1.0003, 1.0003, 1.0003, 1.0003};
        G4double gas_abslength[] = {1000.0 * m, 1000.0 * m, 1000.0 * m, 1000.0 * m};
        G4double plastic_rindex[] = {1.58, 1.58, 1.58, 1.60};
        G4double plastic_abslength[] = {2.0 * m, 2.0 * m, 50.0 * cm, 1.0 * mm};
        G4double silicon_rindex[] = {3.5, 3.5, 4.0, 4.0};
        G4double silicon_abslength[] = {1.0 * um, 1.0 * um, 1.0 * um, 1.0 * um};

        auto set_table = [&](const char* material_name,
                             G4double* rindex,
                             G4double* abslength) {
            G4Material* material = G4Material::GetMaterial(material_name, false);
            if (material == nullptr)
            {
                return;
            }

            auto* mpt = new G4MaterialPropertiesTable();
            mpt->AddProperty("RINDEX", photon_energy, rindex, 4);
            mpt->AddProperty("ABSLENGTH", photon_energy, abslength, 4);
            material->SetMaterialPropertiesTable(mpt);
        };

        set_table("G4_lAr", lar_rindex, lar_abslength);
        set_table("LAr", lar_rindex, lar_abslength);
        set_table("GAr", gas_rindex, gas_abslength);
        set_table("Air", gas_rindex, gas_abslength);
        set_table("NoGas", gas_rindex, gas_abslength);
        set_table("Vac", gas_rindex, gas_abslength);
        set_table("Vacuum", gas_rindex, gas_abslength);
        set_table("Vacuum_cryo", gas_rindex, gas_abslength);
        set_table("FR4", plastic_rindex, plastic_abslength);
        set_table("G10", plastic_rindex, plastic_abslength);
        set_table("PVT", plastic_rindex, plastic_abslength);
        set_table("PSA", plastic_rindex, plastic_abslength);
        set_table("TPB", plastic_rindex, plastic_abslength);
        set_table("ESR", plastic_rindex, plastic_abslength);
        set_table("Silicon", silicon_rindex, silicon_abslength);

        auto* lar = G4Material::GetMaterial("LAr", false);
        if (lar == nullptr)
        {
            lar = G4Material::GetMaterial("G4_lAr", false);
        }
        if (lar != nullptr && lar->GetMaterialPropertiesTable() != nullptr)
        {
            G4double scint_energy[] = {7.0 * eV, 9.69 * eV, 10.5 * eV};
            G4double scint_spectrum[] = {0.05, 1.0, 0.05};
            auto* mpt = lar->GetMaterialPropertiesTable();
            mpt->AddProperty("SCINTILLATIONCOMPONENT1", scint_energy, scint_spectrum, 3);
            mpt->AddConstProperty("SCINTILLATIONYIELD", 2000.0 / MeV);
            mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
            mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", 6.0 * ns);
            mpt->AddConstProperty("SCINTILLATIONYIELD1", 1.0);
        }

        auto* wls = G4Material::GetMaterial("EJ280WLS", false);
        if (wls != nullptr)
        {
            auto* mpt = new G4MaterialPropertiesTable();
            G4double wls_rindex[] = {1.58, 1.58, 1.58, 1.60};
            G4double wls_abs[] = {2.0 * m, 2.0 * m, 50.0 * cm, 1.0 * mm};
            G4double wls_absorb[] = {10.0 * m, 20.0 * cm, 1.0 * cm, 0.1 * mm};
            G4double wls_component[] = {0.05, 1.0, 0.35, 0.0};
            mpt->AddProperty("RINDEX", photon_energy, wls_rindex, 4);
            mpt->AddProperty("ABSLENGTH", photon_energy, wls_abs, 4);
            mpt->AddProperty("WLSABSLENGTH", photon_energy, wls_absorb, 4);
            mpt->AddProperty("WLSCOMPONENT", photon_energy, wls_component, 4);
            mpt->AddConstProperty("WLSTIMECONSTANT", 12.0 * ns);
            wls->SetMaterialPropertiesTable(mpt);
        }
    }
}

#ifndef DETECTORCONSTRUCTION_HH
#define DETECTORCONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "G4UImessenger.hh"
#include "G4UserLimits.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWith3VectorAndUnit.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Material.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4Tokenizer.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include "Messenger.hh"

#include <memory>

class G4GDMLParser;

namespace G4LArBox
{
  class DetectorConstruction : public G4VUserDetectorConstruction
  {
    public:
        DetectorConstruction(Messenger* messenger);
        ~DetectorConstruction();

        G4VPhysicalVolume* Construct();
        G4VPhysicalVolume* GetWorldVolume() const { return world_physical_; }
        G4VPhysicalVolume* GetActiveVolume() const { return active_physical_; }

    private:
        Messenger* messenger_;
        G4VPhysicalVolume* world_physical_;
        G4VPhysicalVolume* active_physical_;
        std::unique_ptr<G4GDMLParser> gdml_parser_;

        double wbox_, hbox_, lbox_;

        G4VPhysicalVolume* ConstructBoxGeometry();
        G4VPhysicalVolume* ConstructGDMLGeometry(const G4String& gdml_file);
        G4VPhysicalVolume* FindPhysicalVolume(const G4String& name) const;
        void ConfigureOpticalMaterials() const;
  };
}

#endif //DETECTORCONSTRUCTION_HH

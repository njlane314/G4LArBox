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

namespace G4LArBox
{
  class DetectorConstruction : public G4VUserDetectorConstruction
  {
    public:
        DetectorConstruction(Messenger* messenger);
        ~DetectorConstruction();

        G4VPhysicalVolume* Construct();
        G4VPhysicalVolume* GetWorldVolume() const { return box_physical; }

    private:
        Messenger* messenger_;
        G4VPhysicalVolume* box_physical;

        double wbox_, hbox_, lbox_;
  };
}

#endif //DETECTORCONSTRUCTION_HH
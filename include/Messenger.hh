#ifndef MESSENGER_HH
#define MESSENGER_HH

#include <iomanip>
#include <iterator>

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

namespace G4LArBox 
{
    class Messenger : public G4UImessenger {
    public:
        Messenger();
        ~Messenger();

        void GetBoxDimensions(double& wbox, double& hbox, double& lbox_);

    private:
        G4UIdirectory* box_directory_;
        G4UIcommand* box_width_cmd_;
        G4UIcommand* box_height_cmd_;
        G4UIcommand* box_length_cmd_;

        void SetNewValue(G4UIcommand* cmd, G4String new_value);

        double box_width_;
        double box_height_;
        double box_length_;
    };
}

#endif // MESSENGER_HH
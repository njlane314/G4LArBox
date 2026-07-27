#ifndef MESSENGER_HH
#define MESSENGER_HH

#include "G4UImessenger.hh"
#include "globals.hh"

class G4UIcmdWithADoubleAndUnit;
class G4UIdirectory;
class G4UIcommand;

namespace G4LArBox
{
    class Messenger final : public G4UImessenger
    {
    public:
        Messenger();
        ~Messenger() override;

        void SetNewValue(G4UIcommand* command, G4String value) override;
        void GetBoxDimensions(G4double& width,
                              G4double& height,
                              G4double& length) const;

    private:
        G4UIdirectory* box_directory_;
        G4UIcmdWithADoubleAndUnit* width_command_;
        G4UIcmdWithADoubleAndUnit* height_command_;
        G4UIcmdWithADoubleAndUnit* length_command_;

        G4double width_;
        G4double height_;
        G4double length_;
    };
}

#endif

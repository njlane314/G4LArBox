#include "Messenger.hh"

#include "G4StateManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIdirectory.hh"

namespace G4LArBox
{
    namespace
    {
        G4UIcmdWithADoubleAndUnit* MakeDimensionCommand(
            const char* path,
            const char* guidance,
            G4UImessenger* messenger)
        {
            auto* command = new G4UIcmdWithADoubleAndUnit(path, messenger);
            command->SetGuidance(guidance);
            command->SetParameterName("size", false);
            command->SetDefaultUnit("m");
            command->SetRange("size>0.");
            command->AvailableForStates(G4State_PreInit);
            return command;
        }
    }

    Messenger::Messenger()
        : box_directory_(new G4UIdirectory("/box/")),
          width_command_(MakeDimensionCommand(
              "/box/width", "Set the liquid-argon box width.", this)),
          height_command_(MakeDimensionCommand(
              "/box/height", "Set the liquid-argon box height.", this)),
          length_command_(MakeDimensionCommand(
              "/box/length", "Set the liquid-argon box length.", this)),
          width_(1.0 * m),
          height_(1.0 * m),
          length_(1.0 * m)
    {
        box_directory_->SetGuidance("Liquid-argon box geometry.");
    }

    Messenger::~Messenger()
    {
        delete length_command_;
        delete height_command_;
        delete width_command_;
        delete box_directory_;
    }

    void Messenger::SetNewValue(G4UIcommand* command, G4String value)
    {
        if (command == width_command_)
        {
            width_ = width_command_->GetNewDoubleValue(value);
        }
        else if (command == height_command_)
        {
            height_ = height_command_->GetNewDoubleValue(value);
        }
        else if (command == length_command_)
        {
            length_ = length_command_->GetNewDoubleValue(value);
        }
    }

    void Messenger::GetBoxDimensions(G4double& width,
                                     G4double& height,
                                     G4double& length) const
    {
        width = width_;
        height = height_;
        length = length_;
    }
}

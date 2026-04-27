#include "Messenger.hh"

namespace G4LArBox 
{
    namespace
    {
        // ../ubcore/ubcore/Geometry/gdml/microboone/micro-tpc.gdml: TPCActive.
        constexpr double kMicroBooNETPCActiveWidth = 2.5635 * m;
        constexpr double kMicroBooNETPCActiveHeight = 2.33 * m;
        constexpr double kMicroBooNETPCActiveLength = 10.368 * m;
    }

    Messenger::Messenger() : G4UImessenger() 
    {
        box_width_ = kMicroBooNETPCActiveWidth;
        box_height_ = kMicroBooNETPCActiveHeight;
        box_length_ = kMicroBooNETPCActiveLength;
        gdml_file_ = "";
        active_volume_name_ = "lArBox.physical";

        box_directory_ = new G4UIdirectory("/box/");
        box_directory_->SetGuidance("Box geometry commands.");

        box_width_cmd_ = new G4UIcommand("/box/width", this);
        box_width_cmd_->SetGuidance("MicroBooNE TPCActive width is fixed to 2.5635 m.");
        box_width_cmd_->SetGuidance("This command is retained for backward compatibility and ignored.");

        box_height_cmd_ = new G4UIcommand("/box/height", this);
        box_height_cmd_->SetGuidance("MicroBooNE TPCActive height is fixed to 2.33 m.");
        box_height_cmd_->SetGuidance("This command is retained for backward compatibility and ignored.");

        box_length_cmd_ = new G4UIcommand("/box/length", this);
        box_length_cmd_->SetGuidance("MicroBooNE TPCActive length is fixed to 10.368 m.");
        box_length_cmd_->SetGuidance("This command is retained for backward compatibility and ignored.");

        G4UIparameter* box_width_parameter = new G4UIparameter("box_width", 'd', true);
        box_width_parameter->SetGuidance("Ignored. Geometry is fixed in code.");
        box_width_parameter->SetDefaultValue(2.5635);
        box_width_cmd_->SetParameter(box_width_parameter);

        G4UIparameter* box_height_parameter = new G4UIparameter("box_height", 'd', true);
        box_height_parameter->SetGuidance("Ignored. Geometry is fixed in code.");
        box_height_parameter->SetDefaultValue(2.33);
        box_height_cmd_->SetParameter(box_height_parameter);

        G4UIparameter* box_length_parameter = new G4UIparameter("box_length", 'd', true);
        box_length_parameter->SetGuidance("Ignored. Geometry is fixed in code.");
        box_length_parameter->SetDefaultValue(10.368);
        box_length_cmd_->SetParameter(box_length_parameter);

        gdml_file_cmd_ = new G4UIcmdWithAString("/box/gdmlFile", this);
        gdml_file_cmd_->SetGuidance("Optional GDML file to use instead of the built-in LAr box.");
        gdml_file_cmd_->SetParameterName("gdml_file", true);
        gdml_file_cmd_->SetDefaultValue("");

        active_volume_cmd_ = new G4UIcmdWithAString("/box/activeVolume", this);
        active_volume_cmd_->SetGuidance("Physical or logical volume used for random vertex sampling.");
        active_volume_cmd_->SetGuidance("For GDML geometries, use names such as volTPCActive, volLAr, or volArCLight.");
        active_volume_cmd_->SetParameterName("active_volume", true);
        active_volume_cmd_->SetDefaultValue("lArBox.physical");
    }
   
    Messenger::~Messenger() {}
   
    void Messenger::SetNewValue(G4UIcommand* cmd, G4String new_value) {
        if (cmd == gdml_file_cmd_)
        {
            gdml_file_ = new_value;
            return;
        }

        if (cmd == active_volume_cmd_)
        {
            active_volume_name_ = new_value;
            return;
        }

        if (cmd != box_width_cmd_ && cmd != box_height_cmd_ && cmd != box_length_cmd_)
        {
            std::cout << "-- Messenger::SetNewValue: Unknown command" << std::endl;
        }
    }

    void Messenger::GetBoxDimensions(double& wbox, double& hbox, double& lbox)
    {
        wbox = box_width_;
        hbox = box_height_;
        lbox = box_length_;
    }
}

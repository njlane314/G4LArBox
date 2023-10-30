#include "Messenger.hh"

namespace G4LArBox 
{
    Messenger::Messenger() : G4UImessenger() 
    {
        box_directory_ = new G4UIdirectory("/box/");
        box_directory_->SetGuidance("Box geometry construction control commands.");

        box_width_cmd_ = new G4UIcommand("/box/width", this);
        box_width_cmd_->SetGuidance("Set box width.");
        box_width_cmd_->SetGuidance("[usage] /box/width <box_width [m]>");

        box_height_cmd_ = new G4UIcommand("/box/height", this);
        box_height_cmd_->SetGuidance("Set box height.");
        box_height_cmd_->SetGuidance("[usage] /box/height <box_height [m]t>");

        box_length_cmd_ = new G4UIcommand("/box/length", this);
        box_length_cmd_->SetGuidance("Set box length.");
        box_length_cmd_->SetGuidance("[usage] /box/length <box_length [m]>");

        G4UIparameter* box_width_parameter = new G4UIparameter("box_width", 'd', true);
        box_width_parameter->SetGuidance("Set box width.");
        box_width_parameter->SetDefaultValue(1.0);
        box_width_cmd_->SetParameter(box_width_parameter);

        G4UIparameter* box_height_parameter = new G4UIparameter("box_height", 'd', true);
        box_height_parameter->SetGuidance("Set box height.");
        box_height_parameter->SetDefaultValue(1.0);
        box_height_cmd_->SetParameter(box_height_parameter);

        G4UIparameter* box_length_parameter = new G4UIparameter("box_length", 'd', true);
        box_length_parameter->SetGuidance("Set box length.");
        box_length_parameter->SetDefaultValue(1.0);
        box_length_cmd_->SetParameter(box_length_parameter);;
    }
   
    Messenger::~Messenger() {}
   
    void Messenger::SetNewValue(G4UIcommand* cmd, G4String new_value) {
        if (cmd == box_width_cmd_) 
        {
            box_width_ = std::stod(new_value)*m;  
        } 
        else if (cmd == box_height_cmd_) 
        {
            box_height_ = std::stod(new_value)*m;
        } 
        else if (cmd == box_length_cmd_) 
        {
            box_length_ = std::stod(new_value)*m;
        } 
        else 
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
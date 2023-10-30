#include "DetectorConstruction.hh"
#include "ActionInitialisation.hh"
#include "PhysicsList.hh"

#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4UIterminal.hh"
#include "G4UItcsh.hh"
#include "G4VisManager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "Randomize.hh"

using namespace G4LArBox;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc,char** argv)
{
    std::cout << "-- Starting program..." << std::endl;     

    std::string generator_config;

    G4UIExecutive* ui = nullptr;
    ui = new G4UIExecutive(argc, argv);

    double lbox_, hbox_, wbox_;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if ((arg == "-g" || arg == "--generator") && i + 1 < argc) 
        {
            generator_config = argv[++i];
        }
        else if (arg == "-w" || arg == "--width") 
        {
            if (i + 1 < argc) {
                wbox_ = std::stod(argv[++i])*m;
            } else {
                std::cout << "-- Failed to parse box width" << std::endl;
                return 1;
            }
        } 
        else if (arg == "-h" || arg == "--height") 
        {
            if (i + 1 < argc) {
                hbox_ = std::stod(argv[++i])*m;
            } else {
                std::cout << "-- Failed to parse box height" << std::endl;
                return 1;
            }
        } 
        else if (arg == "-l" || arg == "--length") 
        {
            if (i + 1 < argc) {
                lbox_ = std::stod(argv[++i])*m;
            } else {
                std::cout << "-- Failed to parse box length" << std::endl;
                    return 1;
            }
        } 
        else 
        {
            std::cout << "-- Failed to parse command line arguments" << std::endl;
        }
    }
    std::cout << "-- Parsing arguments done" << std::endl;

    G4RunManager* runManager = new G4RunManager();

    runManager->SetUserInitialization(new DetectorConstruction(wbox_, hbox_, lbox_));
    runManager->SetUserInitialization(new PhysicsList());
    runManager->SetUserInitialization(new ActionInitialisation());
    
    runManager->Initialize();

    std::ifstream generator_config_stream(generator_config);
    if (generator_config_stream.good()) {
        G4UImanager* ui_manager = G4UImanager::GetUIpointer();
        ui_manager->ApplyCommand("/control/execute " + generator_config);
        std::cout << "-- Macro complete!" << std::endl;
    }
    else {
        std::cout << "-- Failed to open generator macro..." << std::endl;
    }

    delete runManager;

    return 0;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo.....
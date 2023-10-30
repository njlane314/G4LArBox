#include "DetectorConstruction.hh"
#include "ActionInitialisation.hh"
#include "PhysicsList.hh"
#include "Messenger.hh"

#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4UIterminal.hh"
#include "G4UItcsh.hh"
#include "G4VisManager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"

#include "Randomize.hh"
#include <ctime>  
#include "CLHEP/Random/Random.h"

using namespace G4LArBox;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc,char** argv)
{
    std::cout << "-- Starting program..." << std::endl;
    
    CLHEP::HepRandom::setTheEngine(new CLHEP::RanecuEngine());
    G4long seed = time(0);
    CLHEP::HepRandom::setTheSeed(seed);

    std::string detector_config;
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
        else if ((arg == "-d" || arg == "--detector") && i + 1 < argc) 
        {
            detector_config = argv[++i];
        }
        else 
        {
            std::cout << "-- Failed to parse command line arguments" << std::endl;
        }
    }
    std::cout << "-- Parsing arguments done!" << std::endl;

    G4RunManager* runManager = new G4RunManager();

    runManager->SetUserInitialization(new DetectorConstruction(new Messenger()));

    std::ifstream detector_config_stream(detector_config);
    if (detector_config_stream.good()) {
        G4UImanager* ui_manager = G4UImanager::GetUIpointer();
        ui_manager->ApplyCommand("/control/execute " + detector_config);
        std::cout << "-- Detector macro complete!" << std::endl;
    }
    else {
        std::cout << "-- Failed to open detector macro..." << std::endl;
    }

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
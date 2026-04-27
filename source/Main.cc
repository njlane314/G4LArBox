#include "DetectorConstruction.hh"
#include "ActionInitialisation.hh"
#include "PhysicsList.hh"
#include "Messenger.hh"

#include "G4RunManager.hh"
#include "G4UImanager.hh"

#include "Randomize.hh"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include "CLHEP/Random/Random.h"

using namespace G4LArBox;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc,char** argv)
{
    std::cout << "-- Starting program..." << std::endl;

    CLHEP::HepRandom::setTheEngine(new CLHEP::RanecuEngine());
    G4long seed = time(0);
    if (const char* configured_seed = std::getenv("G4LARBOX_RANDOM_SEED"))
    {
        char* parse_end = nullptr;
        const long parsed_seed = std::strtol(configured_seed, &parse_end, 10);
        if (parse_end != configured_seed)
        {
            seed = parsed_seed;
        }
    }
    CLHEP::HepRandom::setTheSeed(seed);
    std::cout << "-- Random seed: " << seed << std::endl;

    std::string detector_config;
    std::string generator_config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            std::cout << "Usage: " << argv[0] << " -d <detector.mac> -g <generator.mac>" << std::endl;
            return 0;
        }
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
            std::cerr << "-- Failed to parse command line argument: " << arg << std::endl;
            std::cerr << "Usage: " << argv[0] << " -d <detector.mac> -g <generator.mac>" << std::endl;
            return 1;
        }
    }
    std::cout << "-- Parsing arguments done!" << std::endl;

    if (detector_config.empty() || generator_config.empty())
    {
        std::cerr << "-- Both detector and generator macro paths are required." << std::endl;
        std::cerr << "Usage: " << argv[0] << " -d <detector.mac> -g <generator.mac>" << std::endl;
        return 1;
    }

    G4RunManager* runManager = new G4RunManager();

    runManager->SetUserInitialization(new DetectorConstruction(new Messenger()));

    std::ifstream detector_config_stream(detector_config);
    if (detector_config_stream.good()) {
        G4UImanager* ui_manager = G4UImanager::GetUIpointer();
        ui_manager->ApplyCommand("/control/execute " + detector_config);
        std::cout << "-- Detector macro complete!" << std::endl;
    }
    else {
        std::cerr << "-- Failed to open detector macro: " << detector_config << std::endl;
        delete runManager;
        return 1;
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
        std::cerr << "-- Failed to open generator macro: " << generator_config << std::endl;
        delete runManager;
        return 1;
    }

    delete runManager;

    return 0;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo.....

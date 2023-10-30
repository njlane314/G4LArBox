#ifndef DATAHANDLER_HH
#define DATAHANDLER_HH

#include <vector>
#include <memory>
#include <string>

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ThreeVector.hh"
#include "G4Event.hh"
#include "G4PrimaryParticle.hh"

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

namespace G4LArBox 
{
    class DataHandler 
    {
    public:
        static DataHandler* Instance();
        ~DataHandler();

        DataHandler(const DataHandler&) = delete;
        DataHandler& operator=(const DataHandler&) = delete;

        void AddStep(const G4Step* step,  int nexc, int nion, int nopt, int ntherm);
        void AddTrack(const G4Track* track);
        void AddEntry();
        void WriteFile();
        void Reset();

    private:
        DataHandler(const char* filename = "data/output.root");

        static DataHandler* instance_;

        TFile* rootFile;
        TTree* stepTree;
        TTree* trackTree;
        TTree* eventTree;

        std::vector<double> edep_, len_, xs_, ys_, zs_, xe_, ye_, ze_, ta_;
        std::vector<int> parid_;

        std::vector<double> nexc_, nion_, nopt_, ntherm_;
        int tnexc_, tnion_, tnopt_, tntherm_;
        
        std::vector<double> xi_, yi_, zi_, ti_, pxi_, pyi_, pzi_, ekini_;
        std::vector<int> pdg_, curid_, preid_;
    };
}

#endif  // DATAHANDLER_HH
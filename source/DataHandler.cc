#include "DataHandler.hh"

namespace G4LArBox 
{
    DataHandler* DataHandler::instance_ = nullptr;

    DataHandler* DataHandler::Instance() 
    {
        if (instance_ == nullptr) 
        {
            instance_ = new DataHandler();
        }

        return instance_;
    }

    DataHandler::~DataHandler() 
    {
        if (rootFile) 
        {
            rootFile->Close();
            delete rootFile;
        }
    }

    DataHandler::DataHandler(const char* filename)
    {
        rootFile = new TFile(filename, "RECREATE");

        stepTree = new TTree("stepTree", "Tree of hits");
        stepTree->Branch("edep", &edep_);
        stepTree->Branch("len", &len_);
        stepTree->Branch("xs", &xs_);
        stepTree->Branch("ys", &ys_);
        stepTree->Branch("zs", &zs_);
        stepTree->Branch("xe", &xe_);
        stepTree->Branch("ye", &ye_);
        stepTree->Branch("ze", &ze_);
        stepTree->Branch("ta", &ta_);    
        stepTree->Branch("parid", &parid_);

        trackTree = new TTree("trackTree", "trackTree");
        trackTree->Branch("xi", &xi_);
        trackTree->Branch("yi", &yi_);
        trackTree->Branch("zi", &zi_);
        trackTree->Branch("ti", &ti_);
        trackTree->Branch("pxi", &pxi_);
        trackTree->Branch("pyi", &pyi_);
        trackTree->Branch("pzi", &pzi_);
        trackTree->Branch("ekini", &ekini_);
        trackTree->Branch("pdg", &pdg_);
        trackTree->Branch("curid", &curid_);
        trackTree->Branch("preid", &preid_);
    }

    void DataHandler::AddStep(const G4Step* step) 
    {
        edep_.push_back(step->GetTotalEnergyDeposit());
        len_.push_back(step->GetStepLength());
        ta_.push_back(step->GetPreStepPoint()->GetGlobalTime());

        G4ThreeVector startPos = step->GetPreStepPoint()->GetPosition();
        xs_.push_back(startPos.x());
        ys_.push_back(startPos.y());
        zs_.push_back(startPos.z());

        G4ThreeVector endPos = step->GetPostStepPoint()->GetPosition();
        xe_.push_back(endPos.x());
        ye_.push_back(endPos.y());
        ze_.push_back(endPos.z());

        parid_.push_back(step->GetTrack()->GetParentID());
    }

    void DataHandler::AddTrack(const G4Track* track) 
    {
        G4ThreeVector pos = track->GetPosition();
        xi_.push_back(pos.x());
        yi_.push_back(pos.y());
        zi_.push_back(pos.z());

        G4ThreeVector momentum = track->GetMomentum();
        pxi_.push_back(momentum.x());
        pyi_.push_back(momentum.y());
        pzi_.push_back(momentum.z());

        ti_.push_back(track->GetGlobalTime());
        ekini_.push_back(track->GetKineticEnergy());
        pdg_.push_back(track->GetDefinition()->GetPDGEncoding());
        curid_.push_back(track->GetTrackID());
        preid_.push_back(track->GetParentID());
    }

    void DataHandler::AddEntry() 
    {
        stepTree->Fill();
        trackTree->Fill();
    }

    void DataHandler::WriteFile() 
    {
        std::cout << "-- Writing to file..." << std::endl;
        rootFile->Write();
    }

    void DataHandler::Reset()
    {
        edep_.clear();
        len_.clear();
        xs_.clear();
        ys_.clear();
        zs_.clear();
        xe_.clear();
        ye_.clear();
        ze_.clear();
        ta_.clear();
        parid_.clear();

        xi_.clear();
        yi_.clear();
        zi_.clear();
        ti_.clear();
        pxi_.clear();
        pyi_.clear();
        pzi_.clear();
        ekini_.clear();
        pdg_.clear();
        curid_.clear();
        preid_.clear();
    }
}
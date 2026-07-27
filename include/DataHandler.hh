#ifndef DATAHANDLER_HH
#define DATAHANDLER_HH

#include "GeneratorTruth.hh"

#include <memory>
#include <vector>

class G4Step;
class G4Track;
class TFile;
class TTree;

namespace G4LArBox
{
    class DataHandler final
    {
    public:
        static DataHandler& Instance();

        DataHandler(const DataHandler&) = delete;
        DataHandler& operator=(const DataHandler&) = delete;
        ~DataHandler();

        void AddStep(const G4Step* step,
                     int excitons,
                     int ions,
                     int photons,
                     int electrons);
        void AddTrack(const G4Track* track);
        void SetGeneratorTruth(const GeneratorTruthRecord& truth);
        void AddEntry();
        void Reset();
        void WriteFile();

    private:
        DataHandler();

        std::unique_ptr<TFile> root_file_;
        TTree* step_tree_ = nullptr;
        TTree* track_tree_ = nullptr;
        TTree* event_tree_ = nullptr;
        TTree* truth_tree_ = nullptr;

        std::vector<double> energy_deposit_;
        std::vector<double> step_length_;
        std::vector<double> start_x_;
        std::vector<double> start_y_;
        std::vector<double> start_z_;
        std::vector<double> end_x_;
        std::vector<double> end_y_;
        std::vector<double> end_z_;
        std::vector<double> step_time_;
        std::vector<int> parent_id_;
        std::vector<int> track_id_;
        std::vector<int> step_pdg_;
        std::vector<int> excitons_;
        std::vector<int> ions_;
        std::vector<int> photons_;
        std::vector<int> electrons_;

        int total_excitons_ = 0;
        int total_ions_ = 0;
        int total_photons_ = 0;
        int total_electrons_ = 0;

        std::vector<double> vertex_x_;
        std::vector<double> vertex_y_;
        std::vector<double> vertex_z_;
        std::vector<double> vertex_t_;
        std::vector<double> final_x_;
        std::vector<double> final_y_;
        std::vector<double> final_z_;
        std::vector<double> final_t_;
        std::vector<double> momentum_x_;
        std::vector<double> momentum_y_;
        std::vector<double> momentum_z_;
        std::vector<double> kinetic_energy_;
        std::vector<int> track_pdg_;
        std::vector<int> current_track_id_;
        std::vector<int> parent_track_id_;

        GeneratorTruthRecord generator_truth_;
    };
}

#endif

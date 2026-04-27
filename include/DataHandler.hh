#ifndef DATAHANDLER_HH
#define DATAHANDLER_HH

#include <vector>
#include <memory>
#include <string>

#include "ElectronicsResponse.hh"
#include "GeneratorTruth.hh"
#include "OpticalResponse.hh"

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

        void AddStep(const G4Step* step, int nexc, int nion, int nopt, int ntherm);
        void AddOpticalHit(const G4Step* step);
        void AddTrack(const G4Track* track);
        void AddEntry();
        void SetGeneratorTruth(const GeneratorTruthRecord& truth);
        void WriteFile();
        void Reset();

    private:
        DataHandler(const char* filename = "data/output.root");
        void AddFastOpticalHits(const G4Step* step, int nopt);
        void BuildElectronicsResponse();

        static DataHandler* instance_;

        TFile* rootFile;
        TTree* stepTree;
        TTree* trackTree;
        TTree* eventTree;
        TTree* truthTree;

        std::vector<double> edep_, len_, xs_, ys_, zs_, xe_, ye_, ze_, ta_;
        std::vector<int> parid_, trkid_, steppdg_;

        std::vector<double> nexc_, nion_, nopt_, ntherm_;
        int tnexc_, tnion_, tnopt_, tntherm_;

        int optical_hits_;
        std::vector<double> optical_x_, optical_y_, optical_z_, optical_t_;
        std::vector<double> optical_energy_, optical_wavelength_nm_;
        std::vector<int> optical_track_id_, optical_parent_id_, optical_copy_number_;
        std::vector<std::string> optical_volume_;

        OpticalResponse optical_response_;
        bool fast_optical_enabled_;
        std::string fast_optical_model_;
        bool photon_library_loaded_;
        int fast_optical_hits_;

        ElectronicsResponse electronics_response_;
        int electronics_waveforms_;
        double electronics_sample_frequency_mhz_;
        double electronics_time_begin_us_;
        double electronics_time_end_us_;
        std::vector<int> electronics_channel_, electronics_sample_offset_, electronics_sample_count_;
        std::vector<std::string> electronics_channel_readout_;
        std::vector<short> electronics_adc_;

        std::vector<double> xi_, yi_, zi_, ti_, pxi_, pyi_, pzi_, ekini_;
        std::vector<double> xv_, yv_, zv_, tv_, xf_, yf_, zf_, tf_;
        std::vector<int> pdg_, curid_, preid_;

        std::string generator_source_;
        double generator_vertex_x_;
        double generator_vertex_y_;
        double generator_vertex_z_;
        double generator_vertex_t_;
        bool generator_has_incident_direction_;
        double generator_incident_dir_x_;
        double generator_incident_dir_y_;
        double generator_incident_dir_z_;

        int genie_iev_;
        int genie_neu_;
        int genie_tgt_;
        int genie_target_z_;
        int genie_target_a_;
        bool genie_cc_;
        bool genie_nc_;
        bool genie_qel_;
        bool genie_res_;
        bool genie_dis_;
        bool genie_coh_;
        bool genie_nuel_;
        bool genie_imd_;
        bool genie_em_;
        double genie_weight_;
        double genie_xs_;
        double genie_ev_;
        double genie_input_vtxx_;
        double genie_input_vtxy_;
        double genie_input_vtxz_;
        double genie_input_vtxt_;
        int marley_event_;
        double marley_flux_averaged_xsec_;
        int marley_projectile_pdg_;
        int marley_target_pdg_;
        int marley_ejectile_pdg_;
        int marley_residue_pdg_;
        double marley_ex_;
        std::string bxdecay0_category_;
        std::string bxdecay0_nuclide_;
        int bxdecay0_seed_;
        int bxdecay0_event_;
        int bxdecay0_particles_;
        double bxdecay0_event_time_ns_;
        std::vector<int> bxdecay0_pdg_;
        std::vector<double> bxdecay0_px_mev_, bxdecay0_py_mev_, bxdecay0_pz_mev_;
        std::vector<double> bxdecay0_time_ns_;
        bool radiological_enabled_;
        double radiological_mass_kg_;
        double radiological_window_us_;
        double radiological_expected_decays_;
        int radiological_decays_;
        std::vector<std::string> radiological_isotope_;
        std::vector<int> radiological_z_, radiological_a_;
        std::vector<double> radiological_activity_bq_per_kg_, radiological_decay_time_ns_;
        bool rock_neutron_enabled_;
        double rock_neutron_window_us_;
        double rock_neutron_expected_;
        int rock_neutron_count_;
        std::vector<double> rock_neutron_x_, rock_neutron_y_, rock_neutron_z_;
        std::vector<double> rock_neutron_time_ns_, rock_neutron_energy_mev_;
        std::vector<double> rock_neutron_dir_x_, rock_neutron_dir_y_, rock_neutron_dir_z_;
        std::vector<int> rock_neutron_face_;

        std::vector<int> generator_primary_pdg_;
        std::vector<double> generator_primary_energy_;
        std::vector<double> generator_primary_px_;
        std::vector<double> generator_primary_py_;
        std::vector<double> generator_primary_pz_;
    };
}

#endif  // DATAHANDLER_HH

#include "DataHandler.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "TFile.h"
#include "TObject.h"
#include "TTree.h"

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace G4LArBox
{
    DataHandler& DataHandler::Instance()
    {
        static DataHandler instance;
        return instance;
    }

    DataHandler::DataHandler()
    {
        const char* configured_path = std::getenv("G4LARBOX_OUTPUT_FILE");
        const std::filesystem::path output_path =
            configured_path == nullptr ? "data/output.root" : configured_path;
        if (output_path.has_parent_path())
        {
            std::filesystem::create_directories(output_path.parent_path());
        }

        root_file_.reset(TFile::Open(output_path.string().c_str(), "RECREATE"));
        if (root_file_ == nullptr || root_file_->IsZombie())
        {
            throw std::runtime_error("Could not create ROOT output file: " +
                                     output_path.string());
        }

        step_tree_ = new TTree("stepTree", "Geant4 steps");
        step_tree_->Branch("edep", &energy_deposit_);
        step_tree_->Branch("len", &step_length_);
        step_tree_->Branch("xs", &start_x_);
        step_tree_->Branch("ys", &start_y_);
        step_tree_->Branch("zs", &start_z_);
        step_tree_->Branch("xe", &end_x_);
        step_tree_->Branch("ye", &end_y_);
        step_tree_->Branch("ze", &end_z_);
        step_tree_->Branch("ta", &step_time_);
        step_tree_->Branch("parid", &parent_id_);
        step_tree_->Branch("trkid", &track_id_);
        step_tree_->Branch("step_pdg", &step_pdg_);
        step_tree_->Branch("nexc", &excitons_);
        step_tree_->Branch("nion", &ions_);
        step_tree_->Branch("nopt", &photons_);
        step_tree_->Branch("ntherm", &electrons_);

        track_tree_ = new TTree("trackTree", "Geant4 tracks");
        track_tree_->Branch("xv", &vertex_x_);
        track_tree_->Branch("yv", &vertex_y_);
        track_tree_->Branch("zv", &vertex_z_);
        track_tree_->Branch("tv", &vertex_t_);
        track_tree_->Branch("xf", &final_x_);
        track_tree_->Branch("yf", &final_y_);
        track_tree_->Branch("zf", &final_z_);
        track_tree_->Branch("tf", &final_t_);
        track_tree_->Branch("pxi", &momentum_x_);
        track_tree_->Branch("pyi", &momentum_y_);
        track_tree_->Branch("pzi", &momentum_z_);
        track_tree_->Branch("ekini", &kinetic_energy_);
        track_tree_->Branch("pdg", &track_pdg_);
        track_tree_->Branch("curid", &current_track_id_);
        track_tree_->Branch("preid", &parent_track_id_);

        event_tree_ = new TTree("eventTree", "Liquid-argon response by event");
        event_tree_->Branch("tnexc", &total_excitons_);
        event_tree_->Branch("tnion", &total_ions_);
        event_tree_->Branch("tnopt", &total_photons_);
        event_tree_->Branch("tntherm", &total_electrons_);

        truth_tree_ = new TTree("truthTree", "Generator truth by event");
        truth_tree_->Branch("source", &generator_truth_.source);
        truth_tree_->Branch("vertex_x", &generator_truth_.vertex_x);
        truth_tree_->Branch("vertex_y", &generator_truth_.vertex_y);
        truth_tree_->Branch("vertex_z", &generator_truth_.vertex_z);
        truth_tree_->Branch("vertex_t", &generator_truth_.vertex_t);
        truth_tree_->Branch("has_incident_direction", &generator_truth_.has_incident_direction);
        truth_tree_->Branch("incident_dir_x", &generator_truth_.incident_dir_x);
        truth_tree_->Branch("incident_dir_y", &generator_truth_.incident_dir_y);
        truth_tree_->Branch("incident_dir_z", &generator_truth_.incident_dir_z);

        truth_tree_->Branch("genie_iev", &generator_truth_.genie_iev);
        truth_tree_->Branch("genie_neu", &generator_truth_.genie_neu);
        truth_tree_->Branch("genie_tgt", &generator_truth_.genie_tgt);
        truth_tree_->Branch("genie_target_z", &generator_truth_.genie_target_z);
        truth_tree_->Branch("genie_target_a", &generator_truth_.genie_target_a);
        truth_tree_->Branch("genie_cc", &generator_truth_.genie_cc);
        truth_tree_->Branch("genie_nc", &generator_truth_.genie_nc);
        truth_tree_->Branch("genie_qel", &generator_truth_.genie_qel);
        truth_tree_->Branch("genie_res", &generator_truth_.genie_res);
        truth_tree_->Branch("genie_dis", &generator_truth_.genie_dis);
        truth_tree_->Branch("genie_coh", &generator_truth_.genie_coh);
        truth_tree_->Branch("genie_nuel", &generator_truth_.genie_nuel);
        truth_tree_->Branch("genie_imd", &generator_truth_.genie_imd);
        truth_tree_->Branch("genie_em", &generator_truth_.genie_em);
        truth_tree_->Branch("genie_weight", &generator_truth_.genie_weight);
        truth_tree_->Branch("genie_xs", &generator_truth_.genie_xs);
        truth_tree_->Branch("genie_ev", &generator_truth_.genie_ev);
        truth_tree_->Branch("genie_input_vtxx", &generator_truth_.genie_input_vtxx);
        truth_tree_->Branch("genie_input_vtxy", &generator_truth_.genie_input_vtxy);
        truth_tree_->Branch("genie_input_vtxz", &generator_truth_.genie_input_vtxz);
        truth_tree_->Branch("genie_input_vtxt", &generator_truth_.genie_input_vtxt);

        truth_tree_->Branch("marley_event", &generator_truth_.marley_event);
        truth_tree_->Branch("marley_flux_averaged_xsec",
                            &generator_truth_.marley_flux_averaged_xsec);
        truth_tree_->Branch("marley_projectile_pdg", &generator_truth_.marley_projectile_pdg);
        truth_tree_->Branch("marley_target_pdg", &generator_truth_.marley_target_pdg);
        truth_tree_->Branch("marley_ejectile_pdg", &generator_truth_.marley_ejectile_pdg);
        truth_tree_->Branch("marley_residue_pdg", &generator_truth_.marley_residue_pdg);
        truth_tree_->Branch("marley_ex", &generator_truth_.marley_ex);

        truth_tree_->Branch("bxdecay0_category", &generator_truth_.bxdecay0_category);
        truth_tree_->Branch("bxdecay0_nuclide", &generator_truth_.bxdecay0_nuclide);
        truth_tree_->Branch("bxdecay0_seed", &generator_truth_.bxdecay0_seed);
        truth_tree_->Branch("bxdecay0_event", &generator_truth_.bxdecay0_event);
        truth_tree_->Branch("bxdecay0_particles", &generator_truth_.bxdecay0_particles);
        truth_tree_->Branch("bxdecay0_event_time_ns", &generator_truth_.bxdecay0_event_time_ns);
        truth_tree_->Branch("bxdecay0_pdg", &generator_truth_.bxdecay0_pdg);
        truth_tree_->Branch("bxdecay0_px_mev", &generator_truth_.bxdecay0_px_mev);
        truth_tree_->Branch("bxdecay0_py_mev", &generator_truth_.bxdecay0_py_mev);
        truth_tree_->Branch("bxdecay0_pz_mev", &generator_truth_.bxdecay0_pz_mev);
        truth_tree_->Branch("bxdecay0_time_ns", &generator_truth_.bxdecay0_time_ns);

        truth_tree_->Branch("radiological_enabled", &generator_truth_.radiological_enabled);
        truth_tree_->Branch("radiological_mass_kg", &generator_truth_.radiological_mass_kg);
        truth_tree_->Branch("radiological_window_us", &generator_truth_.radiological_window_us);
        truth_tree_->Branch("radiological_expected_decays",
                            &generator_truth_.radiological_expected_decays);
        truth_tree_->Branch("radiological_decays", &generator_truth_.radiological_decays);
        truth_tree_->Branch("radiological_isotope", &generator_truth_.radiological_isotope);
        truth_tree_->Branch("radiological_z", &generator_truth_.radiological_z);
        truth_tree_->Branch("radiological_a", &generator_truth_.radiological_a);
        truth_tree_->Branch("radiological_activity_bq_per_kg",
                            &generator_truth_.radiological_activity_bq_per_kg);
        truth_tree_->Branch("radiological_decay_time_ns",
                            &generator_truth_.radiological_decay_time_ns);

        truth_tree_->Branch("rock_neutron_enabled", &generator_truth_.rock_neutron_enabled);
        truth_tree_->Branch("rock_neutron_window_us", &generator_truth_.rock_neutron_window_us);
        truth_tree_->Branch("rock_neutron_expected", &generator_truth_.rock_neutron_expected);
        truth_tree_->Branch("rock_neutron_count", &generator_truth_.rock_neutron_count);
        truth_tree_->Branch("rock_neutron_x", &generator_truth_.rock_neutron_x);
        truth_tree_->Branch("rock_neutron_y", &generator_truth_.rock_neutron_y);
        truth_tree_->Branch("rock_neutron_z", &generator_truth_.rock_neutron_z);
        truth_tree_->Branch("rock_neutron_time_ns", &generator_truth_.rock_neutron_time_ns);
        truth_tree_->Branch("rock_neutron_energy_mev", &generator_truth_.rock_neutron_energy_mev);
        truth_tree_->Branch("rock_neutron_dir_x", &generator_truth_.rock_neutron_dir_x);
        truth_tree_->Branch("rock_neutron_dir_y", &generator_truth_.rock_neutron_dir_y);
        truth_tree_->Branch("rock_neutron_dir_z", &generator_truth_.rock_neutron_dir_z);
        truth_tree_->Branch("rock_neutron_face", &generator_truth_.rock_neutron_face);

        truth_tree_->Branch("primary_pdg", &generator_truth_.primary_pdg);
        truth_tree_->Branch("primary_energy", &generator_truth_.primary_energy);
        truth_tree_->Branch("primary_px", &generator_truth_.primary_px);
        truth_tree_->Branch("primary_py", &generator_truth_.primary_py);
        truth_tree_->Branch("primary_pz", &generator_truth_.primary_pz);

        Reset();
    }

    DataHandler::~DataHandler()
    {
        if (root_file_ != nullptr && root_file_->IsOpen())
        {
            root_file_->Close();
        }
    }

    void DataHandler::AddStep(const G4Step* step,
                              int excitons,
                              int ions,
                              int photons,
                              int electrons)
    {
        const auto* track = step->GetTrack();
        const auto* start = step->GetPreStepPoint();
        const auto* end = step->GetPostStepPoint();

        energy_deposit_.push_back(step->GetTotalEnergyDeposit());
        step_length_.push_back(step->GetStepLength());
        start_x_.push_back(start->GetPosition().x());
        start_y_.push_back(start->GetPosition().y());
        start_z_.push_back(start->GetPosition().z());
        end_x_.push_back(end->GetPosition().x());
        end_y_.push_back(end->GetPosition().y());
        end_z_.push_back(end->GetPosition().z());
        step_time_.push_back(start->GetGlobalTime());
        parent_id_.push_back(track->GetParentID());
        track_id_.push_back(track->GetTrackID());
        step_pdg_.push_back(track->GetDefinition()->GetPDGEncoding());
        excitons_.push_back(excitons);
        ions_.push_back(ions);
        photons_.push_back(photons);
        electrons_.push_back(electrons);

        total_excitons_ += excitons;
        total_ions_ += ions;
        total_photons_ += photons;
        total_electrons_ += electrons;
    }

    void DataHandler::AddTrack(const G4Track* track)
    {
        const auto& vertex = track->GetVertexPosition();
        const auto& final = track->GetPosition();
        const auto& momentum = track->GetMomentum();

        vertex_x_.push_back(vertex.x());
        vertex_y_.push_back(vertex.y());
        vertex_z_.push_back(vertex.z());
        vertex_t_.push_back(track->GetGlobalTime() - track->GetLocalTime());
        final_x_.push_back(final.x());
        final_y_.push_back(final.y());
        final_z_.push_back(final.z());
        final_t_.push_back(track->GetGlobalTime());
        momentum_x_.push_back(momentum.x());
        momentum_y_.push_back(momentum.y());
        momentum_z_.push_back(momentum.z());
        kinetic_energy_.push_back(track->GetKineticEnergy());
        track_pdg_.push_back(track->GetDefinition()->GetPDGEncoding());
        current_track_id_.push_back(track->GetTrackID());
        parent_track_id_.push_back(track->GetParentID());
    }

    void DataHandler::SetGeneratorTruth(const GeneratorTruthRecord& truth)
    {
        generator_truth_ = truth;
    }

    void DataHandler::AddEntry()
    {
        step_tree_->Fill();
        track_tree_->Fill();
        event_tree_->Fill();
        truth_tree_->Fill();
    }

    void DataHandler::WriteFile()
    {
        root_file_->cd();
        root_file_->Write("", TObject::kOverwrite);
    }

    void DataHandler::Reset()
    {
        energy_deposit_.clear();
        step_length_.clear();
        start_x_.clear();
        start_y_.clear();
        start_z_.clear();
        end_x_.clear();
        end_y_.clear();
        end_z_.clear();
        step_time_.clear();
        parent_id_.clear();
        track_id_.clear();
        step_pdg_.clear();
        excitons_.clear();
        ions_.clear();
        photons_.clear();
        electrons_.clear();

        total_excitons_ = 0;
        total_ions_ = 0;
        total_photons_ = 0;
        total_electrons_ = 0;

        vertex_x_.clear();
        vertex_y_.clear();
        vertex_z_.clear();
        vertex_t_.clear();
        final_x_.clear();
        final_y_.clear();
        final_z_.clear();
        final_t_.clear();
        momentum_x_.clear();
        momentum_y_.clear();
        momentum_z_.clear();
        kinetic_energy_.clear();
        track_pdg_.clear();
        current_track_id_.clear();
        parent_track_id_.clear();

        generator_truth_ = GeneratorTruthRecord{};
    }
}

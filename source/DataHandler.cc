#include "DataHandler.hh"

#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"

#include <cstdlib>
#include <string>

namespace G4LArBox 
{
    namespace
    {
        bool ReadBoolEnv(const char* name, bool fallback)
        {
            const char* value = std::getenv(name);
            if (value == nullptr)
            {
                return fallback;
            }

            const std::string text(value);
            return text == "1" || text == "true" || text == "TRUE" ||
                   text == "on" || text == "ON" || text == "yes" || text == "YES";
        }

        double ReadDoubleEnv(const char* name, double fallback)
        {
            const char* value = std::getenv(name);
            if (value == nullptr)
            {
                return fallback;
            }

            char* end = nullptr;
            const double parsed = std::strtod(value, &end);
            return end != value ? parsed : fallback;
        }

        unsigned int ReadUIntEnv(const char* name, unsigned int fallback)
        {
            const char* value = std::getenv(name);
            if (value == nullptr)
            {
                return fallback;
            }

            char* end = nullptr;
            const unsigned long parsed = std::strtoul(value, &end, 10);
            return end != value ? static_cast<unsigned int>(parsed) : fallback;
        }

        int ReadIntEnv(const char* name, int fallback)
        {
            const char* value = std::getenv(name);
            if (value == nullptr)
            {
                return fallback;
            }

            char* end = nullptr;
            const long parsed = std::strtol(value, &end, 10);
            return end != value ? static_cast<int>(parsed) : fallback;
        }

        std::string ReadStringEnv(const char* name, const std::string& fallback)
        {
            const char* value = std::getenv(name);
            return value != nullptr ? std::string(value) : fallback;
        }

        OpticalResponse::Model ParseOpticalModel(const std::string& text)
        {
            if (text == "node" || text == "fastdpsu" || text == "dunevd_node")
            {
                return OpticalResponse::Model::DUNEVDNode;
            }
            if (text == "arapuca" || text == "library" || text == "photon_library" ||
                text == "dunevd_arapuca")
            {
                return OpticalResponse::Model::DUNEVDPhotonLibrary;
            }
            if (text == "dunevd" || text == "hybrid" || text == "dunevd_hybrid")
            {
                return OpticalResponse::Model::DUNEVDHybrid;
            }
            return OpticalResponse::Model::LegacyPanel;
        }

        OpticalResponse::Config MakeOpticalConfigFromEnv()
        {
            OpticalResponse::Config config;
            config.model = ParseOpticalModel(
                ReadStringEnv("G4LARBOX_FAST_OPTICAL_MODEL",
                              ReadStringEnv("G4LARBOX_OPTICAL_MODEL", "legacy")));
            config.collection_efficiency =
                ReadDoubleEnv("G4LARBOX_FAST_OPTICAL_COLLECTION_EFF", config.collection_efficiency);
            config.diffuse_scale =
                ReadDoubleEnv("G4LARBOX_FAST_OPTICAL_DIFFUSE_SCALE", config.diffuse_scale);
            config.absorption_length_mm =
                ReadDoubleEnv("G4LARBOX_FAST_OPTICAL_ABS_LENGTH_MM", config.absorption_length_mm);
            config.rayleigh_length_mm =
                ReadDoubleEnv("G4LARBOX_FAST_OPTICAL_RAYLEIGH_LENGTH_MM", config.rayleigh_length_mm);
            config.seed =
                ReadUIntEnv("G4LARBOX_FAST_OPTICAL_SEED", config.seed);

            config.photon_library.file_path =
                ReadStringEnv("G4LARBOX_PHOTON_LIBRARY_FILE", config.photon_library.file_path);
            config.photon_library.nx =
                ReadIntEnv("G4LARBOX_PHOTON_LIBRARY_NX", config.photon_library.nx);
            config.photon_library.ny =
                ReadIntEnv("G4LARBOX_PHOTON_LIBRARY_NY", config.photon_library.ny);
            config.photon_library.nz =
                ReadIntEnv("G4LARBOX_PHOTON_LIBRARY_NZ", config.photon_library.nz);
            config.photon_library.x_min_cm =
                ReadDoubleEnv("G4LARBOX_PHOTON_LIBRARY_XMIN_CM", config.photon_library.x_min_cm);
            config.photon_library.x_max_cm =
                ReadDoubleEnv("G4LARBOX_PHOTON_LIBRARY_XMAX_CM", config.photon_library.x_max_cm);
            config.photon_library.y_min_cm =
                ReadDoubleEnv("G4LARBOX_PHOTON_LIBRARY_YMIN_CM", config.photon_library.y_min_cm);
            config.photon_library.y_max_cm =
                ReadDoubleEnv("G4LARBOX_PHOTON_LIBRARY_YMAX_CM", config.photon_library.y_max_cm);
            config.photon_library.z_min_cm =
                ReadDoubleEnv("G4LARBOX_PHOTON_LIBRARY_ZMIN_CM", config.photon_library.z_min_cm);
            config.photon_library.z_max_cm =
                ReadDoubleEnv("G4LARBOX_PHOTON_LIBRARY_ZMAX_CM", config.photon_library.z_max_cm);
            config.photon_library.interpolate =
                ReadBoolEnv("G4LARBOX_PHOTON_LIBRARY_INTERPOLATE", config.photon_library.interpolate);
            config.photon_library.channel_count =
                ReadIntEnv("G4LARBOX_PHOTON_LIBRARY_CHANNELS", config.photon_library.channel_count);
            config.photon_library_efficiency =
                ReadDoubleEnv("G4LARBOX_ARAPUCA_PDE", config.photon_library_efficiency);

            config.dunevd_node_strings_y =
                ReadIntEnv("G4LARBOX_NODE_STRINGS_Y", config.dunevd_node_strings_y);
            config.dunevd_node_strings_z =
                ReadIntEnv("G4LARBOX_NODE_STRINGS_Z", config.dunevd_node_strings_z);
            config.dunevd_node_count_x =
                ReadIntEnv("G4LARBOX_NODE_COUNT_X", config.dunevd_node_count_x);
            config.dunevd_node_channel_offset =
                ReadIntEnv("G4LARBOX_NODE_CHANNEL_OFFSET", config.dunevd_node_channel_offset);
            config.dunevd_node_pitch_mm =
                ReadDoubleEnv("G4LARBOX_NODE_PITCH_MM", config.dunevd_node_pitch_mm);
            config.dunevd_node_central_gap_mm =
                ReadDoubleEnv("G4LARBOX_NODE_CENTRAL_GAP_MM", config.dunevd_node_central_gap_mm);
            config.dunevd_node_effective_area_mm2 =
                ReadDoubleEnv("G4LARBOX_NODE_EFFECTIVE_AREA_MM2",
                              config.dunevd_node_effective_area_mm2);
            config.dunevd_node_x_offset_mm =
                ReadDoubleEnv("G4LARBOX_NODE_X_OFFSET_MM", config.dunevd_node_x_offset_mm);
            config.dunevd_node_y_offset_mm =
                ReadDoubleEnv("G4LARBOX_NODE_Y_OFFSET_MM", config.dunevd_node_y_offset_mm);
            config.dunevd_node_z_offset_mm =
                ReadDoubleEnv("G4LARBOX_NODE_Z_OFFSET_MM", config.dunevd_node_z_offset_mm);
            return config;
        }

        std::string OpticalModelName(OpticalResponse::Model model)
        {
            switch (model)
            {
                case OpticalResponse::Model::DUNEVDNode:
                    return "dunevd_node";
                case OpticalResponse::Model::DUNEVDPhotonLibrary:
                    return "dunevd_arapuca_photon_library";
                case OpticalResponse::Model::DUNEVDHybrid:
                    return "dunevd_hybrid";
                case OpticalResponse::Model::LegacyPanel:
                default:
                    return "legacy_panel";
            }
        }

        ElectronicsResponse::Config MakeElectronicsConfigFromEnv()
        {
            ElectronicsResponse::Config config;
            config.sample_frequency_mhz =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_SAMPLE_FREQUENCY_MHZ",
                              config.sample_frequency_mhz);
            config.time_begin_us =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_TIME_BEGIN_US", config.time_begin_us);
            config.time_end_us =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_TIME_END_US", config.time_end_us);
            config.quantum_efficiency =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_QUANTUM_EFFICIENCY",
                              config.quantum_efficiency);
            config.dark_rate_hz =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_DARK_RATE_HZ", config.dark_rate_hz);
            config.gain_mean_adc =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_GAIN_MEAN_ADC", config.gain_mean_adc);
            config.gain_spread =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_GAIN_SPREAD", config.gain_spread);
            config.adc_baseline =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_ADC_BASELINE", config.adc_baseline);
            config.adc_baseline_spread =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_ADC_BASELINE_SPREAD",
                              config.adc_baseline_spread);
            config.adc_sample_noise_sigma =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_ADC_SAMPLE_NOISE_SIGMA",
                              config.adc_sample_noise_sigma);
            config.saturation_adc =
                ReadIntEnv("G4LARBOX_ELECTRONICS_SATURATION_ADC", config.saturation_adc);
            config.pedestal_fluctuation_rate_hz =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_PEDESTAL_FLUCTUATION_RATE_HZ",
                              config.pedestal_fluctuation_rate_hz);
            config.pedestal_fluctuation_amplitude_adc =
                ReadIntEnv("G4LARBOX_ELECTRONICS_PEDESTAL_FLUCTUATION_AMPLITUDE_ADC",
                           config.pedestal_fluctuation_amplitude_adc);
            config.store_noise_only_channels =
                ReadBoolEnv("G4LARBOX_ELECTRONICS_STORE_NOISE_ONLY_CHANNELS",
                            config.store_noise_only_channels);
            config.waveform_length_us =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_WAVEFORM_LENGTH_US",
                              config.waveform_length_us);
            config.waveform_power_factor =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_WAVEFORM_POWER_FACTOR",
                              config.waveform_power_factor);
            config.waveform_time_constant_us =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_WAVEFORM_TIME_CONSTANT_US",
                              config.waveform_time_constant_us);
            config.voltage_amplitude_for_spe =
                ReadDoubleEnv("G4LARBOX_ELECTRONICS_VOLTAGE_AMPLITUDE_FOR_SPE",
                              config.voltage_amplitude_for_spe);
            config.waveform_charge_normalized =
                ReadBoolEnv("G4LARBOX_ELECTRONICS_WAVEFORM_CHARGE_NORMALIZED",
                            config.waveform_charge_normalized);
            config.channel_count =
                ReadIntEnv("G4LARBOX_ELECTRONICS_CHANNEL_COUNT", config.channel_count);
            config.seed =
                ReadUIntEnv("G4LARBOX_ELECTRONICS_SEED", config.seed);
            return config;
        }

        bool StartsWith(const std::string& text, const std::string& prefix)
        {
            return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
        }

        int ResolveOpticalCopyNumber(const G4Step* step, const std::string& volume_name)
        {
            auto touchable = step->GetPostStepPoint()->GetTouchableHandle();
            int copy_number = touchable->GetCopyNumber();
            if (!StartsWith(volume_name, "volOpDetSensitive_") || copy_number > 0)
            {
                return copy_number;
            }

            const int depth = touchable->GetHistoryDepth();
            for (int i = 1; i <= depth; ++i)
            {
                const auto* volume = touchable->GetVolume(i);
                if (volume == nullptr || volume->GetLogicalVolume() == nullptr)
                {
                    continue;
                }

                const G4String logical_name = volume->GetLogicalVolume()->GetName();
                const int ancestor_copy_number = touchable->GetCopyNumber(i);
                if (logical_name.find("FastDPSU") != std::string::npos && ancestor_copy_number > 0)
                {
                    return ancestor_copy_number;
                }
            }

            return copy_number;
        }
    }

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
        : optical_response_(MakeOpticalConfigFromEnv()),
          fast_optical_enabled_(ReadBoolEnv("G4LARBOX_FAST_OPTICAL", false)),
          fast_optical_model_(OpticalModelName(MakeOpticalConfigFromEnv().model)),
          photon_library_loaded_(optical_response_.PhotonLibraryLoaded()),
          electronics_response_(MakeElectronicsConfigFromEnv())
    {
        const char* configured_filename = std::getenv("G4LARBOX_OUTPUT_FILE");
        rootFile = new TFile(configured_filename != nullptr ? configured_filename : filename, "RECREATE");

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
        stepTree->Branch("trkid", &trkid_);
        stepTree->Branch("step_pdg", &steppdg_);
        stepTree->Branch("nexc", &nexc_);
        stepTree->Branch("nion", &nion_);
        stepTree->Branch("nopt", &nopt_);
        stepTree->Branch("ntherm", &ntherm_);

        trackTree = new TTree("trackTree", "trackTree");
        trackTree->Branch("xv", &xv_);
        trackTree->Branch("yv", &yv_);
        trackTree->Branch("zv", &zv_);
        trackTree->Branch("tv", &tv_);
        trackTree->Branch("xf", &xf_);
        trackTree->Branch("yf", &yf_);
        trackTree->Branch("zf", &zf_);
        trackTree->Branch("tf", &tf_);
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

        eventTree = new TTree("eventTree", "eventTree");
        eventTree->Branch("tnexc", &tnexc_);
        eventTree->Branch("tnion", &tnion_);
        eventTree->Branch("tnopt", &tnopt_);
        eventTree->Branch("tntherm", &tntherm_);
        eventTree->Branch("optical_hits", &optical_hits_);
        eventTree->Branch("optical_x", &optical_x_);
        eventTree->Branch("optical_y", &optical_y_);
        eventTree->Branch("optical_z", &optical_z_);
        eventTree->Branch("optical_t", &optical_t_);
        eventTree->Branch("optical_energy", &optical_energy_);
        eventTree->Branch("optical_wavelength_nm", &optical_wavelength_nm_);
        eventTree->Branch("optical_track_id", &optical_track_id_);
        eventTree->Branch("optical_parent_id", &optical_parent_id_);
        eventTree->Branch("optical_copy_number", &optical_copy_number_);
        eventTree->Branch("optical_volume", &optical_volume_);
        eventTree->Branch("fast_optical_enabled", &fast_optical_enabled_);
        eventTree->Branch("fast_optical_model", &fast_optical_model_);
        eventTree->Branch("photon_library_loaded", &photon_library_loaded_);
        eventTree->Branch("fast_optical_hits", &fast_optical_hits_);
        eventTree->Branch("electronics_waveforms", &electronics_waveforms_);
        eventTree->Branch("electronics_sample_frequency_mhz", &electronics_sample_frequency_mhz_);
        eventTree->Branch("electronics_time_begin_us", &electronics_time_begin_us_);
        eventTree->Branch("electronics_time_end_us", &electronics_time_end_us_);
        eventTree->Branch("electronics_channel", &electronics_channel_);
        eventTree->Branch("electronics_channel_readout", &electronics_channel_readout_);
        eventTree->Branch("electronics_sample_offset", &electronics_sample_offset_);
        eventTree->Branch("electronics_sample_count", &electronics_sample_count_);
        eventTree->Branch("electronics_adc", &electronics_adc_);

        truthTree = new TTree("truthTree", "Generator truth information");
        truthTree->Branch("source", &generator_source_);
        truthTree->Branch("vertex_x", &generator_vertex_x_);
        truthTree->Branch("vertex_y", &generator_vertex_y_);
        truthTree->Branch("vertex_z", &generator_vertex_z_);
        truthTree->Branch("vertex_t", &generator_vertex_t_);
        truthTree->Branch("has_incident_direction", &generator_has_incident_direction_);
        truthTree->Branch("incident_dir_x", &generator_incident_dir_x_);
        truthTree->Branch("incident_dir_y", &generator_incident_dir_y_);
        truthTree->Branch("incident_dir_z", &generator_incident_dir_z_);
        truthTree->Branch("genie_iev", &genie_iev_);
        truthTree->Branch("genie_neu", &genie_neu_);
        truthTree->Branch("genie_tgt", &genie_tgt_);
        truthTree->Branch("genie_target_z", &genie_target_z_);
        truthTree->Branch("genie_target_a", &genie_target_a_);
        truthTree->Branch("genie_cc", &genie_cc_);
        truthTree->Branch("genie_nc", &genie_nc_);
        truthTree->Branch("genie_qel", &genie_qel_);
        truthTree->Branch("genie_res", &genie_res_);
        truthTree->Branch("genie_dis", &genie_dis_);
        truthTree->Branch("genie_coh", &genie_coh_);
        truthTree->Branch("genie_nuel", &genie_nuel_);
        truthTree->Branch("genie_imd", &genie_imd_);
        truthTree->Branch("genie_em", &genie_em_);
        truthTree->Branch("genie_weight", &genie_weight_);
        truthTree->Branch("genie_xs", &genie_xs_);
        truthTree->Branch("genie_ev", &genie_ev_);
        truthTree->Branch("genie_input_vtxx", &genie_input_vtxx_);
        truthTree->Branch("genie_input_vtxy", &genie_input_vtxy_);
        truthTree->Branch("genie_input_vtxz", &genie_input_vtxz_);
        truthTree->Branch("genie_input_vtxt", &genie_input_vtxt_);
        truthTree->Branch("marley_event", &marley_event_);
        truthTree->Branch("marley_flux_averaged_xsec", &marley_flux_averaged_xsec_);
        truthTree->Branch("marley_projectile_pdg", &marley_projectile_pdg_);
        truthTree->Branch("marley_target_pdg", &marley_target_pdg_);
        truthTree->Branch("marley_ejectile_pdg", &marley_ejectile_pdg_);
        truthTree->Branch("marley_residue_pdg", &marley_residue_pdg_);
        truthTree->Branch("marley_ex", &marley_ex_);
        truthTree->Branch("bxdecay0_category", &bxdecay0_category_);
        truthTree->Branch("bxdecay0_nuclide", &bxdecay0_nuclide_);
        truthTree->Branch("bxdecay0_seed", &bxdecay0_seed_);
        truthTree->Branch("bxdecay0_event", &bxdecay0_event_);
        truthTree->Branch("bxdecay0_particles", &bxdecay0_particles_);
        truthTree->Branch("bxdecay0_event_time_ns", &bxdecay0_event_time_ns_);
        truthTree->Branch("bxdecay0_pdg", &bxdecay0_pdg_);
        truthTree->Branch("bxdecay0_px_mev", &bxdecay0_px_mev_);
        truthTree->Branch("bxdecay0_py_mev", &bxdecay0_py_mev_);
        truthTree->Branch("bxdecay0_pz_mev", &bxdecay0_pz_mev_);
        truthTree->Branch("bxdecay0_time_ns", &bxdecay0_time_ns_);
        truthTree->Branch("radiological_enabled", &radiological_enabled_);
        truthTree->Branch("radiological_mass_kg", &radiological_mass_kg_);
        truthTree->Branch("radiological_window_us", &radiological_window_us_);
        truthTree->Branch("radiological_expected_decays", &radiological_expected_decays_);
        truthTree->Branch("radiological_decays", &radiological_decays_);
        truthTree->Branch("radiological_isotope", &radiological_isotope_);
        truthTree->Branch("radiological_z", &radiological_z_);
        truthTree->Branch("radiological_a", &radiological_a_);
        truthTree->Branch("radiological_activity_bq_per_kg", &radiological_activity_bq_per_kg_);
        truthTree->Branch("radiological_decay_time_ns", &radiological_decay_time_ns_);
        truthTree->Branch("rock_neutron_enabled", &rock_neutron_enabled_);
        truthTree->Branch("rock_neutron_window_us", &rock_neutron_window_us_);
        truthTree->Branch("rock_neutron_expected", &rock_neutron_expected_);
        truthTree->Branch("rock_neutron_count", &rock_neutron_count_);
        truthTree->Branch("rock_neutron_x", &rock_neutron_x_);
        truthTree->Branch("rock_neutron_y", &rock_neutron_y_);
        truthTree->Branch("rock_neutron_z", &rock_neutron_z_);
        truthTree->Branch("rock_neutron_time_ns", &rock_neutron_time_ns_);
        truthTree->Branch("rock_neutron_energy_mev", &rock_neutron_energy_mev_);
        truthTree->Branch("rock_neutron_dir_x", &rock_neutron_dir_x_);
        truthTree->Branch("rock_neutron_dir_y", &rock_neutron_dir_y_);
        truthTree->Branch("rock_neutron_dir_z", &rock_neutron_dir_z_);
        truthTree->Branch("rock_neutron_face", &rock_neutron_face_);
        truthTree->Branch("primary_pdg", &generator_primary_pdg_);
        truthTree->Branch("primary_energy", &generator_primary_energy_);
        truthTree->Branch("primary_px", &generator_primary_px_);
        truthTree->Branch("primary_py", &generator_primary_py_);
        truthTree->Branch("primary_pz", &generator_primary_pz_);

        Reset();
    }

    void DataHandler::AddStep(const G4Step* step, int nexc, int nion, int nopt, int ntherm) 
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
        trkid_.push_back(step->GetTrack()->GetTrackID());
        steppdg_.push_back(step->GetTrack()->GetDefinition()->GetPDGEncoding());

        nexc_.push_back(nexc);
        nion_.push_back(nion);
        nopt_.push_back(nopt);
        ntherm_.push_back(ntherm);

        tnexc_ += nexc;
        tnion_ += nion;
        tnopt_ += nopt;
        tntherm_ += ntherm;

        AddFastOpticalHits(step, nopt);
    }

    void DataHandler::AddFastOpticalHits(const G4Step* step, int nopt)
    {
        if (!fast_optical_enabled_ || nopt <= 0)
        {
            return;
        }

        const G4ThreeVector start = step->GetPreStepPoint()->GetPosition();
        const G4ThreeVector end = step->GetPostStepPoint()->GetPosition();
        const G4ThreeVector mid = 0.5 * (start + end);

        OpticalResponse::StepDeposit deposit;
        deposit.x_mm = mid.x();
        deposit.y_mm = mid.y();
        deposit.z_mm = mid.z();
        deposit.time_ns = 0.5 * (step->GetPreStepPoint()->GetGlobalTime() +
                                 step->GetPostStepPoint()->GetGlobalTime());
        deposit.photons = nopt;

        const auto hits = optical_response_.GenerateHits(deposit);
        const double detected_energy = (h_Planck * c_light) / (430.0 * nm);
        const auto* track = step->GetTrack();
        for (const auto& hit : hits)
        {
            optical_x_.push_back(hit.x_mm);
            optical_y_.push_back(hit.y_mm);
            optical_z_.push_back(hit.z_mm);
            optical_t_.push_back(hit.time_ns);
            optical_energy_.push_back(detected_energy);
            optical_wavelength_nm_.push_back(hit.wavelength_nm);
            optical_track_id_.push_back(track->GetTrackID());
            optical_parent_id_.push_back(track->GetParentID());
            optical_copy_number_.push_back(hit.channel);
            if (!hit.label.empty())
            {
                optical_volume_.push_back(hit.label);
            }
            else
            {
                optical_volume_.push_back(
                    hit.component == OpticalResponse::Component::Direct
                        ? "fastOpticalDirect"
                        : (hit.component == OpticalResponse::Component::Diffuse
                               ? "fastOpticalDiffuse"
                               : "dunevdArapucaPhotonLibrary"));
            }
            ++optical_hits_;
            ++fast_optical_hits_;
        }
    }

    void DataHandler::AddOpticalHit(const G4Step* step)
    {
        const auto* track = step->GetTrack();
        const auto* post_step = step->GetPostStepPoint();
        const G4ThreeVector pos = post_step->GetPosition();
        const double energy = track->GetKineticEnergy();

        optical_x_.push_back(pos.x());
        optical_y_.push_back(pos.y());
        optical_z_.push_back(pos.z());
        optical_t_.push_back(post_step->GetGlobalTime());
        optical_energy_.push_back(energy);
        optical_wavelength_nm_.push_back(energy > 0.0 ? (h_Planck * c_light / energy) / nm : 0.0);
        optical_track_id_.push_back(track->GetTrackID());
        optical_parent_id_.push_back(track->GetParentID());

        std::string volume_name = "";
        const auto* volume = post_step->GetPhysicalVolume();
        if (volume != nullptr)
        {
            volume_name = volume->GetLogicalVolume()->GetName();
        }

        const int copy_number = ResolveOpticalCopyNumber(step, volume_name);
        optical_copy_number_.push_back(copy_number);
        optical_volume_.push_back(volume_name);
        ++optical_hits_;
    }

    void DataHandler::AddTrack(const G4Track* track) 
    {
        G4ThreeVector pos = track->GetPosition();
        G4ThreeVector vertex = track->GetVertexPosition();

        xv_.push_back(vertex.x());
        yv_.push_back(vertex.y());
        zv_.push_back(vertex.z());
        tv_.push_back(track->GetGlobalTime() - track->GetLocalTime());

        xf_.push_back(pos.x());
        yf_.push_back(pos.y());
        zf_.push_back(pos.z());
        tf_.push_back(track->GetGlobalTime());

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
        BuildElectronicsResponse();
        stepTree->Fill();
        trackTree->Fill();
        eventTree->Fill();
        truthTree->Fill();
    }

    void DataHandler::BuildElectronicsResponse()
    {
        electronics_waveforms_ = 0;
        electronics_channel_.clear();
        electronics_channel_readout_.clear();
        electronics_sample_offset_.clear();
        electronics_sample_count_.clear();
        electronics_adc_.clear();

        std::vector<ElectronicsResponse::OpticalHit> hits;
        hits.reserve(optical_t_.size());
        for (size_t i = 0; i < optical_t_.size(); ++i)
        {
            if (i >= optical_copy_number_.size() || optical_copy_number_[i] < 0)
            {
                continue;
            }

            ElectronicsResponse::OpticalHit hit;
            hit.channel = optical_copy_number_[i];
            hit.time_ns = optical_t_[i];
            hit.wavelength_nm = i < optical_wavelength_nm_.size() ? optical_wavelength_nm_[i] : 0.0;
            hits.push_back(hit);
        }

        const auto waveforms = electronics_response_.GenerateWaveforms(hits);
        electronics_waveforms_ = static_cast<int>(waveforms.size());
        for (const auto& waveform : waveforms)
        {
            electronics_channel_.push_back(waveform.channel);
            electronics_channel_readout_.push_back(optical_response_.ChannelReadout(waveform.channel));
            electronics_sample_offset_.push_back(static_cast<int>(electronics_adc_.size()));
            electronics_sample_count_.push_back(static_cast<int>(waveform.adc.size()));
            electronics_adc_.insert(electronics_adc_.end(),
                                    waveform.adc.begin(),
                                    waveform.adc.end());
        }

        electronics_sample_frequency_mhz_ = electronics_response_.SampleFrequencyMHz();
        electronics_time_begin_us_ = electronics_response_.TimeBeginUs();
        electronics_time_end_us_ = electronics_response_.TimeEndUs();
    }

    void DataHandler::SetGeneratorTruth(const GeneratorTruthRecord& truth)
    {
        generator_source_ = truth.source;
        generator_vertex_x_ = truth.vertex_x;
        generator_vertex_y_ = truth.vertex_y;
        generator_vertex_z_ = truth.vertex_z;
        generator_vertex_t_ = truth.vertex_t;
        generator_has_incident_direction_ = truth.has_incident_direction;
        generator_incident_dir_x_ = truth.incident_dir_x;
        generator_incident_dir_y_ = truth.incident_dir_y;
        generator_incident_dir_z_ = truth.incident_dir_z;

        genie_iev_ = truth.genie_iev;
        genie_neu_ = truth.genie_neu;
        genie_tgt_ = truth.genie_tgt;
        genie_target_z_ = truth.genie_target_z;
        genie_target_a_ = truth.genie_target_a;
        genie_cc_ = truth.genie_cc;
        genie_nc_ = truth.genie_nc;
        genie_qel_ = truth.genie_qel;
        genie_res_ = truth.genie_res;
        genie_dis_ = truth.genie_dis;
        genie_coh_ = truth.genie_coh;
        genie_nuel_ = truth.genie_nuel;
        genie_imd_ = truth.genie_imd;
        genie_em_ = truth.genie_em;
        genie_weight_ = truth.genie_weight;
        genie_xs_ = truth.genie_xs;
        genie_ev_ = truth.genie_ev;
        genie_input_vtxx_ = truth.genie_input_vtxx;
        genie_input_vtxy_ = truth.genie_input_vtxy;
        genie_input_vtxz_ = truth.genie_input_vtxz;
        genie_input_vtxt_ = truth.genie_input_vtxt;
        marley_event_ = truth.marley_event;
        marley_flux_averaged_xsec_ = truth.marley_flux_averaged_xsec;
        marley_projectile_pdg_ = truth.marley_projectile_pdg;
        marley_target_pdg_ = truth.marley_target_pdg;
        marley_ejectile_pdg_ = truth.marley_ejectile_pdg;
        marley_residue_pdg_ = truth.marley_residue_pdg;
        marley_ex_ = truth.marley_ex;
        bxdecay0_category_ = truth.bxdecay0_category;
        bxdecay0_nuclide_ = truth.bxdecay0_nuclide;
        bxdecay0_seed_ = truth.bxdecay0_seed;
        bxdecay0_event_ = truth.bxdecay0_event;
        bxdecay0_particles_ = truth.bxdecay0_particles;
        bxdecay0_event_time_ns_ = truth.bxdecay0_event_time_ns;
        bxdecay0_pdg_ = truth.bxdecay0_pdg;
        bxdecay0_px_mev_ = truth.bxdecay0_px_mev;
        bxdecay0_py_mev_ = truth.bxdecay0_py_mev;
        bxdecay0_pz_mev_ = truth.bxdecay0_pz_mev;
        bxdecay0_time_ns_ = truth.bxdecay0_time_ns;
        radiological_enabled_ = truth.radiological_enabled;
        radiological_mass_kg_ = truth.radiological_mass_kg;
        radiological_window_us_ = truth.radiological_window_us;
        radiological_expected_decays_ = truth.radiological_expected_decays;
        radiological_decays_ = truth.radiological_decays;
        radiological_isotope_ = truth.radiological_isotope;
        radiological_z_ = truth.radiological_z;
        radiological_a_ = truth.radiological_a;
        radiological_activity_bq_per_kg_ = truth.radiological_activity_bq_per_kg;
        radiological_decay_time_ns_ = truth.radiological_decay_time_ns;
        rock_neutron_enabled_ = truth.rock_neutron_enabled;
        rock_neutron_window_us_ = truth.rock_neutron_window_us;
        rock_neutron_expected_ = truth.rock_neutron_expected;
        rock_neutron_count_ = truth.rock_neutron_count;
        rock_neutron_x_ = truth.rock_neutron_x;
        rock_neutron_y_ = truth.rock_neutron_y;
        rock_neutron_z_ = truth.rock_neutron_z;
        rock_neutron_time_ns_ = truth.rock_neutron_time_ns;
        rock_neutron_energy_mev_ = truth.rock_neutron_energy_mev;
        rock_neutron_dir_x_ = truth.rock_neutron_dir_x;
        rock_neutron_dir_y_ = truth.rock_neutron_dir_y;
        rock_neutron_dir_z_ = truth.rock_neutron_dir_z;
        rock_neutron_face_ = truth.rock_neutron_face;

        generator_primary_pdg_ = truth.primary_pdg;
        generator_primary_energy_ = truth.primary_energy;
        generator_primary_px_ = truth.primary_px;
        generator_primary_py_ = truth.primary_py;
        generator_primary_pz_ = truth.primary_pz;
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
        trkid_.clear();
        steppdg_.clear();
        nexc_.clear();
        nion_.clear();
        nopt_.clear();
        ntherm_.clear();

        optical_hits_ = 0;
        optical_x_.clear();
        optical_y_.clear();
        optical_z_.clear();
        optical_t_.clear();
        optical_energy_.clear();
        optical_wavelength_nm_.clear();
        optical_track_id_.clear();
        optical_parent_id_.clear();
        optical_copy_number_.clear();
        optical_volume_.clear();
        fast_optical_hits_ = 0;
        electronics_waveforms_ = 0;
        electronics_sample_frequency_mhz_ = electronics_response_.SampleFrequencyMHz();
        electronics_time_begin_us_ = electronics_response_.TimeBeginUs();
        electronics_time_end_us_ = electronics_response_.TimeEndUs();
        electronics_channel_.clear();
        electronics_sample_offset_.clear();
        electronics_sample_count_.clear();
        electronics_channel_readout_.clear();
        electronics_adc_.clear();

        xi_.clear();
        yi_.clear();
        zi_.clear();
        ti_.clear();
        xv_.clear();
        yv_.clear();
        zv_.clear();
        tv_.clear();
        xf_.clear();
        yf_.clear();
        zf_.clear();
        tf_.clear();
        pxi_.clear();
        pyi_.clear();
        pzi_.clear();
        ekini_.clear();
        pdg_.clear();
        curid_.clear();
        preid_.clear();

        generator_source_ = "unknown";
        generator_vertex_x_ = 0.0;
        generator_vertex_y_ = 0.0;
        generator_vertex_z_ = 0.0;
        generator_vertex_t_ = 0.0;
        generator_has_incident_direction_ = false;
        generator_incident_dir_x_ = 0.0;
        generator_incident_dir_y_ = 0.0;
        generator_incident_dir_z_ = 0.0;

        genie_iev_ = -1;
        genie_neu_ = 0;
        genie_tgt_ = 0;
        genie_target_z_ = 0;
        genie_target_a_ = 0;
        genie_cc_ = false;
        genie_nc_ = false;
        genie_qel_ = false;
        genie_res_ = false;
        genie_dis_ = false;
        genie_coh_ = false;
        genie_nuel_ = false;
        genie_imd_ = false;
        genie_em_ = false;
        genie_weight_ = 1.0;
        genie_xs_ = 0.0;
        genie_ev_ = 0.0;
        genie_input_vtxx_ = 0.0;
        genie_input_vtxy_ = 0.0;
        genie_input_vtxz_ = 0.0;
        genie_input_vtxt_ = 0.0;
        marley_event_ = -1;
        marley_flux_averaged_xsec_ = 0.0;
        marley_projectile_pdg_ = 0;
        marley_target_pdg_ = 0;
        marley_ejectile_pdg_ = 0;
        marley_residue_pdg_ = 0;
        marley_ex_ = 0.0;
        bxdecay0_category_.clear();
        bxdecay0_nuclide_.clear();
        bxdecay0_seed_ = 0;
        bxdecay0_event_ = -1;
        bxdecay0_particles_ = 0;
        bxdecay0_event_time_ns_ = 0.0;
        bxdecay0_pdg_.clear();
        bxdecay0_px_mev_.clear();
        bxdecay0_py_mev_.clear();
        bxdecay0_pz_mev_.clear();
        bxdecay0_time_ns_.clear();
        radiological_enabled_ = false;
        radiological_mass_kg_ = 0.0;
        radiological_window_us_ = 0.0;
        radiological_expected_decays_ = 0.0;
        radiological_decays_ = 0;
        radiological_isotope_.clear();
        radiological_z_.clear();
        radiological_a_.clear();
        radiological_activity_bq_per_kg_.clear();
        radiological_decay_time_ns_.clear();
        rock_neutron_enabled_ = false;
        rock_neutron_window_us_ = 0.0;
        rock_neutron_expected_ = 0.0;
        rock_neutron_count_ = 0;
        rock_neutron_x_.clear();
        rock_neutron_y_.clear();
        rock_neutron_z_.clear();
        rock_neutron_time_ns_.clear();
        rock_neutron_energy_mev_.clear();
        rock_neutron_dir_x_.clear();
        rock_neutron_dir_y_.clear();
        rock_neutron_dir_z_.clear();
        rock_neutron_face_.clear();

        generator_primary_pdg_.clear();
        generator_primary_energy_.clear();
        generator_primary_px_.clear();
        generator_primary_py_.clear();
        generator_primary_pz_.clear();

        tnexc_ = 0;
        tnion_ = 0;
        tnopt_ = 0;
        tntherm_ = 0;
    }
}

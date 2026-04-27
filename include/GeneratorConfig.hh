#ifndef GENERATORCONFIG_HH
#define GENERATORCONFIG_HH

#include <string>
#include <vector>

namespace G4LArBox
{
    enum class GeneratorMode
    {
        GPS,
        GenieGST,
        GenieProtonDecay,
        Corsika,
        CorsikaGenieOverlay,
        Marley,
        BxDecay0,
        Radiological,
        RockNeutrons
    };

    struct BxDecay0Config
    {
        std::string category = "background";
        std::string nuclide = "Ar39";
        int seed = 123456;
        bool debug = false;
        int verbosity = 0;

        int dbd_mode = 0;
        int dbd_level = 0;
        double dbd_min_energy_mev = -1.0;
        double dbd_max_energy_mev = -1.0;

        bool use_mdl = false;
        std::string mdl_target_name = "all";
        int mdl_target_rank = -1;
        double mdl_cone_longitude_deg = 0.0;
        double mdl_cone_colatitude_deg = 0.0;
        double mdl_cone_aperture_deg = 0.0;
        double mdl_cone_aperture2_deg = -1.0;
        bool mdl_error_on_missing_particle = false;
    };

    struct RadiologicalIsotopeConfig
    {
        std::string name = "Ar39";
        int z = 18;
        int a = 39;
        double activity_bq_per_kg = 1.01;
        double excitation_mev = 0.0;
    };

    struct RockNeutronConfig
    {
        bool enabled = false;
        double window_us = 10.0;
        double mean_per_event = -1.0;
        double flux_per_cm2_s = 0.0;
        double rate_hz = 0.0;
        int max_neutrons_per_event = 10000;
        double shell_padding_cm = 100.0;
        std::string spectrum = "radiogenic";
        double energy_min_mev = 0.1;
        double energy_max_mev = 10.0;
        double energy_mean_mev = 2.0;
        std::string direction_model = "cosine";
    };

    struct GeneratorConfig
    {
        GeneratorMode mode = GeneratorMode::GPS;
        std::string genie_file_path;
        std::string genie_tree_name = "gst";
        bool genie_use_input_vertex = false;
        bool genie_cycle_events = false;
        std::string corsika_file_path;
        std::string corsika_tree_name = "corsika";
        bool corsika_cycle_events = false;
        std::string marley_config_path;
        std::string marley_event_file_path;
        bool marley_cycle_events = false;
        unsigned long marley_seed = 0;
        bool marley_has_seed = false;
        BxDecay0Config bxdecay0;
        bool radiological_enabled = false;
        double radiological_window_us = 10.0;
        double radiological_mass_override_kg = 0.0;
        int radiological_max_decays_per_event = 10000;
        std::vector<RadiologicalIsotopeConfig> radiological_isotopes;
        RockNeutronConfig rock_neutrons;
    };
}

#endif // GENERATORCONFIG_HH

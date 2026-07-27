#include "GeneratorMessenger.hh"

#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithAString.hh"
#include "G4Exception.hh"
#include "G4SystemOfUnits.hh"
#include "G4UIdirectory.hh"
#include "G4UIcommand.hh"
#include "G4UIparameter.hh"
#include "G4String.hh"
#include "G4Tokenizer.hh"

#include <cstdlib>
#include <vector>
#include <string>

namespace G4LArBox
{
    namespace
    {
        bool ParseBoolToken(const std::string& text, bool fallback)
        {
            if (text.empty())
            {
                return fallback;
            }
            return text == "1" || text == "true" || text == "TRUE" ||
                   text == "on" || text == "ON" ||
                   text == "yes" || text == "YES";
        }

        G4UIparameter* OptionalParameter(const char* name, char type, const char* default_value)
        {
            auto* parameter = new G4UIparameter(name, type, true);
            parameter->SetDefaultValue(default_value);
            return parameter;
        }
    }

    GeneratorMessenger::GeneratorMessenger(GeneratorConfig& config)
        : G4UImessenger(),
          config_(config),
          generator_directory_(new G4UIdirectory("/generator/")),
          genie_directory_(new G4UIdirectory("/generator/genie/")),
          corsika_directory_(new G4UIdirectory("/generator/corsika/")),
          marley_directory_(new G4UIdirectory("/generator/marley/")),
          bxdecay0_directory_(new G4UIdirectory("/generator/bxdecay0/")),
          radiological_directory_(new G4UIdirectory("/generator/radiological/")),
          rock_neutron_directory_(new G4UIdirectory("/generator/rockNeutrons/")),
          generator_type_cmd_(new G4UIcmdWithAString("/generator/type", this)),
          genie_file_cmd_(new G4UIcmdWithAString("/generator/genie/file", this)),
          genie_tree_cmd_(new G4UIcmdWithAString("/generator/genie/tree", this)),
          genie_use_input_vertex_cmd_(new G4UIcmdWithABool("/generator/genie/useInputVertex", this)),
          genie_cycle_events_cmd_(new G4UIcmdWithABool("/generator/genie/cycleEvents", this)),
          corsika_file_cmd_(new G4UIcmdWithAString("/generator/corsika/file", this)),
          corsika_tree_cmd_(new G4UIcmdWithAString("/generator/corsika/tree", this)),
          corsika_cycle_events_cmd_(new G4UIcmdWithABool("/generator/corsika/cycleEvents", this)),
          marley_config_cmd_(new G4UIcmdWithAString("/generator/marley/config", this)),
          marley_event_file_cmd_(new G4UIcmdWithAString("/generator/marley/file", this)),
          marley_cycle_events_cmd_(new G4UIcmdWithABool("/generator/marley/cycleEvents", this)),
          marley_seed_cmd_(new G4UIcmdWithAnInteger("/generator/marley/seed", this)),
          bxdecay0_background_cmd_(new G4UIcommand("/generator/bxdecay0/background", this)),
          bxdecay0_dbd_cmd_(new G4UIcommand("/generator/bxdecay0/dbd", this)),
          bxdecay0_dbdranged_cmd_(new G4UIcommand("/generator/bxdecay0/dbdranged", this)),
          bxdecay0_mdl_cmd_(new G4UIcommand("/generator/bxdecay0/mdl", this)),
          bxdecay0_mdlr_cmd_(new G4UIcommand("/generator/bxdecay0/mdlr", this)),
          radiological_enable_cmd_(new G4UIcmdWithABool("/generator/radiological/enable", this)),
          radiological_window_cmd_(new G4UIcmdWithADoubleAndUnit("/generator/radiological/window", this)),
          radiological_mass_override_cmd_(new G4UIcmdWithADouble("/generator/radiological/massOverrideKg", this)),
          radiological_max_decays_cmd_(new G4UIcmdWithAnInteger("/generator/radiological/maxDecaysPerEvent", this)),
          radiological_clear_cmd_(new G4UIcommand("/generator/radiological/clear", this)),
          radiological_add_isotope_cmd_(new G4UIcommand("/generator/radiological/addIsotope", this)),
          rock_neutron_enable_cmd_(new G4UIcmdWithABool("/generator/rockNeutrons/enable", this)),
          rock_neutron_window_cmd_(new G4UIcmdWithADoubleAndUnit("/generator/rockNeutrons/window", this)),
          rock_neutron_mean_cmd_(new G4UIcmdWithADouble("/generator/rockNeutrons/meanPerEvent", this)),
          rock_neutron_flux_cmd_(new G4UIcmdWithADouble("/generator/rockNeutrons/flux", this)),
          rock_neutron_rate_cmd_(new G4UIcmdWithADouble("/generator/rockNeutrons/rate", this)),
          rock_neutron_max_cmd_(new G4UIcmdWithAnInteger("/generator/rockNeutrons/maxNeutronsPerEvent", this)),
          rock_neutron_padding_cmd_(new G4UIcmdWithADoubleAndUnit("/generator/rockNeutrons/shellPadding", this)),
          rock_neutron_spectrum_cmd_(new G4UIcmdWithAString("/generator/rockNeutrons/spectrum", this)),
          rock_neutron_energy_range_cmd_(new G4UIcommand("/generator/rockNeutrons/energyRange", this)),
          rock_neutron_energy_mean_cmd_(new G4UIcmdWithADoubleAndUnit("/generator/rockNeutrons/energyMean", this)),
          rock_neutron_direction_cmd_(new G4UIcmdWithAString("/generator/rockNeutrons/direction", this))
    {
        generator_directory_->SetGuidance("Primary generator configuration.");
        genie_directory_->SetGuidance("GENIE GST input configuration.");
        corsika_directory_->SetGuidance("CORSIKA primary input configuration.");
        marley_directory_->SetGuidance("MARLEY low-energy neutrino generator configuration.");
        bxdecay0_directory_->SetGuidance("BxDecay0 radiological and double-beta decay generator configuration.");
        radiological_directory_->SetGuidance("Radiological background overlay configuration.");
        rock_neutron_directory_->SetGuidance("External neutron flux from surrounding rock.");

        generator_type_cmd_->SetGuidance("Select the primary generator source: gps, genie_gst, genie_pdecay, corsika, corsika_genie_overlay, marley, bxdecay0, radiological, or rock_neutrons.");
        generator_type_cmd_->SetParameterName("type", false);
        generator_type_cmd_->SetCandidates("gps genie_gst genie_pdecay corsika corsika_genie_overlay marley bxdecay0 radiological rock_neutrons");

        genie_file_cmd_->SetGuidance("Path to a ROOT file containing a GENIE gst tree.");
        genie_file_cmd_->SetParameterName("path", false);

        genie_tree_cmd_->SetGuidance("Tree name inside the GENIE ROOT file.");
        genie_tree_cmd_->SetParameterName("tree", false);
        genie_tree_cmd_->SetDefaultValue("gst");

        genie_use_input_vertex_cmd_->SetGuidance("Use the GENIE gst SI-unit interaction vertex instead of sampling inside the detector.");
        genie_use_input_vertex_cmd_->SetParameterName("useInputVertex", true);
        genie_use_input_vertex_cmd_->SetDefaultValue(false);

        genie_cycle_events_cmd_->SetGuidance("Loop back to the first GENIE event when /run/beamOn exceeds the input tree length.");
        genie_cycle_events_cmd_->SetParameterName("cycleEvents", true);
        genie_cycle_events_cmd_->SetDefaultValue(false);

        corsika_file_cmd_->SetGuidance("Path to a ROOT file containing a CORSIKA primary tree.");
        corsika_file_cmd_->SetParameterName("path", false);

        corsika_tree_cmd_->SetGuidance("Tree name inside the CORSIKA ROOT file.");
        corsika_tree_cmd_->SetParameterName("tree", false);
        corsika_tree_cmd_->SetDefaultValue("corsika");

        corsika_cycle_events_cmd_->SetGuidance("Loop back to the first CORSIKA event when /run/beamOn exceeds the input tree length.");
        corsika_cycle_events_cmd_->SetParameterName("cycleEvents", true);
        corsika_cycle_events_cmd_->SetDefaultValue(false);

        marley_config_cmd_->SetGuidance("Path to a MARLEY JSON configuration file used to generate events on the fly.");
        marley_config_cmd_->SetParameterName("path", false);

        marley_event_file_cmd_->SetGuidance("Optional path to a pre-generated MARLEY event file. If set, this is read instead of generating from JSON.");
        marley_event_file_cmd_->SetParameterName("path", false);

        marley_cycle_events_cmd_->SetGuidance("Loop back to the first MARLEY file event when /run/beamOn exceeds the input file length.");
        marley_cycle_events_cmd_->SetParameterName("cycleEvents", true);
        marley_cycle_events_cmd_->SetDefaultValue(false);

        marley_seed_cmd_->SetGuidance("Optional seed used after loading the MARLEY JSON configuration.");
        marley_seed_cmd_->SetParameterName("seed", false);

        bxdecay0_background_cmd_->SetGuidance("Configure BxDecay0 background decays: nuclide seed [debug].");
        bxdecay0_background_cmd_->SetParameter(new G4UIparameter("nuclide", 's', false));
        bxdecay0_background_cmd_->SetParameter(new G4UIparameter("seed", 'i', false));
        auto* bx_background_debug = new G4UIparameter("debug", 'b', true);
        bx_background_debug->SetDefaultValue("false");
        bxdecay0_background_cmd_->SetParameter(bx_background_debug);

        bxdecay0_dbd_cmd_->SetGuidance("Configure BxDecay0 double-beta decays: nuclide seed dbd_mode dbd_level [debug].");
        bxdecay0_dbd_cmd_->SetParameter(new G4UIparameter("nuclide", 's', false));
        bxdecay0_dbd_cmd_->SetParameter(new G4UIparameter("seed", 'i', false));
        bxdecay0_dbd_cmd_->SetParameter(new G4UIparameter("dbd_mode", 'i', false));
        bxdecay0_dbd_cmd_->SetParameter(new G4UIparameter("dbd_level", 'i', false));
        auto* bx_dbd_debug = new G4UIparameter("debug", 'b', true);
        bx_dbd_debug->SetDefaultValue("false");
        bxdecay0_dbd_cmd_->SetParameter(bx_dbd_debug);

        bxdecay0_dbdranged_cmd_->SetGuidance("Configure BxDecay0 double-beta decays with an energy-sum range: nuclide seed dbd_mode dbd_level min_MeV max_MeV [debug].");
        bxdecay0_dbdranged_cmd_->SetParameter(new G4UIparameter("nuclide", 's', false));
        bxdecay0_dbdranged_cmd_->SetParameter(new G4UIparameter("seed", 'i', false));
        bxdecay0_dbdranged_cmd_->SetParameter(new G4UIparameter("dbd_mode", 'i', false));
        bxdecay0_dbdranged_cmd_->SetParameter(new G4UIparameter("dbd_level", 'i', false));
        bxdecay0_dbdranged_cmd_->SetParameter(new G4UIparameter("min_MeV", 'd', false));
        bxdecay0_dbdranged_cmd_->SetParameter(new G4UIparameter("max_MeV", 'd', false));
        auto* bx_dbdranged_debug = new G4UIparameter("debug", 'b', true);
        bx_dbdranged_debug->SetDefaultValue("false");
        bxdecay0_dbdranged_cmd_->SetParameter(bx_dbdranged_debug);

        bxdecay0_mdl_cmd_->SetGuidance("Configure BxDecay0 momentum-direction lock: target rank longitude_deg colatitude_deg aperture_deg [error_on_missing_target].");
        bxdecay0_mdl_cmd_->SetParameter(OptionalParameter("target", 's', "all"));
        bxdecay0_mdl_cmd_->SetParameter(OptionalParameter("rank", 'i', "-1"));
        bxdecay0_mdl_cmd_->SetParameter(OptionalParameter("longitude_deg", 'd', "0.0"));
        bxdecay0_mdl_cmd_->SetParameter(OptionalParameter("colatitude_deg", 'd', "0.0"));
        bxdecay0_mdl_cmd_->SetParameter(OptionalParameter("aperture_deg", 'd', "0.0"));
        bxdecay0_mdl_cmd_->SetParameter(OptionalParameter("error_on_missing_target", 'b', "false"));

        bxdecay0_mdlr_cmd_->SetGuidance("Configure BxDecay0 rectangular momentum-direction lock: target rank longitude_deg colatitude_deg aperture_deg aperture2_deg [error_on_missing_target].");
        bxdecay0_mdlr_cmd_->SetParameter(OptionalParameter("target", 's', "all"));
        bxdecay0_mdlr_cmd_->SetParameter(OptionalParameter("rank", 'i', "-1"));
        bxdecay0_mdlr_cmd_->SetParameter(OptionalParameter("longitude_deg", 'd', "0.0"));
        bxdecay0_mdlr_cmd_->SetParameter(OptionalParameter("colatitude_deg", 'd', "0.0"));
        bxdecay0_mdlr_cmd_->SetParameter(OptionalParameter("aperture_deg", 'd', "0.0"));
        bxdecay0_mdlr_cmd_->SetParameter(OptionalParameter("aperture2_deg", 'd', "0.0"));
        bxdecay0_mdlr_cmd_->SetParameter(OptionalParameter("error_on_missing_target", 'b', "false"));

        radiological_enable_cmd_->SetGuidance("Overlay radiological backgrounds on top of the selected generator.");
        radiological_enable_cmd_->SetParameterName("enable", true);
        radiological_enable_cmd_->SetDefaultValue(false);

        radiological_window_cmd_->SetGuidance("Time window over which radiological decays are sampled.");
        radiological_window_cmd_->SetParameterName("window", false);
        radiological_window_cmd_->SetUnitCategory("Time");
        radiological_window_cmd_->SetDefaultUnit("us");
        radiological_window_cmd_->SetDefaultValue(10.0);

        radiological_mass_override_cmd_->SetGuidance("Override active-volume mass used to convert Bq/kg to decays/event. Set <= 0 to use geometry/material mass.");
        radiological_mass_override_cmd_->SetParameterName("massKg", false);
        radiological_mass_override_cmd_->SetDefaultValue(0.0);

        radiological_max_decays_cmd_->SetGuidance("Maximum sampled radiological decays per event. Set <= 0 for no cap.");
        radiological_max_decays_cmd_->SetParameterName("maxDecays", false);
        radiological_max_decays_cmd_->SetDefaultValue(10000);

        radiological_clear_cmd_->SetGuidance("Remove all configured radiological isotope activities.");

        radiological_add_isotope_cmd_->SetGuidance("Add an isotope activity: name Z A activity_Bq_per_kg [excitation_MeV].");
        auto* name = new G4UIparameter("name", 's', false);
        auto* z = new G4UIparameter("Z", 'i', false);
        auto* a = new G4UIparameter("A", 'i', false);
        auto* activity = new G4UIparameter("activity_Bq_per_kg", 'd', false);
        auto* excitation = new G4UIparameter("excitation_MeV", 'd', true);
        excitation->SetDefaultValue(0.0);
        radiological_add_isotope_cmd_->SetParameter(name);
        radiological_add_isotope_cmd_->SetParameter(z);
        radiological_add_isotope_cmd_->SetParameter(a);
        radiological_add_isotope_cmd_->SetParameter(activity);
        radiological_add_isotope_cmd_->SetParameter(excitation);

        rock_neutron_enable_cmd_->SetGuidance("Overlay surrounding-rock neutrons on top of the selected generator.");
        rock_neutron_enable_cmd_->SetParameterName("enable", true);
        rock_neutron_enable_cmd_->SetDefaultValue(false);

        rock_neutron_window_cmd_->SetGuidance("Time window over which surrounding-rock neutrons are sampled.");
        rock_neutron_window_cmd_->SetParameterName("window", false);
        rock_neutron_window_cmd_->SetUnitCategory("Time");
        rock_neutron_window_cmd_->SetDefaultUnit("us");
        rock_neutron_window_cmd_->SetDefaultValue(10.0);

        rock_neutron_mean_cmd_->SetGuidance("Override the sampled mean number of rock neutrons per event. Set < 0 to derive it from rate or flux.");
        rock_neutron_mean_cmd_->SetParameterName("mean", false);
        rock_neutron_mean_cmd_->SetDefaultValue(-1.0);

        rock_neutron_flux_cmd_->SetGuidance("Isotropic inward rock-neutron flux through the source shell in cm^-2 s^-1.");
        rock_neutron_flux_cmd_->SetParameterName("fluxPerCm2S", false);
        rock_neutron_flux_cmd_->SetDefaultValue(0.0);

        rock_neutron_rate_cmd_->SetGuidance("Total rock-neutron rate over the source shell in Hz. Overrides flux when > 0.");
        rock_neutron_rate_cmd_->SetParameterName("rateHz", false);
        rock_neutron_rate_cmd_->SetDefaultValue(0.0);

        rock_neutron_max_cmd_->SetGuidance("Maximum sampled rock neutrons per event. Set <= 0 for no cap.");
        rock_neutron_max_cmd_->SetParameterName("maxNeutrons", false);
        rock_neutron_max_cmd_->SetDefaultValue(10000);

        rock_neutron_padding_cmd_->SetGuidance("Distance between active-volume box and the external neutron source shell.");
        rock_neutron_padding_cmd_->SetParameterName("padding", false);
        rock_neutron_padding_cmd_->SetUnitCategory("Length");
        rock_neutron_padding_cmd_->SetDefaultUnit("cm");
        rock_neutron_padding_cmd_->SetDefaultValue(100.0);

        rock_neutron_spectrum_cmd_->SetGuidance("Rock-neutron kinetic-energy spectrum: radiogenic, exponential, flat, or mono.");
        rock_neutron_spectrum_cmd_->SetParameterName("spectrum", false);
        rock_neutron_spectrum_cmd_->SetCandidates("radiogenic exponential flat mono");
        rock_neutron_spectrum_cmd_->SetDefaultValue("radiogenic");

        rock_neutron_energy_range_cmd_->SetGuidance("Rock-neutron kinetic-energy range: min max unit.");
        rock_neutron_energy_range_cmd_->SetParameter(new G4UIparameter("min", 'd', false));
        rock_neutron_energy_range_cmd_->SetParameter(new G4UIparameter("max", 'd', false));
        auto* rock_energy_unit = new G4UIparameter("unit", 's', true);
        rock_energy_unit->SetDefaultValue("MeV");
        rock_neutron_energy_range_cmd_->SetParameter(rock_energy_unit);

        rock_neutron_energy_mean_cmd_->SetGuidance("Mean/scale kinetic energy used by exponential and radiogenic spectra.");
        rock_neutron_energy_mean_cmd_->SetParameterName("energy", false);
        rock_neutron_energy_mean_cmd_->SetUnitCategory("Energy");
        rock_neutron_energy_mean_cmd_->SetDefaultUnit("MeV");
        rock_neutron_energy_mean_cmd_->SetDefaultValue(2.0);

        rock_neutron_direction_cmd_->SetGuidance("Rock-neutron inward direction model: cosine, isotropic, or target.");
        rock_neutron_direction_cmd_->SetParameterName("direction", false);
        rock_neutron_direction_cmd_->SetCandidates("cosine isotropic target");
        rock_neutron_direction_cmd_->SetDefaultValue("cosine");
    }

    GeneratorMessenger::~GeneratorMessenger()
    {
        delete rock_neutron_direction_cmd_;
        delete rock_neutron_energy_mean_cmd_;
        delete rock_neutron_energy_range_cmd_;
        delete rock_neutron_spectrum_cmd_;
        delete rock_neutron_padding_cmd_;
        delete rock_neutron_max_cmd_;
        delete rock_neutron_rate_cmd_;
        delete rock_neutron_flux_cmd_;
        delete rock_neutron_mean_cmd_;
        delete rock_neutron_window_cmd_;
        delete rock_neutron_enable_cmd_;
        delete radiological_add_isotope_cmd_;
        delete radiological_clear_cmd_;
        delete radiological_max_decays_cmd_;
        delete radiological_mass_override_cmd_;
        delete radiological_window_cmd_;
        delete radiological_enable_cmd_;
        delete bxdecay0_mdlr_cmd_;
        delete bxdecay0_mdl_cmd_;
        delete bxdecay0_dbdranged_cmd_;
        delete bxdecay0_dbd_cmd_;
        delete bxdecay0_background_cmd_;
        delete generator_type_cmd_;
        delete genie_file_cmd_;
        delete genie_tree_cmd_;
        delete genie_use_input_vertex_cmd_;
        delete genie_cycle_events_cmd_;
        delete corsika_file_cmd_;
        delete corsika_tree_cmd_;
        delete corsika_cycle_events_cmd_;
        delete marley_config_cmd_;
        delete marley_event_file_cmd_;
        delete marley_cycle_events_cmd_;
        delete marley_seed_cmd_;
        delete rock_neutron_directory_;
        delete radiological_directory_;
        delete bxdecay0_directory_;
        delete marley_directory_;
        delete corsika_directory_;
        delete genie_directory_;
        delete generator_directory_;
    }

    void GeneratorMessenger::SetNewValue(G4UIcommand* command, G4String new_value)
    {
        if (command == generator_type_cmd_)
        {
            if (new_value == "gps")
            {
                config_.mode = GeneratorMode::GPS;
                return;
            }

            if (new_value == "genie_gst")
            {
                config_.mode = GeneratorMode::GenieGST;
                return;
            }

            if (new_value == "genie_pdecay")
            {
                config_.mode = GeneratorMode::GenieProtonDecay;
                return;
            }

            if (new_value == "corsika")
            {
                config_.mode = GeneratorMode::Corsika;
                return;
            }

            if (new_value == "corsika_genie_overlay")
            {
                config_.mode = GeneratorMode::CorsikaGenieOverlay;
                return;
            }

            if (new_value == "marley")
            {
                config_.mode = GeneratorMode::Marley;
                return;
            }

            if (new_value == "bxdecay0")
            {
                config_.mode = GeneratorMode::BxDecay0;
                return;
            }

            if (new_value == "radiological")
            {
                config_.mode = GeneratorMode::Radiological;
                config_.radiological_enabled = true;
                return;
            }

            if (new_value == "rock_neutrons")
            {
                config_.mode = GeneratorMode::RockNeutrons;
                config_.rock_neutrons.enabled = true;
                return;
            }

            G4ExceptionDescription description;
            description << "Unsupported generator type: " << new_value;
            G4Exception("GeneratorMessenger::SetNewValue",
                        "G4LArBoxGeneratorCommand",
                        FatalException,
                        description);
            return;
        }

        if (command == genie_file_cmd_)
        {
            config_.genie_file_path = new_value;
            return;
        }

        if (command == genie_tree_cmd_)
        {
            config_.genie_tree_name = new_value;
            return;
        }

        if (command == genie_use_input_vertex_cmd_)
        {
            config_.genie_use_input_vertex = genie_use_input_vertex_cmd_->GetNewBoolValue(new_value);
            return;
        }

        if (command == genie_cycle_events_cmd_)
        {
            config_.genie_cycle_events = genie_cycle_events_cmd_->GetNewBoolValue(new_value);
            return;
        }

        if (command == corsika_file_cmd_)
        {
            config_.corsika_file_path = new_value;
            return;
        }

        if (command == corsika_tree_cmd_)
        {
            config_.corsika_tree_name = new_value;
            return;
        }

        if (command == corsika_cycle_events_cmd_)
        {
            config_.corsika_cycle_events = corsika_cycle_events_cmd_->GetNewBoolValue(new_value);
            return;
        }

        if (command == marley_config_cmd_)
        {
            config_.marley_config_path = new_value;
            return;
        }

        if (command == marley_event_file_cmd_)
        {
            config_.marley_event_file_path = new_value;
            return;
        }

        if (command == marley_cycle_events_cmd_)
        {
            config_.marley_cycle_events = marley_cycle_events_cmd_->GetNewBoolValue(new_value);
            return;
        }

        if (command == marley_seed_cmd_)
        {
            config_.marley_seed = static_cast<unsigned long>(marley_seed_cmd_->GetNewIntValue(new_value));
            config_.marley_has_seed = true;
            return;
        }

        if (command == bxdecay0_background_cmd_)
        {
            G4Tokenizer tokenizer(new_value);
            const std::string nuclide = tokenizer();
            const std::string seed_text = tokenizer();
            const std::string debug_text = tokenizer();
            if (nuclide.empty() || seed_text.empty())
            {
                G4Exception("GeneratorMessenger::SetNewValue",
                            "G4LArBoxBxDecay0Command",
                            FatalException,
                            "Expected: /generator/bxdecay0/background nuclide seed [debug]");
                return;
            }

            config_.mode = GeneratorMode::BxDecay0;
            config_.bxdecay0.category = "background";
            config_.bxdecay0.nuclide = nuclide;
            config_.bxdecay0.seed = std::atoi(seed_text.c_str());
            config_.bxdecay0.debug = ParseBoolToken(debug_text, false);
            config_.bxdecay0.dbd_mode = 0;
            config_.bxdecay0.dbd_level = 0;
            config_.bxdecay0.dbd_min_energy_mev = -1.0;
            config_.bxdecay0.dbd_max_energy_mev = -1.0;
            return;
        }

        if (command == bxdecay0_dbd_cmd_ || command == bxdecay0_dbdranged_cmd_)
        {
            G4Tokenizer tokenizer(new_value);
            const std::string nuclide = tokenizer();
            const std::string seed_text = tokenizer();
            const std::string mode_text = tokenizer();
            const std::string level_text = tokenizer();
            std::string min_text;
            std::string max_text;
            if (command == bxdecay0_dbdranged_cmd_)
            {
                min_text = tokenizer();
                max_text = tokenizer();
            }
            const std::string debug_text = tokenizer();
            if (nuclide.empty() || seed_text.empty() ||
                mode_text.empty() || level_text.empty() ||
                (command == bxdecay0_dbdranged_cmd_ && (min_text.empty() || max_text.empty())))
            {
                G4Exception("GeneratorMessenger::SetNewValue",
                            "G4LArBoxBxDecay0Command",
                            FatalException,
                            command == bxdecay0_dbdranged_cmd_
                                ? "Expected: /generator/bxdecay0/dbdranged nuclide seed dbd_mode dbd_level min_MeV max_MeV [debug]"
                                : "Expected: /generator/bxdecay0/dbd nuclide seed dbd_mode dbd_level [debug]");
                return;
            }

            config_.mode = GeneratorMode::BxDecay0;
            config_.bxdecay0.category = "dbd";
            config_.bxdecay0.nuclide = nuclide;
            config_.bxdecay0.seed = std::atoi(seed_text.c_str());
            config_.bxdecay0.dbd_mode = std::atoi(mode_text.c_str());
            config_.bxdecay0.dbd_level = std::atoi(level_text.c_str());
            config_.bxdecay0.dbd_min_energy_mev =
                min_text.empty() ? -1.0 : std::strtod(min_text.c_str(), nullptr);
            config_.bxdecay0.dbd_max_energy_mev =
                max_text.empty() ? -1.0 : std::strtod(max_text.c_str(), nullptr);
            config_.bxdecay0.debug = ParseBoolToken(debug_text, false);
            return;
        }

        if (command == bxdecay0_mdl_cmd_ || command == bxdecay0_mdlr_cmd_)
        {
            G4Tokenizer tokenizer(new_value);
            const std::string target = tokenizer();
            const std::string rank_text = tokenizer();
            const std::string longitude_text = tokenizer();
            const std::string colatitude_text = tokenizer();
            const std::string aperture_text = tokenizer();
            std::string aperture2_text;
            if (command == bxdecay0_mdlr_cmd_)
            {
                aperture2_text = tokenizer();
            }
            const std::string error_text = tokenizer();

            config_.bxdecay0.use_mdl = true;
            config_.bxdecay0.mdl_target_name = target.empty() ? "all" : target;
            config_.bxdecay0.mdl_target_rank = rank_text.empty() ? -1 : std::atoi(rank_text.c_str());
            config_.bxdecay0.mdl_cone_longitude_deg =
                longitude_text.empty() ? 0.0 : std::strtod(longitude_text.c_str(), nullptr);
            config_.bxdecay0.mdl_cone_colatitude_deg =
                colatitude_text.empty() ? 0.0 : std::strtod(colatitude_text.c_str(), nullptr);
            config_.bxdecay0.mdl_cone_aperture_deg =
                aperture_text.empty() ? 0.0 : std::strtod(aperture_text.c_str(), nullptr);
            config_.bxdecay0.mdl_cone_aperture2_deg =
                command == bxdecay0_mdlr_cmd_
                    ? (aperture2_text.empty() ? 0.0 : std::strtod(aperture2_text.c_str(), nullptr))
                    : -1.0;
            config_.bxdecay0.mdl_error_on_missing_particle =
                ParseBoolToken(error_text, false);
            return;
        }

        if (command == radiological_enable_cmd_)
        {
            config_.radiological_enabled = radiological_enable_cmd_->GetNewBoolValue(new_value);
            return;
        }

        if (command == radiological_window_cmd_)
        {
            config_.radiological_window_us =
                radiological_window_cmd_->GetNewDoubleValue(new_value) / microsecond;
            return;
        }

        if (command == radiological_mass_override_cmd_)
        {
            config_.radiological_mass_override_kg =
                radiological_mass_override_cmd_->GetNewDoubleValue(new_value);
            return;
        }

        if (command == radiological_max_decays_cmd_)
        {
            config_.radiological_max_decays_per_event =
                radiological_max_decays_cmd_->GetNewIntValue(new_value);
            return;
        }

        if (command == radiological_clear_cmd_)
        {
            config_.radiological_isotopes.clear();
            return;
        }

        if (command == radiological_add_isotope_cmd_)
        {
            G4Tokenizer tokenizer(new_value);
            const std::string isotope_name = tokenizer();
            const std::string z_text = tokenizer();
            const std::string a_text = tokenizer();
            const std::string activity_text = tokenizer();
            const std::string excitation_text = tokenizer();

            if (isotope_name.empty() || z_text.empty() || a_text.empty() || activity_text.empty())
            {
                G4ExceptionDescription description;
                description << "Expected: /generator/radiological/addIsotope name Z A activity_Bq_per_kg [excitation_MeV]";
                G4Exception("GeneratorMessenger::SetNewValue",
                            "G4LArBoxRadiologicalCommand",
                            FatalException,
                            description);
                return;
            }

            RadiologicalIsotopeConfig isotope;
            isotope.name = isotope_name;
            isotope.z = std::atoi(z_text.c_str());
            isotope.a = std::atoi(a_text.c_str());
            isotope.activity_bq_per_kg = std::strtod(activity_text.c_str(), nullptr);
            isotope.excitation_mev = excitation_text.empty()
                                         ? 0.0
                                         : std::strtod(excitation_text.c_str(), nullptr);
            config_.radiological_isotopes.push_back(isotope);
            return;
        }

        if (command == rock_neutron_enable_cmd_)
        {
            config_.rock_neutrons.enabled = rock_neutron_enable_cmd_->GetNewBoolValue(new_value);
            return;
        }

        if (command == rock_neutron_window_cmd_)
        {
            config_.rock_neutrons.window_us =
                rock_neutron_window_cmd_->GetNewDoubleValue(new_value) / microsecond;
            return;
        }

        if (command == rock_neutron_mean_cmd_)
        {
            config_.rock_neutrons.mean_per_event =
                rock_neutron_mean_cmd_->GetNewDoubleValue(new_value);
            return;
        }

        if (command == rock_neutron_flux_cmd_)
        {
            config_.rock_neutrons.flux_per_cm2_s =
                rock_neutron_flux_cmd_->GetNewDoubleValue(new_value);
            return;
        }

        if (command == rock_neutron_rate_cmd_)
        {
            config_.rock_neutrons.rate_hz =
                rock_neutron_rate_cmd_->GetNewDoubleValue(new_value);
            return;
        }

        if (command == rock_neutron_max_cmd_)
        {
            config_.rock_neutrons.max_neutrons_per_event =
                rock_neutron_max_cmd_->GetNewIntValue(new_value);
            return;
        }

        if (command == rock_neutron_padding_cmd_)
        {
            config_.rock_neutrons.shell_padding_cm =
                rock_neutron_padding_cmd_->GetNewDoubleValue(new_value) / cm;
            return;
        }

        if (command == rock_neutron_spectrum_cmd_)
        {
            config_.rock_neutrons.spectrum = new_value;
            return;
        }

        if (command == rock_neutron_energy_range_cmd_)
        {
            G4Tokenizer tokenizer(new_value);
            const std::string min_text = tokenizer();
            const std::string max_text = tokenizer();
            const std::string unit_text = tokenizer();
            if (min_text.empty() || max_text.empty())
            {
                G4Exception("GeneratorMessenger::SetNewValue",
                            "G4LArBoxRockNeutronCommand",
                            FatalException,
                            "Expected: /generator/rockNeutrons/energyRange min max [unit]");
                return;
            }

            const double unit = unit_text.empty()
                                    ? MeV
                                    : G4UIcommand::ValueOf(unit_text.c_str());
            config_.rock_neutrons.energy_min_mev =
                std::strtod(min_text.c_str(), nullptr) * unit / MeV;
            config_.rock_neutrons.energy_max_mev =
                std::strtod(max_text.c_str(), nullptr) * unit / MeV;
            return;
        }

        if (command == rock_neutron_energy_mean_cmd_)
        {
            config_.rock_neutrons.energy_mean_mev =
                rock_neutron_energy_mean_cmd_->GetNewDoubleValue(new_value) / MeV;
            return;
        }

        if (command == rock_neutron_direction_cmd_)
        {
            config_.rock_neutrons.direction_model = new_value;
            return;
        }
    }
}

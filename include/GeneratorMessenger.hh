#ifndef GENERATORMESSENGER_HH
#define GENERATORMESSENGER_HH

#include "GeneratorConfig.hh"

#include "G4UImessenger.hh"

class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithABool;
class G4UIcmdWithAnInteger;
class G4UIcmdWithADouble;
class G4UIcmdWithADoubleAndUnit;
class G4UIcommand;
class G4String;

namespace G4LArBox
{
    class GeneratorMessenger : public G4UImessenger
    {
    public:
        explicit GeneratorMessenger(GeneratorConfig& config);
        ~GeneratorMessenger() override;

        void SetNewValue(G4UIcommand* command, G4String new_value) override;

    private:
        GeneratorConfig& config_;

        G4UIdirectory* generator_directory_;
        G4UIdirectory* genie_directory_;
        G4UIdirectory* corsika_directory_;
        G4UIdirectory* marley_directory_;
        G4UIdirectory* bxdecay0_directory_;
        G4UIdirectory* radiological_directory_;
        G4UIdirectory* rock_neutron_directory_;
        G4UIcmdWithAString* generator_type_cmd_;
        G4UIcmdWithAString* genie_file_cmd_;
        G4UIcmdWithAString* genie_tree_cmd_;
        G4UIcmdWithABool* genie_use_input_vertex_cmd_;
        G4UIcmdWithABool* genie_cycle_events_cmd_;
        G4UIcmdWithAString* corsika_file_cmd_;
        G4UIcmdWithAString* corsika_tree_cmd_;
        G4UIcmdWithABool* corsika_cycle_events_cmd_;
        G4UIcmdWithAString* marley_config_cmd_;
        G4UIcmdWithAString* marley_event_file_cmd_;
        G4UIcmdWithABool* marley_cycle_events_cmd_;
        G4UIcmdWithAnInteger* marley_seed_cmd_;
        G4UIcmdWithAnInteger* bxdecay0_verbosity_cmd_;
        G4UIcommand* bxdecay0_background_cmd_;
        G4UIcommand* bxdecay0_dbd_cmd_;
        G4UIcommand* bxdecay0_dbdranged_cmd_;
        G4UIcommand* bxdecay0_mdl_cmd_;
        G4UIcommand* bxdecay0_mdlr_cmd_;
        G4UIcmdWithABool* radiological_enable_cmd_;
        G4UIcmdWithADoubleAndUnit* radiological_window_cmd_;
        G4UIcmdWithADouble* radiological_mass_override_cmd_;
        G4UIcmdWithAnInteger* radiological_max_decays_cmd_;
        G4UIcommand* radiological_clear_cmd_;
        G4UIcommand* radiological_add_isotope_cmd_;
        G4UIcmdWithABool* rock_neutron_enable_cmd_;
        G4UIcmdWithADoubleAndUnit* rock_neutron_window_cmd_;
        G4UIcmdWithADouble* rock_neutron_mean_cmd_;
        G4UIcmdWithADouble* rock_neutron_flux_cmd_;
        G4UIcmdWithADouble* rock_neutron_rate_cmd_;
        G4UIcmdWithAnInteger* rock_neutron_max_cmd_;
        G4UIcmdWithADoubleAndUnit* rock_neutron_padding_cmd_;
        G4UIcmdWithAString* rock_neutron_spectrum_cmd_;
        G4UIcommand* rock_neutron_energy_range_cmd_;
        G4UIcmdWithADoubleAndUnit* rock_neutron_energy_mean_cmd_;
        G4UIcmdWithAString* rock_neutron_direction_cmd_;
    };
}

#endif // GENERATORMESSENGER_HH

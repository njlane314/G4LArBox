#ifndef GENERATORTRUTH_HH
#define GENERATORTRUTH_HH

#include <string>
#include <vector>

namespace G4LArBox
{
    struct GeneratorTruthRecord
    {
        std::string source = "unknown";

        double vertex_x = 0.0;
        double vertex_y = 0.0;
        double vertex_z = 0.0;
        double vertex_t = 0.0;
        bool has_incident_direction = false;
        double incident_dir_x = 0.0;
        double incident_dir_y = 0.0;
        double incident_dir_z = 0.0;

        int genie_iev = -1;
        int genie_neu = 0;
        int genie_tgt = 0;
        int genie_target_z = 0;
        int genie_target_a = 0;
        bool genie_cc = false;
        bool genie_nc = false;
        bool genie_qel = false;
        bool genie_res = false;
        bool genie_dis = false;
        bool genie_coh = false;
        bool genie_nuel = false;
        bool genie_imd = false;
        bool genie_em = false;
        double genie_weight = 1.0;
        double genie_xs = 0.0;
        double genie_ev = 0.0;
        double genie_input_vtxx = 0.0;
        double genie_input_vtxy = 0.0;
        double genie_input_vtxz = 0.0;
        double genie_input_vtxt = 0.0;

        int marley_event = -1;
        double marley_flux_averaged_xsec = 0.0;
        int marley_projectile_pdg = 0;
        int marley_target_pdg = 0;
        int marley_ejectile_pdg = 0;
        int marley_residue_pdg = 0;
        double marley_ex = 0.0;

        std::string bxdecay0_category;
        std::string bxdecay0_nuclide;
        int bxdecay0_seed = 0;
        int bxdecay0_event = -1;
        int bxdecay0_particles = 0;
        double bxdecay0_event_time_ns = 0.0;
        std::vector<int> bxdecay0_pdg;
        std::vector<double> bxdecay0_px_mev;
        std::vector<double> bxdecay0_py_mev;
        std::vector<double> bxdecay0_pz_mev;
        std::vector<double> bxdecay0_time_ns;

        bool radiological_enabled = false;
        double radiological_mass_kg = 0.0;
        double radiological_window_us = 0.0;
        double radiological_expected_decays = 0.0;
        int radiological_decays = 0;
        std::vector<std::string> radiological_isotope;
        std::vector<int> radiological_z;
        std::vector<int> radiological_a;
        std::vector<double> radiological_activity_bq_per_kg;
        std::vector<double> radiological_decay_time_ns;

        bool rock_neutron_enabled = false;
        double rock_neutron_window_us = 0.0;
        double rock_neutron_expected = 0.0;
        int rock_neutron_count = 0;
        std::vector<double> rock_neutron_x;
        std::vector<double> rock_neutron_y;
        std::vector<double> rock_neutron_z;
        std::vector<double> rock_neutron_time_ns;
        std::vector<double> rock_neutron_energy_mev;
        std::vector<double> rock_neutron_dir_x;
        std::vector<double> rock_neutron_dir_y;
        std::vector<double> rock_neutron_dir_z;
        std::vector<int> rock_neutron_face;

        std::vector<int> primary_pdg;
        std::vector<double> primary_energy;
        std::vector<double> primary_px;
        std::vector<double> primary_py;
        std::vector<double> primary_pz;
    };
}

#endif // GENERATORTRUTH_HH

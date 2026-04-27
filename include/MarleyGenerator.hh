#ifndef MARLEYGENERATOR_HH
#define MARLEYGENERATOR_HH

#include <memory>
#include <string>
#include <vector>

namespace marley
{
    class Generator;
    class RootEventFileReader;
    class Event;
}

namespace G4LArBox
{
    struct MarleyEventRecord
    {
        int event_index = -1;
        double flux_averaged_xsec = 0.0;
        int projectile_pdg = 0;
        double projectile_px_mev = 0.0;
        double projectile_py_mev = 0.0;
        double projectile_pz_mev = 0.0;
        int target_pdg = 0;
        int ejectile_pdg = 0;
        int residue_pdg = 0;
        double excitation_energy_mev = 0.0;
        std::vector<int> pdg;
        std::vector<int> charge;
        std::vector<double> total_energy_mev;
        std::vector<double> px_mev;
        std::vector<double> py_mev;
        std::vector<double> pz_mev;
        std::vector<double> cascade_level_energy_mev;
    };

    class MarleyGenerator
    {
    public:
        MarleyGenerator();
        ~MarleyGenerator();

        void Configure(const std::string& config_path,
                       const std::string& event_file_path,
                       bool cycle_events,
                       unsigned long seed,
                       bool has_seed);
        MarleyEventRecord ReadNext();

    private:
        void Close();
        void EnsureOpen();
        MarleyEventRecord ConvertEvent(const marley::Event& event) const;

        std::string config_path_;
        std::string event_file_path_;
        bool cycle_events_;
        unsigned long seed_;
        bool has_seed_;
        long long event_index_;
        std::unique_ptr<marley::Generator> generator_;
        std::unique_ptr<marley::RootEventFileReader> event_reader_;
    };
}

#endif // MARLEYGENERATOR_HH

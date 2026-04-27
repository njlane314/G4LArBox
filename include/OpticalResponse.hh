#ifndef OPTICALRESPONSE_HH
#define OPTICALRESPONSE_HH

#include "PhotonLibrary.hh"

#include <random>
#include <string>
#include <vector>

namespace G4LArBox
{
    class OpticalResponse
    {
    public:
        enum class Component
        {
            Direct,
            Diffuse,
            PhotonLibrary
        };

        enum class Model
        {
            LegacyPanel,
            DUNEVDNode,
            DUNEVDPhotonLibrary,
            DUNEVDHybrid
        };

        struct Config
        {
            Model model = Model::LegacyPanel;

            double detector_z_negative_mm = -483.478;
            double detector_z_positive_mm = 483.478;
            double detector_half_x_mm = 231.3135;
            double detector_half_y_mm = 1497.995;
            int panels_per_side = 10;
            int sensors_per_panel = 6;

            double collection_efficiency = 0.03;
            double diffuse_scale = 0.25;
            double absorption_length_mm = 20000.0;
            double rayleigh_length_mm = 550.0;
            double refractive_index = 1.23;
            double scintillation_wavelength_nm = 127.0;
            double detected_wavelength_nm = 430.0;

            double singlet_fraction = 0.25;
            double singlet_lifetime_ns = 6.0;
            double triplet_lifetime_ns = 1300.0;
            double diffuse_lognormal_sigma = 0.45;

            unsigned int seed = 271828;

            PhotonLibrary::Config photon_library;
            double photon_library_efficiency = 0.027;

            int dunevd_node_strings_y = 8;
            int dunevd_node_strings_z = 39;
            int dunevd_node_count_x = 8;
            int dunevd_node_channel_offset = -1;
            double dunevd_node_pitch_mm = 1500.0;
            double dunevd_node_central_gap_mm = 300.0;
            double dunevd_node_effective_area_mm2 = 100.0;
            double dunevd_node_x_offset_mm = 0.0;
            double dunevd_node_y_offset_mm = 0.0;
            double dunevd_node_z_offset_mm = 0.0;
            double dunevd_node_wavelength_nm = 430.0;
        };

        struct StepDeposit
        {
            double x_mm = 0.0;
            double y_mm = 0.0;
            double z_mm = 0.0;
            double time_ns = 0.0;
            int photons = 0;
        };

        struct Hit
        {
            int channel = -1;
            double x_mm = 0.0;
            double y_mm = 0.0;
            double z_mm = 0.0;
            double time_ns = 0.0;
            double wavelength_nm = 0.0;
            Component component = Component::Direct;
            std::string label;
        };

        OpticalResponse();
        explicit OpticalResponse(const Config& config);
        ~OpticalResponse();

        std::vector<Hit> GenerateHits(const StepDeposit& deposit);
        int ChannelCount() const;
        bool PhotonLibraryLoaded() const;
        std::string ChannelReadout(int channel) const;

    private:
        enum class NormalAxis
        {
            X,
            Y,
            Z
        };

        enum class ChannelKind
        {
            LegacyPanel,
            FastDPSUNode
        };

        struct ChannelPatch
        {
            int channel = -1;
            double center_x_mm = 0.0;
            double center_y_mm = 0.0;
            double center_z_mm = 0.0;
            double half_u_mm = 0.0;
            double half_v_mm = 0.0;
            double effective_area_mm2 = 0.0;
            double efficiency = 1.0;
            double wavelength_nm = 430.0;
            NormalAxis normal_axis = NormalAxis::Z;
            ChannelKind kind = ChannelKind::LegacyPanel;
            bool omnidirectional = false;
        };

        Config config_;
        std::vector<ChannelPatch> patches_;
        PhotonLibrary photon_library_;
        std::mt19937 rng_;
        std::uniform_real_distribution<double> uniform_;

        void BuildPatches();
        void BuildLegacyPanelPatches();
        void BuildDUNEVDNodePatches();
        double SolidAngle(const StepDeposit& deposit, const ChannelPatch& patch) const;
        double DistanceToPatch(const StepDeposit& deposit, const ChannelPatch& patch) const;
        double Attenuation(double distance_mm) const;
        double DirectExpectation(const StepDeposit& deposit, const ChannelPatch& patch) const;
        double DiffuseExpectation(const StepDeposit& deposit,
                                  const ChannelPatch& patch,
                                  double unscattered_survival) const;
        std::vector<Hit> GenerateGeometricHits(const StepDeposit& deposit);
        std::vector<Hit> GeneratePhotonLibraryHits(const StepDeposit& deposit);
        double SampleEmissionDelay();
        double DirectTransportTime(double distance_mm) const;
        double SampleDiffuseTransportTime(double distance_mm);
        void AddHits(int count,
                     const StepDeposit& deposit,
                     const ChannelPatch& patch,
                     Component component,
                     std::vector<Hit>& hits);
        void AddPhotonLibraryHits(int count,
                                  int channel,
                                  const StepDeposit& deposit,
                                  std::vector<Hit>& hits);
    };
}

#endif // OPTICALRESPONSE_HH

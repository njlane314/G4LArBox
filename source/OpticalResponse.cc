#include "OpticalResponse.hh"

#include <algorithm>
#include <cmath>

namespace G4LArBox
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double kFourPi = 4.0 * kPi;
        constexpr double kSpeedOfLightMmPerNs = 299.792458;

        std::vector<double> CenteredPositions(int count, double pitch_mm, double offset_mm)
        {
            std::vector<double> positions;
            if (count <= 0)
            {
                return positions;
            }

            positions.reserve(count);
            const double center = 0.5 * (count - 1);
            for (int index = 0; index < count; ++index)
            {
                positions.push_back((index - center) * pitch_mm + offset_mm);
            }
            return positions;
        }

        std::vector<double> SplitNodePositions(int count,
                                               double pitch_mm,
                                               double central_gap_mm,
                                               double offset_mm)
        {
            std::vector<double> positions;
            if (count <= 0)
            {
                return positions;
            }

            positions.reserve(count);
            if (count == 1)
            {
                positions.push_back(offset_mm);
                return positions;
            }

            const int lower_count = count / 2;
            const int upper_count = count - lower_count;
            for (int index = lower_count - 1; index >= 0; --index)
            {
                positions.push_back(offset_mm -
                                    (0.5 * central_gap_mm + 0.5 * pitch_mm + index * pitch_mm));
            }
            for (int index = 0; index < upper_count; ++index)
            {
                positions.push_back(offset_mm +
                                    (0.5 * central_gap_mm + 0.5 * pitch_mm + index * pitch_mm));
            }
            return positions;
        }
    }

    OpticalResponse::OpticalResponse()
        : OpticalResponse(Config())
    {}

    OpticalResponse::OpticalResponse(const Config& config)
        : config_(config),
          photon_library_(config.photon_library),
          rng_(config.seed),
          uniform_(0.0, 1.0)
    {
        if ((config_.model == Model::DUNEVDPhotonLibrary ||
             config_.model == Model::DUNEVDHybrid) &&
            !config_.photon_library.file_path.empty())
        {
            photon_library_.Load();
        }
        BuildPatches();
    }

    OpticalResponse::~OpticalResponse()
    {}

    std::vector<OpticalResponse::Hit>
    OpticalResponse::GenerateHits(const StepDeposit& deposit)
    {
        std::vector<Hit> hits;
        if (deposit.photons <= 0)
        {
            return hits;
        }

        if (config_.model == Model::LegacyPanel || config_.model == Model::DUNEVDNode ||
            config_.model == Model::DUNEVDHybrid)
        {
            auto geometric_hits = GenerateGeometricHits(deposit);
            hits.insert(hits.end(), geometric_hits.begin(), geometric_hits.end());
        }

        if (config_.model == Model::DUNEVDPhotonLibrary || config_.model == Model::DUNEVDHybrid)
        {
            auto library_hits = GeneratePhotonLibraryHits(deposit);
            hits.insert(hits.end(), library_hits.begin(), library_hits.end());
        }

        return hits;
    }

    int OpticalResponse::ChannelCount() const
    {
        int count = 0;
        for (const auto& patch : patches_)
        {
            count = std::max(count, patch.channel + 1);
        }
        if (photon_library_.IsLoaded())
        {
            count = std::max(count, photon_library_.ChannelCount());
        }
        return count;
    }

    bool OpticalResponse::PhotonLibraryLoaded() const
    {
        return photon_library_.IsLoaded();
    }

    std::string OpticalResponse::ChannelReadout(int channel) const
    {
        if (channel < 0)
        {
            return "invalid";
        }

        for (const auto& patch : patches_)
        {
            if (patch.channel != channel)
            {
                continue;
            }

            return patch.kind == ChannelKind::FastDPSUNode
                       ? "node"
                       : "legacy";
        }

        if ((config_.model == Model::DUNEVDPhotonLibrary ||
             config_.model == Model::DUNEVDHybrid) &&
            photon_library_.IsLoaded() &&
            channel < photon_library_.ChannelCount())
        {
            return "arapuca";
        }

        return "unknown";
    }

    void OpticalResponse::BuildPatches()
    {
        patches_.clear();
        if (config_.model == Model::LegacyPanel)
        {
            BuildLegacyPanelPatches();
        }
        else if (config_.model == Model::DUNEVDNode || config_.model == Model::DUNEVDHybrid)
        {
            BuildDUNEVDNodePatches();
        }
    }

    void OpticalResponse::BuildLegacyPanelPatches()
    {
        const int panels = std::max(1, config_.panels_per_side);
        const int sensors = std::max(1, config_.sensors_per_panel);
        const double panel_height = (2.0 * config_.detector_half_y_mm) / panels;
        const double sensor_height = panel_height / sensors;

        auto add_side = [&](int side, double z_mm) {
            const int side_offset = side * panels * sensors;
            for (int panel = 0; panel < panels; ++panel)
            {
                const double panel_y_min = -config_.detector_half_y_mm + panel * panel_height;
                for (int sensor = 0; sensor < sensors; ++sensor)
                {
                    ChannelPatch patch;
                    patch.channel = side_offset + panel * sensors + sensor;
                    patch.center_x_mm = 0.0;
                    patch.center_y_mm = panel_y_min + (sensor + 0.5) * sensor_height;
                    patch.center_z_mm = z_mm;
                    patch.half_u_mm = config_.detector_half_x_mm;
                    patch.half_v_mm = 0.5 * sensor_height;
                    patch.effective_area_mm2 = 4.0 * patch.half_u_mm * patch.half_v_mm;
                    patch.efficiency = config_.collection_efficiency;
                    patch.wavelength_nm = config_.detected_wavelength_nm;
                    patch.normal_axis = NormalAxis::Z;
                    patch.kind = ChannelKind::LegacyPanel;
                    patches_.push_back(patch);
                }
            }
        };

        add_side(0, config_.detector_z_negative_mm);
        add_side(1, config_.detector_z_positive_mm);
    }

    void OpticalResponse::BuildDUNEVDNodePatches()
    {
        const auto xs = SplitNodePositions(config_.dunevd_node_count_x,
                                           config_.dunevd_node_pitch_mm,
                                           config_.dunevd_node_central_gap_mm,
                                           config_.dunevd_node_x_offset_mm);
        const auto ys = CenteredPositions(config_.dunevd_node_strings_y,
                                          config_.dunevd_node_pitch_mm,
                                          config_.dunevd_node_y_offset_mm);
        const auto zs = CenteredPositions(config_.dunevd_node_strings_z,
                                          config_.dunevd_node_pitch_mm,
                                          config_.dunevd_node_z_offset_mm);

        const double half_side = 0.5 * std::sqrt(std::max(0.0, config_.dunevd_node_effective_area_mm2));
        int channel = config_.dunevd_node_channel_offset >= 0
                          ? config_.dunevd_node_channel_offset
                          : ((config_.model == Model::DUNEVDHybrid && photon_library_.IsLoaded())
                                 ? photon_library_.ChannelCount()
                                 : 0);
        for (double z : zs)
        {
            for (double y : ys)
            {
                for (double x : xs)
                {
                    ChannelPatch patch;
                    patch.channel = channel++;
                    patch.center_x_mm = x;
                    patch.center_y_mm = y;
                    patch.center_z_mm = z;
                    patch.half_u_mm = half_side;
                    patch.half_v_mm = half_side;
                    patch.effective_area_mm2 = config_.dunevd_node_effective_area_mm2;
                    patch.efficiency = config_.collection_efficiency;
                    patch.wavelength_nm = config_.dunevd_node_wavelength_nm;
                    patch.normal_axis = NormalAxis::X;
                    patch.kind = ChannelKind::FastDPSUNode;
                    patch.omnidirectional = true;
                    patches_.push_back(patch);
                }
            }
        }
    }

    std::vector<OpticalResponse::Hit>
    OpticalResponse::GenerateGeometricHits(const StepDeposit& deposit)
    {
        std::vector<Hit> hits;
        for (const auto& patch : patches_)
        {
            const double distance = DistanceToPatch(deposit, patch);
            if (distance <= 0.0)
            {
                continue;
            }

            const double direct_mean = DirectExpectation(deposit, patch);
            if (direct_mean > 0.0)
            {
                std::poisson_distribution<int> direct_count(direct_mean);
                AddHits(direct_count(rng_), deposit, patch, Component::Direct, hits);
            }

            const double unscattered_survival =
                std::exp(-distance / std::max(config_.rayleigh_length_mm, 1.0e-9));
            const double diffuse_mean = DiffuseExpectation(deposit, patch, unscattered_survival);
            if (diffuse_mean > 0.0)
            {
                std::poisson_distribution<int> diffuse_count(diffuse_mean);
                AddHits(diffuse_count(rng_), deposit, patch, Component::Diffuse, hits);
            }
        }

        return hits;
    }

    std::vector<OpticalResponse::Hit>
    OpticalResponse::GeneratePhotonLibraryHits(const StepDeposit& deposit)
    {
        std::vector<Hit> hits;
        const auto visibilities = photon_library_.Visibilities(deposit.x_mm,
                                                               deposit.y_mm,
                                                               deposit.z_mm,
                                                               false);
        if (visibilities.empty())
        {
            return hits;
        }

        for (int channel = 0; channel < static_cast<int>(visibilities.size()); ++channel)
        {
            const double mean = deposit.photons *
                                std::max(0.0, visibilities[channel]) *
                                config_.photon_library_efficiency;
            if (mean <= 0.0)
            {
                continue;
            }

            std::poisson_distribution<int> detected_count(mean);
            AddPhotonLibraryHits(detected_count(rng_), channel, deposit, hits);
        }

        return hits;
    }

    double OpticalResponse::SolidAngle(const StepDeposit& deposit,
                                       const ChannelPatch& patch) const
    {
        if (patch.omnidirectional)
        {
            const double distance = DistanceToPatch(deposit, patch);
            if (distance <= 0.0)
            {
                return 0.0;
            }
            return std::min(kFourPi, patch.effective_area_mm2 / (distance * distance));
        }

        double normal_delta = 0.0;
        double u0 = 0.0;
        double v0 = 0.0;
        if (patch.normal_axis == NormalAxis::X)
        {
            normal_delta = patch.center_x_mm - deposit.x_mm;
            u0 = deposit.y_mm - patch.center_y_mm;
            v0 = deposit.z_mm - patch.center_z_mm;
        }
        else if (patch.normal_axis == NormalAxis::Y)
        {
            normal_delta = patch.center_y_mm - deposit.y_mm;
            u0 = deposit.x_mm - patch.center_x_mm;
            v0 = deposit.z_mm - patch.center_z_mm;
        }
        else
        {
            normal_delta = patch.center_z_mm - deposit.z_mm;
            u0 = deposit.x_mm - patch.center_x_mm;
            v0 = deposit.y_mm - patch.center_y_mm;
        }

        if (std::abs(normal_delta) < 1.0e-9)
        {
            return 0.0;
        }

        const double u_edges[2] = {-patch.half_u_mm - u0, patch.half_u_mm - u0};
        const double v_edges[2] = {-patch.half_v_mm - v0, patch.half_v_mm - v0};

        double omega = 0.0;
        for (int iu = 0; iu < 2; ++iu)
        {
            for (int iv = 0; iv < 2; ++iv)
            {
                const double u = u_edges[iu];
                const double v = v_edges[iv];
                const double r = std::sqrt(u * u + v * v + normal_delta * normal_delta);
                const double term = std::atan2(u * v, normal_delta * r);
                omega += ((iu == iv) ? 1.0 : -1.0) * term;
            }
        }

        return std::min(kFourPi, std::abs(omega));
    }

    double OpticalResponse::DistanceToPatch(const StepDeposit& deposit,
                                            const ChannelPatch& patch) const
    {
        double normal_delta = 0.0;
        double du = 0.0;
        double dv = 0.0;

        if (patch.normal_axis == NormalAxis::X)
        {
            normal_delta = deposit.x_mm - patch.center_x_mm;
            du = deposit.y_mm - std::clamp(deposit.y_mm,
                                           patch.center_y_mm - patch.half_u_mm,
                                           patch.center_y_mm + patch.half_u_mm);
            dv = deposit.z_mm - std::clamp(deposit.z_mm,
                                           patch.center_z_mm - patch.half_v_mm,
                                           patch.center_z_mm + patch.half_v_mm);
        }
        else if (patch.normal_axis == NormalAxis::Y)
        {
            normal_delta = deposit.y_mm - patch.center_y_mm;
            du = deposit.x_mm - std::clamp(deposit.x_mm,
                                           patch.center_x_mm - patch.half_u_mm,
                                           patch.center_x_mm + patch.half_u_mm);
            dv = deposit.z_mm - std::clamp(deposit.z_mm,
                                           patch.center_z_mm - patch.half_v_mm,
                                           patch.center_z_mm + patch.half_v_mm);
        }
        else
        {
            normal_delta = deposit.z_mm - patch.center_z_mm;
            du = deposit.x_mm - std::clamp(deposit.x_mm,
                                           patch.center_x_mm - patch.half_u_mm,
                                           patch.center_x_mm + patch.half_u_mm);
            dv = deposit.y_mm - std::clamp(deposit.y_mm,
                                           patch.center_y_mm - patch.half_v_mm,
                                           patch.center_y_mm + patch.half_v_mm);
        }

        return std::sqrt(normal_delta * normal_delta + du * du + dv * dv);
    }

    double OpticalResponse::Attenuation(double distance_mm) const
    {
        const double absorption_length = std::max(config_.absorption_length_mm, 1.0e-9);
        return std::exp(-distance_mm / absorption_length);
    }

    double OpticalResponse::DirectExpectation(const StepDeposit& deposit,
                                              const ChannelPatch& patch) const
    {
        const double distance = DistanceToPatch(deposit, patch);
        const double rayleigh_length = std::max(config_.rayleigh_length_mm, 1.0e-9);
        const double unscattered_survival = std::exp(-distance / rayleigh_length);
        return deposit.photons * patch.efficiency *
               (SolidAngle(deposit, patch) / kFourPi) *
               Attenuation(distance) * unscattered_survival;
    }

    double OpticalResponse::DiffuseExpectation(const StepDeposit& deposit,
                                               const ChannelPatch& patch,
                                               double unscattered_survival) const
    {
        const double distance = DistanceToPatch(deposit, patch);
        return deposit.photons * patch.efficiency * config_.diffuse_scale *
               (SolidAngle(deposit, patch) / kFourPi) *
               Attenuation(distance) * (1.0 - unscattered_survival);
    }

    double OpticalResponse::SampleEmissionDelay()
    {
        const bool singlet = uniform_(rng_) < std::clamp(config_.singlet_fraction, 0.0, 1.0);
        const double lifetime = singlet ? config_.singlet_lifetime_ns : config_.triplet_lifetime_ns;
        std::exponential_distribution<double> scint_time(1.0 / std::max(lifetime, 1.0e-9));
        return scint_time(rng_);
    }

    double OpticalResponse::DirectTransportTime(double distance_mm) const
    {
        return distance_mm * config_.refractive_index / kSpeedOfLightMmPerNs;
    }

    double OpticalResponse::SampleDiffuseTransportTime(double distance_mm)
    {
        const double photon_speed = kSpeedOfLightMmPerNs / std::max(config_.refractive_index, 1.0e-9);
        const double diffusion = photon_speed * std::max(config_.rayleigh_length_mm, 1.0e-9) / 3.0;
        const double mean_time = DirectTransportTime(distance_mm) +
                                 distance_mm * distance_mm / (6.0 * diffusion);
        const double sigma = std::max(config_.diffuse_lognormal_sigma, 1.0e-6);
        const double mu = std::log(std::max(mean_time, 1.0e-9)) - 0.5 * sigma * sigma;
        std::lognormal_distribution<double> transport_time(mu, sigma);
        return std::max(DirectTransportTime(distance_mm), transport_time(rng_));
    }

    void OpticalResponse::AddHits(int count,
                                  const StepDeposit& deposit,
                                  const ChannelPatch& patch,
                                  Component component,
                                  std::vector<Hit>& hits)
    {
        if (count <= 0)
        {
            return;
        }

        const double distance = DistanceToPatch(deposit, patch);
        for (int i = 0; i < count; ++i)
        {
            Hit hit;
            hit.channel = patch.channel;
            hit.wavelength_nm = patch.wavelength_nm;
            hit.component = component;
            hit.label = patch.kind == ChannelKind::FastDPSUNode
                            ? (component == Component::Direct
                                   ? "fastDPSUNodeDirect"
                                   : "fastDPSUNodeDiffuse")
                            : (component == Component::Direct
                                   ? "fastOpticalDirect"
                                   : "fastOpticalDiffuse");
            hit.time_ns = deposit.time_ns + SampleEmissionDelay();
            hit.time_ns += component == Component::Direct
                             ? DirectTransportTime(distance)
                             : SampleDiffuseTransportTime(distance);

            if (patch.normal_axis == NormalAxis::X)
            {
                hit.x_mm = patch.center_x_mm;
                hit.y_mm = std::clamp(deposit.y_mm,
                                      patch.center_y_mm - patch.half_u_mm,
                                      patch.center_y_mm + patch.half_u_mm);
                hit.z_mm = std::clamp(deposit.z_mm,
                                      patch.center_z_mm - patch.half_v_mm,
                                      patch.center_z_mm + patch.half_v_mm);
            }
            else if (patch.normal_axis == NormalAxis::Y)
            {
                hit.x_mm = std::clamp(deposit.x_mm,
                                      patch.center_x_mm - patch.half_u_mm,
                                      patch.center_x_mm + patch.half_u_mm);
                hit.y_mm = patch.center_y_mm;
                hit.z_mm = std::clamp(deposit.z_mm,
                                      patch.center_z_mm - patch.half_v_mm,
                                      patch.center_z_mm + patch.half_v_mm);
            }
            else
            {
                hit.x_mm = std::clamp(deposit.x_mm,
                                      patch.center_x_mm - patch.half_u_mm,
                                      patch.center_x_mm + patch.half_u_mm);
                hit.y_mm = std::clamp(deposit.y_mm,
                                      patch.center_y_mm - patch.half_v_mm,
                                      patch.center_y_mm + patch.half_v_mm);
                hit.z_mm = patch.center_z_mm;
            }

            hits.push_back(hit);
        }
    }

    void OpticalResponse::AddPhotonLibraryHits(int count,
                                               int channel,
                                               const StepDeposit& deposit,
                                               std::vector<Hit>& hits)
    {
        if (count <= 0)
        {
            return;
        }

        for (int i = 0; i < count; ++i)
        {
            Hit hit;
            hit.channel = channel;
            hit.x_mm = deposit.x_mm;
            hit.y_mm = deposit.y_mm;
            hit.z_mm = deposit.z_mm;
            hit.wavelength_nm = config_.detected_wavelength_nm;
            hit.component = Component::PhotonLibrary;
            hit.label = "dunevdArapucaPhotonLibrary";
            hit.time_ns = deposit.time_ns + SampleEmissionDelay();
            hits.push_back(hit);
        }
    }
}

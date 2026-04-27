#include "PrimaryGeneratorAction.hh"

#include "DataHandler.hh"
#include "GeneratorMessenger.hh"

#include "G4Event.hh"
#include "G4Exception.hh"
#include "G4IonTable.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4ios.hh"

#include "CLHEP/Random/RandExponential.h"
#include "CLHEP/Random/RandPoisson.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace G4LArBox
{
    namespace
    {
        constexpr int kPotassium40ResiduePdg = 1000190400;
        constexpr double kCascadeEnergyToleranceMev = 1.0e-6;

        struct DelayedGammaState
        {
            double energy_mev;
            double half_life;
        };

        struct ActiveBoxBounds
        {
            G4ThreeVector center;
            double hx = 0.0;
            double hy = 0.0;
            double hz = 0.0;
        };

        const std::array<DelayedGammaState, 5> kPotassium40DelayedGammaStates = {{
            {0.0298299, 4.25 * ns},
            {0.800143, 0.26 * ps},
            {1.64364, 0.336 * us},
            {1.95907, 0.54 * ps},
            {2.010368, 0.32 * ps},
        }};

        bool FindDelayedGammaHalfLife(double excitation_energy_mev, double& half_life)
        {
            for (const DelayedGammaState& state : kPotassium40DelayedGammaStates)
            {
                if (std::abs(excitation_energy_mev - state.energy_mev) < kCascadeEnergyToleranceMev)
                {
                    half_life = state.half_life;
                    return true;
                }
            }

            return false;
        }

        double SampleFiniteParticleTime(double half_life)
        {
            return CLHEP::RandExponential::shoot(half_life / std::log(2.0));
        }

        void AppendSourceTag(std::string& source, const std::string& tag)
        {
            if (source.find(tag) == std::string::npos)
            {
                source += "+" + tag;
            }
        }

        ActiveBoxBounds GetActiveBoxBounds()
        {
            const auto* detector_construction = static_cast<const DetectorConstruction*>(
                G4RunManager::GetRunManager()->GetUserDetectorConstruction());
            if (detector_construction == nullptr)
            {
                throw std::runtime_error("Failed to locate detector construction while sampling rock neutrons.");
            }

            G4VPhysicalVolume* active_volume = detector_construction->GetActiveVolume();
            if (active_volume == nullptr || active_volume->GetLogicalVolume() == nullptr)
            {
                throw std::runtime_error("Failed to locate the active detector volume while sampling rock neutrons.");
            }

            auto* box = dynamic_cast<G4Box*>(active_volume->GetLogicalVolume()->GetSolid());
            if (box == nullptr)
            {
                throw std::runtime_error("Rock-neutron shell sampling requires a box-shaped active detector volume.");
            }

            ActiveBoxBounds bounds;
            bounds.center = active_volume->GetObjectTranslation();
            bounds.hx = box->GetXHalfLength();
            bounds.hy = box->GetYHalfLength();
            bounds.hz = box->GetZHalfLength();
            return bounds;
        }
    }

    PrimaryGeneratorAction::PrimaryGeneratorAction()
        : generalgen_(new G4GeneralParticleSource()),
          generator_messenger_(new GeneratorMessenger(generator_config_)),
          genie_reader_(std::make_unique<GenieGSTReader>()),
          corsika_reader_(std::make_unique<CorsikaReader>()),
          marley_generator_(std::make_unique<MarleyGenerator>()),
          bxdecay0_generator_(std::make_unique<BxDecay0Generator>()),
          bulk_vertex_generator_(std::make_unique<G4LArBox::BulkVertexGenerator>())
    {}

    PrimaryGeneratorAction::~PrimaryGeneratorAction()
    {
        delete generator_messenger_;
        delete generalgen_;
    }

    void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
    {
        try
        {
            switch (generator_config_.mode)
            {
                case GeneratorMode::GPS:
                {
                    generalgen_->GeneratePrimaryVertex(event);

                    GeneratorTruthRecord truth;
                    truth.source = "gps";
                    AddRadiologicalBackgrounds(event, truth);
                    AddRockNeutronBackgrounds(event, truth);
                    CaptureTruthFromEvent(event, truth);
                    DataHandler::Instance()->SetGeneratorTruth(truth);
                    return;
                }

                case GeneratorMode::GenieGST:
                    GenerateGeniePrimaries(event);
                    return;

                case GeneratorMode::GenieProtonDecay:
                    GenerateGenieProtonDecayPrimaries(event);
                    return;

                case GeneratorMode::Corsika:
                    GenerateCorsikaPrimaries(event);
                    return;

                case GeneratorMode::CorsikaGenieOverlay:
                    GenerateCorsikaGenieOverlayPrimaries(event);
                    return;

                case GeneratorMode::Marley:
                    GenerateMarleyPrimaries(event);
                    return;

                case GeneratorMode::BxDecay0:
                    GenerateBxDecay0Primaries(event);
                    return;

                case GeneratorMode::Radiological:
                    GenerateRadiologicalPrimaries(event);
                    return;

                case GeneratorMode::RockNeutrons:
                    GenerateRockNeutronPrimaries(event);
                    return;
            }
        }
        catch (const std::exception& error)
        {
            G4ExceptionDescription description;
            description << error.what();
            G4Exception("PrimaryGeneratorAction::GeneratePrimaries",
                        "G4LArBoxGeneratorError",
                        FatalException,
                        description);
        }
    }

    void PrimaryGeneratorAction::BulkVertexGenerator(G4ThreeVector& vtx)
    {
        vtx = bulk_vertex_generator_->ShootVertex();
    }

    bool PrimaryGeneratorAction::RadiologicalOverlayEnabled() const
    {
        return generator_config_.radiological_enabled ||
               generator_config_.mode == GeneratorMode::Radiological;
    }

    bool PrimaryGeneratorAction::RockNeutronOverlayEnabled() const
    {
        return generator_config_.rock_neutrons.enabled ||
               generator_config_.mode == GeneratorMode::RockNeutrons;
    }

    double PrimaryGeneratorAction::ActiveVolumeMassKg() const
    {
        auto detectorConstruction = static_cast<const DetectorConstruction*>(
            G4RunManager::GetRunManager()->GetUserDetectorConstruction());
        if (detectorConstruction == nullptr)
        {
            return 0.0;
        }

        G4VPhysicalVolume* active_volume = detectorConstruction->GetActiveVolume();
        if (active_volume == nullptr || active_volume->GetLogicalVolume() == nullptr)
        {
            return 0.0;
        }

        const auto* logical = active_volume->GetLogicalVolume();
        if (logical->GetSolid() == nullptr || logical->GetMaterial() == nullptr)
        {
            return 0.0;
        }

        return logical->GetSolid()->GetCubicVolume() *
               logical->GetMaterial()->GetDensity() / kg;
    }

    double PrimaryGeneratorAction::SampleAllowedBetaKineticEnergy(double endpoint_energy) const
    {
        if (endpoint_energy <= 0.0)
        {
            return 0.0;
        }

        constexpr double electron_mass = 0.510998950 * MeV;
        auto weight = [&](double kinetic_energy) {
            if (kinetic_energy <= 0.0 || kinetic_energy >= endpoint_energy)
            {
                return 0.0;
            }
            const double total_energy = kinetic_energy + electron_mass;
            const double momentum = std::sqrt(kinetic_energy * (kinetic_energy + 2.0 * electron_mass));
            const double neutrino_energy = endpoint_energy - kinetic_energy;
            return momentum * total_energy * neutrino_energy * neutrino_energy;
        };

        double max_weight = 0.0;
        for (int i = 1; i < 256; ++i)
        {
            max_weight = std::max(max_weight, weight(endpoint_energy * i / 256.0));
        }
        if (max_weight <= 0.0)
        {
            return 0.0;
        }

        for (int attempt = 0; attempt < 10000; ++attempt)
        {
            const double candidate = CLHEP::RandFlat::shoot(0.0, endpoint_energy);
            if (CLHEP::RandFlat::shoot(0.0, max_weight) <= weight(candidate))
            {
                return candidate;
            }
        }

        return 0.5 * endpoint_energy;
    }

    void PrimaryGeneratorAction::AddIsotropicPrimary(G4Event* event,
                                                     G4ParticleDefinition* definition,
                                                     const G4ThreeVector& vertex,
                                                     double time,
                                                     double kinetic_energy)
    {
        if (definition == nullptr || kinetic_energy < 0.0)
        {
            return;
        }

        const double mass = definition->GetPDGMass();
        const double momentum = std::sqrt(std::max(0.0,
                                                   kinetic_energy *
                                                       (kinetic_energy + 2.0 * mass)));
        const double cos_theta = CLHEP::RandFlat::shoot(-1.0, 1.0);
        const double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
        const double phi = CLHEP::RandFlat::shoot(0.0, 2.0 * std::acos(-1.0));

        auto* primary_vertex = new G4PrimaryVertex(vertex, time);
        auto* particle = new G4PrimaryParticle(definition,
                                               momentum * sin_theta * std::cos(phi),
                                               momentum * sin_theta * std::sin(phi),
                                               momentum * cos_theta);
        primary_vertex->SetPrimary(particle);
        event->AddPrimaryVertex(primary_vertex);
    }

    int PrimaryGeneratorAction::AddRadiologicalDecayProducts(
        G4Event* event,
        const RadiologicalIsotopeConfig& isotope,
        const G4ThreeVector& vertex,
        double decay_time)
    {
        auto isotope_matches = [&](const char* name, int z, int a) {
            return isotope.name == name || (isotope.z == z && isotope.a == a);
        };

        G4ParticleDefinition* electron = FindParticleDefinition(11);
        G4ParticleDefinition* gamma = FindParticleDefinition(22);
        G4ParticleDefinition* alpha = G4IonTable::GetIonTable()->GetIon(2, 4, 0.0);

        if (isotope_matches("Ar39", 18, 39))
        {
            AddIsotropicPrimary(event, electron, vertex, decay_time,
                                SampleAllowedBetaKineticEnergy(0.565 * MeV));
            return 1;
        }

        if (isotope_matches("Ar42", 18, 42))
        {
            AddIsotropicPrimary(event, electron, vertex, decay_time,
                                SampleAllowedBetaKineticEnergy(0.599 * MeV));
            return 1;
        }

        if (isotope_matches("K42", 19, 42))
        {
            AddIsotropicPrimary(event, electron, vertex, decay_time,
                                SampleAllowedBetaKineticEnergy(3.525 * MeV));
            return 1;
        }

        if (isotope_matches("Kr85", 36, 85))
        {
            AddIsotropicPrimary(event, electron, vertex, decay_time,
                                SampleAllowedBetaKineticEnergy(0.687 * MeV));
            return 1;
        }

        if (isotope_matches("K40", 19, 40))
        {
            if (CLHEP::RandFlat::shoot() < 0.8928)
            {
                AddIsotropicPrimary(event, electron, vertex, decay_time,
                                    SampleAllowedBetaKineticEnergy(1.311 * MeV));
            }
            else
            {
                AddIsotropicPrimary(event, gamma, vertex, decay_time, 1.461 * MeV);
            }
            return 1;
        }

        if (isotope_matches("Rn222", 86, 222))
        {
            AddIsotropicPrimary(event, alpha, vertex, decay_time, 5.49 * MeV);
            return 1;
        }

        G4ParticleDefinition* ion =
            G4IonTable::GetIonTable()->GetIon(isotope.z,
                                               isotope.a,
                                               std::max(0.0, isotope.excitation_mev) * MeV);
        if (ion == nullptr)
        {
            return 0;
        }

        auto* primary_vertex = new G4PrimaryVertex(vertex, decay_time);
        auto* particle = new G4PrimaryParticle(ion, 0.0, 0.0, 0.0);
        particle->SetProperTime(0.0);
        primary_vertex->SetPrimary(particle);
        event->AddPrimaryVertex(primary_vertex);
        return 1;
    }

    int PrimaryGeneratorAction::AddRadiologicalBackgrounds(G4Event* event,
                                                           GeneratorTruthRecord& truth)
    {
        if (!RadiologicalOverlayEnabled())
        {
            return 0;
        }

        std::vector<RadiologicalIsotopeConfig> isotopes = generator_config_.radiological_isotopes;
        if (isotopes.empty())
        {
            RadiologicalIsotopeConfig ar39;
            ar39.name = "Ar39";
            ar39.z = 18;
            ar39.a = 39;
            ar39.activity_bq_per_kg = 1.01;
            ar39.excitation_mev = 0.0;
            isotopes.push_back(ar39);
        }

        const double mass_kg = generator_config_.radiological_mass_override_kg > 0.0
                                   ? generator_config_.radiological_mass_override_kg
                                   : ActiveVolumeMassKg();
        const double window_us = std::max(0.0, generator_config_.radiological_window_us);
        const double window_s = window_us * 1.0e-6;

        truth.radiological_enabled = true;
        truth.radiological_mass_kg = mass_kg;
        truth.radiological_window_us = window_us;

        if (mass_kg <= 0.0 || window_s <= 0.0)
        {
            G4ExceptionDescription description;
            description << "Radiological backgrounds require positive mass and time window. "
                        << "Set /generator/radiological/massOverrideKg if the active-volume "
                        << "mass cannot be inferred from the geometry.";
            G4Exception("PrimaryGeneratorAction::AddRadiologicalBackgrounds",
                        "G4LArBoxRadiologicalConfig",
                        FatalException,
                        description);
            return 0;
        }

        if (truth.source.find("radiological") == std::string::npos)
        {
            truth.source += "+radiological";
        }

        int total_decays = 0;
        const int max_decays = generator_config_.radiological_max_decays_per_event;
        for (const auto& isotope : isotopes)
        {
            if (isotope.z <= 0 || isotope.a <= 0 || isotope.activity_bq_per_kg <= 0.0)
            {
                continue;
            }

            const double expected = isotope.activity_bq_per_kg * mass_kg * window_s;
            truth.radiological_expected_decays += expected;
            long sampled_decays = CLHEP::RandPoisson::shoot(expected);
            if (max_decays > 0)
            {
                sampled_decays = std::min<long>(sampled_decays,
                                                std::max(0, max_decays - total_decays));
            }

            for (long decay = 0; decay < sampled_decays; ++decay)
            {
                G4ThreeVector vertex;
                BulkVertexGenerator(vertex);
                const double decay_time = CLHEP::RandFlat::shoot(0.0, window_us * microsecond);
                const int primaries =
                    AddRadiologicalDecayProducts(event, isotope, vertex, decay_time);
                if (primaries <= 0)
                {
                    G4cout << "-- Skipping unsupported radiological isotope "
                           << isotope.name << " Z=" << isotope.z
                           << " A=" << isotope.a << G4endl;
                    continue;
                }

                ++total_decays;
                truth.radiological_isotope.push_back(isotope.name);
                truth.radiological_z.push_back(isotope.z);
                truth.radiological_a.push_back(isotope.a);
                truth.radiological_activity_bq_per_kg.push_back(isotope.activity_bq_per_kg);
                truth.radiological_decay_time_ns.push_back(decay_time / ns);
            }

            if (max_decays > 0 && total_decays >= max_decays)
            {
                break;
            }
        }

        truth.radiological_decays = total_decays;
        return total_decays;
    }

    double PrimaryGeneratorAction::RockNeutronShellArea() const
    {
        const ActiveBoxBounds bounds = GetActiveBoxBounds();
        const double padding = std::max(0.0, generator_config_.rock_neutrons.shell_padding_cm) * cm;
        const double hx = bounds.hx + padding;
        const double hy = bounds.hy + padding;
        const double hz = bounds.hz + padding;
        return 8.0 * (hx * hy + hx * hz + hy * hz) / (cm * cm);
    }

    double PrimaryGeneratorAction::RockNeutronExpectedCount(double shell_area) const
    {
        const RockNeutronConfig& config = generator_config_.rock_neutrons;
        if (config.mean_per_event >= 0.0)
        {
            return config.mean_per_event;
        }

        const double window_s = std::max(0.0, config.window_us) * 1.0e-6;
        if (config.rate_hz > 0.0)
        {
            return config.rate_hz * window_s;
        }

        if (config.flux_per_cm2_s > 0.0)
        {
            return config.flux_per_cm2_s * std::max(0.0, shell_area) * window_s;
        }

        return RockNeutronOverlayEnabled() ? 1.0 : 0.0;
    }

    double PrimaryGeneratorAction::SampleRockNeutronEnergy() const
    {
        const RockNeutronConfig& config = generator_config_.rock_neutrons;
        double emin = std::max(0.0, config.energy_min_mev) * MeV;
        double emax = std::max(emin, config.energy_max_mev * MeV);
        if (emax <= emin)
        {
            return emin;
        }

        if (config.spectrum == "mono")
        {
            return std::max(emin, std::min(emax, config.energy_mean_mev * MeV));
        }

        if (config.spectrum == "flat")
        {
            return CLHEP::RandFlat::shoot(emin, emax);
        }

        const double scale = std::max(1.0e-9 * MeV, config.energy_mean_mev * MeV);
        if (config.spectrum == "exponential")
        {
            for (int attempt = 0; attempt < 10000; ++attempt)
            {
                const double energy = -scale * std::log(std::max(1.0e-12, CLHEP::RandFlat::shoot()));
                if (energy >= emin && energy <= emax)
                {
                    return energy;
                }
            }

            return std::max(emin, std::min(emax, scale));
        }

        auto radiogenic_weight = [scale](double energy) {
            if (energy <= 0.0)
            {
                return 0.0;
            }
            return std::sqrt(energy / MeV) * std::exp(-energy / scale);
        };

        const double peak = std::max(emin, std::min(emax, 0.5 * scale));
        const double max_weight = std::max(radiogenic_weight(peak),
                                           std::max(radiogenic_weight(emin),
                                                    radiogenic_weight(emax)));
        if (max_weight <= 0.0)
        {
            return CLHEP::RandFlat::shoot(emin, emax);
        }

        for (int attempt = 0; attempt < 10000; ++attempt)
        {
            const double energy = CLHEP::RandFlat::shoot(emin, emax);
            if (CLHEP::RandFlat::shoot(0.0, max_weight) <= radiogenic_weight(energy))
            {
                return energy;
            }
        }

        return std::max(emin, std::min(emax, scale));
    }

    G4ThreeVector PrimaryGeneratorAction::SampleRockNeutronVertex(G4ThreeVector& inward_normal,
                                                                  int& face) const
    {
        const ActiveBoxBounds bounds = GetActiveBoxBounds();
        const double padding = std::max(0.0, generator_config_.rock_neutrons.shell_padding_cm) * cm;
        const double hx = bounds.hx + padding;
        const double hy = bounds.hy + padding;
        const double hz = bounds.hz + padding;

        const std::array<double, 6> face_area = {{
            4.0 * hy * hz,
            4.0 * hy * hz,
            4.0 * hx * hz,
            4.0 * hx * hz,
            4.0 * hx * hy,
            4.0 * hx * hy,
        }};
        double total_area = 0.0;
        for (double area : face_area)
        {
            total_area += area;
        }

        double pick = CLHEP::RandFlat::shoot(0.0, total_area);
        face = 0;
        for (std::size_t i = 0; i < face_area.size(); ++i)
        {
            if (pick <= face_area[i])
            {
                face = static_cast<int>(i);
                break;
            }
            pick -= face_area[i];
        }

        const double x = CLHEP::RandFlat::shoot(-hx, hx);
        const double y = CLHEP::RandFlat::shoot(-hy, hy);
        const double z = CLHEP::RandFlat::shoot(-hz, hz);
        switch (face)
        {
            case 0:
                inward_normal = G4ThreeVector(1.0, 0.0, 0.0);
                return bounds.center + G4ThreeVector(-hx, y, z);
            case 1:
                inward_normal = G4ThreeVector(-1.0, 0.0, 0.0);
                return bounds.center + G4ThreeVector(hx, y, z);
            case 2:
                inward_normal = G4ThreeVector(0.0, 1.0, 0.0);
                return bounds.center + G4ThreeVector(x, -hy, z);
            case 3:
                inward_normal = G4ThreeVector(0.0, -1.0, 0.0);
                return bounds.center + G4ThreeVector(x, hy, z);
            case 4:
                inward_normal = G4ThreeVector(0.0, 0.0, 1.0);
                return bounds.center + G4ThreeVector(x, y, -hz);
            default:
                inward_normal = G4ThreeVector(0.0, 0.0, -1.0);
                return bounds.center + G4ThreeVector(x, y, hz);
        }
    }

    G4ThreeVector PrimaryGeneratorAction::SampleRockNeutronDirection(
        const G4ThreeVector& vertex,
        const G4ThreeVector& inward_normal)
    {
        const std::string& model = generator_config_.rock_neutrons.direction_model;
        if (model == "target")
        {
            G4ThreeVector target;
            BulkVertexGenerator(target);
            G4ThreeVector direction = target - vertex;
            if (direction.mag2() > 0.0)
            {
                return direction.unit();
            }
            return inward_normal.unit();
        }

        const double mu = model == "isotropic"
                              ? CLHEP::RandFlat::shoot(0.0, 1.0)
                              : std::sqrt(CLHEP::RandFlat::shoot(0.0, 1.0));
        const double transverse = std::sqrt(std::max(0.0, 1.0 - mu * mu));
        const double phi = CLHEP::RandFlat::shoot(0.0, 2.0 * std::acos(-1.0));
        const G4ThreeVector normal = inward_normal.unit();
        const G4ThreeVector helper = std::abs(normal.x()) < 0.9
                                         ? G4ThreeVector(1.0, 0.0, 0.0)
                                         : G4ThreeVector(0.0, 1.0, 0.0);
        const G4ThreeVector tangent1 = normal.cross(helper).unit();
        const G4ThreeVector tangent2 = normal.cross(tangent1).unit();
        return (mu * normal +
                transverse * std::cos(phi) * tangent1 +
                transverse * std::sin(phi) * tangent2).unit();
    }

    int PrimaryGeneratorAction::AddRockNeutronBackgrounds(G4Event* event,
                                                          GeneratorTruthRecord& truth)
    {
        if (!RockNeutronOverlayEnabled())
        {
            return 0;
        }

        const double shell_area = RockNeutronShellArea();
        const double expected = RockNeutronExpectedCount(shell_area);
        const double window_us = std::max(0.0, generator_config_.rock_neutrons.window_us);
        truth.rock_neutron_enabled = true;
        truth.rock_neutron_window_us = window_us;
        truth.rock_neutron_expected = expected;

        if (expected <= 0.0)
        {
            return 0;
        }

        long sampled_neutrons = CLHEP::RandPoisson::shoot(expected);
        const int max_neutrons = generator_config_.rock_neutrons.max_neutrons_per_event;
        if (max_neutrons > 0)
        {
            sampled_neutrons = std::min<long>(sampled_neutrons, max_neutrons);
        }

        G4ParticleDefinition* neutron = FindParticleDefinition(2112);
        if (neutron == nullptr)
        {
            throw std::runtime_error("Failed to find Geant4 neutron particle definition.");
        }

        AppendSourceTag(truth.source, "rock_neutrons");

        int injected_neutrons = 0;
        for (long i = 0; i < sampled_neutrons; ++i)
        {
            G4ThreeVector inward_normal;
            int face = 0;
            const G4ThreeVector vertex = SampleRockNeutronVertex(inward_normal, face);
            const G4ThreeVector direction =
                SampleRockNeutronDirection(vertex, inward_normal);
            const double kinetic_energy = SampleRockNeutronEnergy();
            const double mass = neutron->GetPDGMass();
            const double momentum = std::sqrt(std::max(0.0,
                                                       kinetic_energy *
                                                           (kinetic_energy + 2.0 * mass)));
            const double time = CLHEP::RandFlat::shoot(0.0, window_us * microsecond);

            auto* primary_vertex = new G4PrimaryVertex(vertex, time);
            auto* particle = new G4PrimaryParticle(neutron,
                                                   momentum * direction.x(),
                                                   momentum * direction.y(),
                                                   momentum * direction.z());
            primary_vertex->SetPrimary(particle);
            event->AddPrimaryVertex(primary_vertex);

            ++injected_neutrons;
            truth.rock_neutron_x.push_back(vertex.x());
            truth.rock_neutron_y.push_back(vertex.y());
            truth.rock_neutron_z.push_back(vertex.z());
            truth.rock_neutron_time_ns.push_back(time / ns);
            truth.rock_neutron_energy_mev.push_back(kinetic_energy / MeV);
            truth.rock_neutron_dir_x.push_back(direction.x());
            truth.rock_neutron_dir_y.push_back(direction.y());
            truth.rock_neutron_dir_z.push_back(direction.z());
            truth.rock_neutron_face.push_back(face);
        }

        truth.rock_neutron_count = injected_neutrons;
        return injected_neutrons;
    }

    void PrimaryGeneratorAction::GenerateGeniePrimaries(G4Event* event)
    {
        genie_reader_->Configure(generator_config_.genie_file_path,
                                 generator_config_.genie_tree_name,
                                 false);
        const GenieGSTEvent genie_event = genie_reader_->ReadNext(generator_config_.genie_cycle_events);

        GeneratorTruthRecord truth;
        truth.source = "genie_gst";
        AddGenieEvent(event, genie_event, truth, true);
        AddRadiologicalBackgrounds(event, truth);
        AddRockNeutronBackgrounds(event, truth);
        CaptureTruthFromEvent(event, truth);
        DataHandler::Instance()->SetGeneratorTruth(truth);
    }

    void PrimaryGeneratorAction::GenerateGenieProtonDecayPrimaries(G4Event* event)
    {
        genie_reader_->Configure(generator_config_.genie_file_path,
                                 generator_config_.genie_tree_name,
                                 true);
        const GenieGSTEvent genie_event = genie_reader_->ReadNext(generator_config_.genie_cycle_events);

        GeneratorTruthRecord truth;
        truth.source = "genie_pdecay";
        AddGenieEvent(event, genie_event, truth, false);
        AddRadiologicalBackgrounds(event, truth);
        AddRockNeutronBackgrounds(event, truth);
        CaptureTruthFromEvent(event, truth);
        DataHandler::Instance()->SetGeneratorTruth(truth);
    }

    void PrimaryGeneratorAction::GenerateCorsikaPrimaries(G4Event* event)
    {
        corsika_reader_->Configure(generator_config_.corsika_file_path,
                                   generator_config_.corsika_tree_name);
        const CorsikaEvent corsika_event = corsika_reader_->ReadNext(generator_config_.corsika_cycle_events);

        const int injected_particles = AddCorsikaEvent(event, corsika_event);
        if (injected_particles == 0)
        {
            throw std::runtime_error(
                "CORSIKA event " + std::to_string(corsika_event.iev) +
                " produced no Geant4 primaries.");
        }

        GeneratorTruthRecord truth;
        truth.source = "corsika";
        AddRadiologicalBackgrounds(event, truth);
        AddRockNeutronBackgrounds(event, truth);
        CaptureTruthFromEvent(event, truth);
        DataHandler::Instance()->SetGeneratorTruth(truth);
    }

    void PrimaryGeneratorAction::GenerateCorsikaGenieOverlayPrimaries(G4Event* event)
    {
        genie_reader_->Configure(generator_config_.genie_file_path,
                                 generator_config_.genie_tree_name,
                                 false);
        corsika_reader_->Configure(generator_config_.corsika_file_path,
                                   generator_config_.corsika_tree_name);

        const GenieGSTEvent genie_event = genie_reader_->ReadNext(generator_config_.genie_cycle_events);
        const CorsikaEvent corsika_event = corsika_reader_->ReadNext(generator_config_.corsika_cycle_events);

        GeneratorTruthRecord truth;
        truth.source = "corsika+genie_gst_overlay";
        AddGenieEvent(event, genie_event, truth, true);

        const int injected_cosmics = AddCorsikaEvent(event, corsika_event);
        if (injected_cosmics == 0)
        {
            throw std::runtime_error(
                "CORSIKA event " + std::to_string(corsika_event.iev) +
                " produced no Geant4 primaries for overlay.");
        }

        AddRadiologicalBackgrounds(event, truth);
        AddRockNeutronBackgrounds(event, truth);
        CaptureTruthFromEvent(event, truth);
        DataHandler::Instance()->SetGeneratorTruth(truth);
    }

    void PrimaryGeneratorAction::GenerateMarleyPrimaries(G4Event* event)
    {
        marley_generator_->Configure(generator_config_.marley_config_path,
                                     generator_config_.marley_event_file_path,
                                     generator_config_.marley_cycle_events,
                                     generator_config_.marley_seed,
                                     generator_config_.marley_has_seed);
        const MarleyEventRecord marley_event = marley_generator_->ReadNext();

        GeneratorTruthRecord truth;
        truth.source = generator_config_.marley_event_file_path.empty()
                           ? "marley"
                           : "marley_file";
        AddMarleyEvent(event, marley_event, truth);
        AddRadiologicalBackgrounds(event, truth);
        AddRockNeutronBackgrounds(event, truth);
        CaptureTruthFromEvent(event, truth);
        DataHandler::Instance()->SetGeneratorTruth(truth);
    }

    void PrimaryGeneratorAction::GenerateBxDecay0Primaries(G4Event* event)
    {
        const BxDecay0EventRecord bx_event =
            bxdecay0_generator_->Generate(generator_config_.bxdecay0);

        GeneratorTruthRecord truth;
        truth.source = "bxdecay0";
        AddBxDecay0Event(event, bx_event, truth);
        AddRadiologicalBackgrounds(event, truth);
        AddRockNeutronBackgrounds(event, truth);
        CaptureTruthFromEvent(event, truth);
        DataHandler::Instance()->SetGeneratorTruth(truth);
    }

    void PrimaryGeneratorAction::GenerateRadiologicalPrimaries(G4Event* event)
    {
        GeneratorTruthRecord truth;
        truth.source = "radiological";
        AddRadiologicalBackgrounds(event, truth);
        AddRockNeutronBackgrounds(event, truth);
        CaptureTruthFromEvent(event, truth);
        DataHandler::Instance()->SetGeneratorTruth(truth);
    }

    void PrimaryGeneratorAction::GenerateRockNeutronPrimaries(G4Event* event)
    {
        GeneratorTruthRecord truth;
        truth.source = "rock_neutrons";
        AddRockNeutronBackgrounds(event, truth);
        AddRadiologicalBackgrounds(event, truth);
        CaptureTruthFromEvent(event, truth);
        DataHandler::Instance()->SetGeneratorTruth(truth);
    }

    int PrimaryGeneratorAction::AddGenieEvent(G4Event* event,
                                              const GenieGSTEvent& genie_event,
                                              GeneratorTruthRecord& truth,
                                              bool include_primary_lepton)
    {
        truth.genie_iev = genie_event.iev;
        truth.genie_neu = genie_event.neu;
        truth.genie_tgt = genie_event.tgt;
        truth.genie_target_z = genie_event.target_z;
        truth.genie_target_a = genie_event.target_a;
        truth.genie_cc = genie_event.cc;
        truth.genie_nc = genie_event.nc;
        truth.genie_qel = genie_event.qel;
        truth.genie_res = genie_event.res;
        truth.genie_dis = genie_event.dis;
        truth.genie_coh = genie_event.coh;
        truth.genie_nuel = genie_event.nuel;
        truth.genie_imd = genie_event.imd;
        truth.genie_em = genie_event.em;
        truth.genie_weight = genie_event.weight;
        truth.genie_xs = genie_event.xs;
        truth.genie_ev = genie_event.Ev;
        truth.genie_input_vtxx = genie_event.vtxx;
        truth.genie_input_vtxy = genie_event.vtxy;
        truth.genie_input_vtxz = genie_event.vtxz;
        truth.genie_input_vtxt = genie_event.vtxt;

        const int abs_neutrino = std::abs(genie_event.neu);
        if (abs_neutrino == 12 || abs_neutrino == 14 || abs_neutrino == 16)
        {
            truth.has_incident_direction = true;
            truth.incident_dir_x = 0.0;
            truth.incident_dir_y = 0.0;
            truth.incident_dir_z = 1.0;
        }

        G4ThreeVector interaction_vertex;
        double interaction_time = 0.0;
        if (generator_config_.genie_use_input_vertex)
        {
            interaction_vertex = G4ThreeVector(genie_event.vtxx * m,
                                               genie_event.vtxy * m,
                                               genie_event.vtxz * m);
            interaction_time = genie_event.vtxt * s;
        }
        else
        {
            BulkVertexGenerator(interaction_vertex);
        }

        auto* primary_vertex = new G4PrimaryVertex(interaction_vertex, interaction_time);
        int injected_particles = 0;

        auto append_particle = [&](int pdg_code, double px, double py, double pz, const char* label) {
            G4ParticleDefinition* definition = FindParticleDefinition(pdg_code);
            if (definition == nullptr)
            {
                G4cout << "-- Skipping unsupported GENIE " << label
                       << " PDG " << pdg_code << G4endl;
                return;
            }

            auto* particle = new G4PrimaryParticle(definition,
                                                   px * GeV,
                                                   py * GeV,
                                                   pz * GeV);
            primary_vertex->SetPrimary(particle);
            ++injected_particles;
        };

        const int lepton_pdg = DetermineGeniePrimaryLeptonPdg(genie_event);
        const bool has_explicit_lepton =
            lepton_pdg != 0 &&
            (genie_event.El > 0.0 ||
             genie_event.pxl != 0.0 ||
             genie_event.pyl != 0.0 ||
             genie_event.pzl != 0.0);

        if (include_primary_lepton && has_explicit_lepton)
        {
            append_particle(lepton_pdg,
                            genie_event.pxl,
                            genie_event.pyl,
                            genie_event.pzl,
                            "primary lepton");
        }

        for (std::size_t i = 0; i < genie_event.pdgf.size(); ++i)
        {
            append_particle(genie_event.pdgf[i],
                            genie_event.pxf[i],
                            genie_event.pyf[i],
                            genie_event.pzf[i],
                            "final-state particle");
        }

        if (injected_particles == 0)
        {
            delete primary_vertex;
            throw std::runtime_error(
                "GENIE event " + std::to_string(genie_event.iev) +
                " produced no Geant4 primaries.");
        }

        event->AddPrimaryVertex(primary_vertex);
        return injected_particles;
    }

    int PrimaryGeneratorAction::AddMarleyEvent(G4Event* event,
                                               const MarleyEventRecord& marley_event,
                                               GeneratorTruthRecord& truth)
    {
        truth.marley_event = marley_event.event_index;
        truth.marley_flux_averaged_xsec = marley_event.flux_averaged_xsec;
        truth.marley_projectile_pdg = marley_event.projectile_pdg;
        truth.marley_target_pdg = marley_event.target_pdg;
        truth.marley_ejectile_pdg = marley_event.ejectile_pdg;
        truth.marley_residue_pdg = marley_event.residue_pdg;
        truth.marley_ex = marley_event.excitation_energy_mev;

        const double projectile_p =
            std::sqrt(marley_event.projectile_px_mev * marley_event.projectile_px_mev +
                      marley_event.projectile_py_mev * marley_event.projectile_py_mev +
                      marley_event.projectile_pz_mev * marley_event.projectile_pz_mev);
        if (projectile_p > 0.0)
        {
            truth.has_incident_direction = true;
            truth.incident_dir_x = marley_event.projectile_px_mev / projectile_p;
            truth.incident_dir_y = marley_event.projectile_py_mev / projectile_p;
            truth.incident_dir_z = marley_event.projectile_pz_mev / projectile_p;
        }

        G4ThreeVector interaction_vertex;
        BulkVertexGenerator(interaction_vertex);

        int injected_particles = 0;
        double global_time = 0.0;
        std::size_t cascade_idx = 0;
        for (std::size_t i = 0; i < marley_event.pdg.size(); ++i)
        {
            double primary_time = global_time;
            if (i > 1 &&
                marley_event.residue_pdg == kPotassium40ResiduePdg &&
                cascade_idx < marley_event.cascade_level_energy_mev.size())
            {
                double half_life = 0.0;
                if (FindDelayedGammaHalfLife(marley_event.cascade_level_energy_mev[cascade_idx], half_life))
                {
                    global_time += SampleFiniteParticleTime(half_life);
                    primary_time = global_time;
                }
                ++cascade_idx;
            }

            G4ParticleDefinition* definition = FindParticleDefinition(marley_event.pdg[i]);
            if (definition == nullptr)
            {
                G4cout << "-- Skipping unsupported MARLEY final-state PDG "
                       << marley_event.pdg[i] << G4endl;
                continue;
            }

            auto* primary_vertex = new G4PrimaryVertex(interaction_vertex, primary_time);
            auto* particle = new G4PrimaryParticle(definition,
                                                   marley_event.px_mev[i] * MeV,
                                                   marley_event.py_mev[i] * MeV,
                                                   marley_event.pz_mev[i] * MeV);
            if (i < marley_event.charge.size())
            {
                particle->SetCharge(marley_event.charge[i]);
            }
            primary_vertex->SetPrimary(particle);
            event->AddPrimaryVertex(primary_vertex);
            ++injected_particles;
        }

        if (injected_particles == 0)
        {
            throw std::runtime_error(
                "MARLEY event " + std::to_string(marley_event.event_index) +
                " produced no Geant4 primaries.");
        }

        return injected_particles;
    }

    int PrimaryGeneratorAction::AddBxDecay0Event(G4Event* event,
                                                 const BxDecay0EventRecord& bx_event,
                                                 GeneratorTruthRecord& truth)
    {
        truth.bxdecay0_category = bx_event.category;
        truth.bxdecay0_nuclide = bx_event.nuclide;
        truth.bxdecay0_seed = bx_event.seed;
        truth.bxdecay0_event = bx_event.event_index;
        truth.bxdecay0_particles = static_cast<int>(bx_event.pdg.size());
        truth.bxdecay0_event_time_ns = bx_event.event_time_ns;
        truth.bxdecay0_pdg = bx_event.pdg;
        truth.bxdecay0_px_mev = bx_event.px_mev;
        truth.bxdecay0_py_mev = bx_event.py_mev;
        truth.bxdecay0_pz_mev = bx_event.pz_mev;
        truth.bxdecay0_time_ns = bx_event.time_ns;

        G4ThreeVector decay_vertex;
        BulkVertexGenerator(decay_vertex);

        int injected_particles = 0;
        for (std::size_t i = 0; i < bx_event.pdg.size(); ++i)
        {
            G4ParticleDefinition* definition = FindParticleDefinition(bx_event.pdg[i]);
            if (definition == nullptr)
            {
                G4cout << "-- Skipping unsupported BxDecay0 primary PDG "
                       << bx_event.pdg[i] << G4endl;
                continue;
            }

            const double primary_time = i < bx_event.time_ns.size()
                                            ? bx_event.time_ns[i] * ns
                                            : bx_event.event_time_ns * ns;
            auto* primary_vertex = new G4PrimaryVertex(decay_vertex, primary_time);
            auto* particle = new G4PrimaryParticle(
                definition,
                bx_event.px_mev[i] * MeV,
                bx_event.py_mev[i] * MeV,
                bx_event.pz_mev[i] * MeV);
            primary_vertex->SetPrimary(particle);
            event->AddPrimaryVertex(primary_vertex);
            ++injected_particles;
        }

        if (injected_particles == 0)
        {
            throw std::runtime_error("BxDecay0 event " +
                                     std::to_string(bx_event.event_index) +
                                     " produced no Geant4 primaries.");
        }

        return injected_particles;
    }

    int PrimaryGeneratorAction::AddCorsikaEvent(G4Event* event, const CorsikaEvent& corsika_event)
    {
        int injected_particles = 0;
        for (const auto& primary : corsika_event.primaries)
        {
            G4ParticleDefinition* definition = FindParticleDefinition(primary.pdg);
            if (definition == nullptr)
            {
                G4cout << "-- Skipping unsupported CORSIKA primary PDG "
                       << primary.pdg << G4endl;
                continue;
            }

            auto* primary_vertex = new G4PrimaryVertex(G4ThreeVector(primary.x * m,
                                                                     primary.y * m,
                                                                     primary.z * m),
                                                       primary.t * s);
            auto* particle = new G4PrimaryParticle(definition,
                                                   primary.px * GeV,
                                                   primary.py * GeV,
                                                   primary.pz * GeV);
            primary_vertex->SetPrimary(particle);
            event->AddPrimaryVertex(primary_vertex);
            ++injected_particles;
        }

        return injected_particles;
    }

    void PrimaryGeneratorAction::CaptureTruthFromEvent(const G4Event* event, GeneratorTruthRecord& truth) const
    {
        const G4PrimaryVertex* vertex = event->GetPrimaryVertex();
        if (vertex == nullptr)
        {
            return;
        }

        truth.vertex_x = vertex->GetX0();
        truth.vertex_y = vertex->GetY0();
        truth.vertex_z = vertex->GetZ0();
        truth.vertex_t = vertex->GetT0();

        for (const G4PrimaryVertex* current_vertex = vertex;
             current_vertex != nullptr;
             current_vertex = current_vertex->GetNext())
        {
            for (const G4PrimaryParticle* particle = current_vertex->GetPrimary();
                 particle != nullptr;
                 particle = particle->GetNext())
            {
                truth.primary_pdg.push_back(particle->GetPDGcode());
                truth.primary_energy.push_back(particle->GetTotalEnergy());
                truth.primary_px.push_back(particle->GetPx());
                truth.primary_py.push_back(particle->GetPy());
                truth.primary_pz.push_back(particle->GetPz());
            }
        }
    }

    G4ParticleDefinition* PrimaryGeneratorAction::FindParticleDefinition(int pdg_code) const
    {
        G4ParticleTable* particle_table = G4ParticleTable::GetParticleTable();
        G4ParticleDefinition* definition = particle_table->FindParticle(pdg_code);
        if (definition != nullptr)
        {
            return definition;
        }

        const int abs_pdg = std::abs(pdg_code);
        if (abs_pdg < 1000000000 || pdg_code < 0)
        {
            return nullptr;
        }

        const int strange_quarks = (abs_pdg / 10000000) % 10;
        const int z = (abs_pdg / 10000) % 1000;
        const int a = (abs_pdg / 10) % 1000;
        if (strange_quarks != 0 || z <= 0 || a <= 0)
        {
            return nullptr;
        }

        return G4IonTable::GetIonTable()->GetIon(z, a, 0.0);
    }

    int PrimaryGeneratorAction::DetermineGeniePrimaryLeptonPdg(const GenieGSTEvent& event) const
    {
        const int neutrino_pdg = event.neu;

        if (event.cc)
        {
            switch (std::abs(neutrino_pdg))
            {
                case 12:
                    return neutrino_pdg > 0 ? 11 : -11;
                case 14:
                    return neutrino_pdg > 0 ? 13 : -13;
                case 16:
                    return neutrino_pdg > 0 ? 15 : -15;
                default:
                    return 0;
            }
        }

        if (event.nc)
        {
            return neutrino_pdg;
        }

        if (event.nuel || event.em)
        {
            return 11;
        }

        if (event.imd)
        {
            return neutrino_pdg >= 0 ? 13 : -13;
        }

        return 0;
    }
}

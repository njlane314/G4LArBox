#include "BxDecay0Generator.hh"

#include "bxdecay0/bb_utils.h"
#include "bxdecay0/decay0_generator.h"
#include "bxdecay0/event.h"
#include "bxdecay0/mdl_event_op.h"
#include "bxdecay0/particle.h"
#include "bxdecay0/std_random.h"

#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>

namespace G4LArBox
{
    namespace
    {
        bool SameConfig(const BxDecay0Config& lhs, const BxDecay0Config& rhs)
        {
            return lhs.category == rhs.category &&
                   lhs.nuclide == rhs.nuclide &&
                   lhs.seed == rhs.seed &&
                   lhs.debug == rhs.debug &&
                   lhs.verbosity == rhs.verbosity &&
                   lhs.dbd_mode == rhs.dbd_mode &&
                   lhs.dbd_level == rhs.dbd_level &&
                   lhs.dbd_min_energy_mev == rhs.dbd_min_energy_mev &&
                   lhs.dbd_max_energy_mev == rhs.dbd_max_energy_mev &&
                   lhs.use_mdl == rhs.use_mdl &&
                   lhs.mdl_target_name == rhs.mdl_target_name &&
                   lhs.mdl_target_rank == rhs.mdl_target_rank &&
                   lhs.mdl_cone_longitude_deg == rhs.mdl_cone_longitude_deg &&
                   lhs.mdl_cone_colatitude_deg == rhs.mdl_cone_colatitude_deg &&
                   lhs.mdl_cone_aperture_deg == rhs.mdl_cone_aperture_deg &&
                   lhs.mdl_cone_aperture2_deg == rhs.mdl_cone_aperture2_deg &&
                   lhs.mdl_error_on_missing_particle == rhs.mdl_error_on_missing_particle;
        }

        int ParticlePdg(const bxdecay0::particle& particle)
        {
            if (particle.is_gamma())
            {
                return 22;
            }
            if (particle.is_electron())
            {
                return 11;
            }
            if (particle.is_positron())
            {
                return -11;
            }
            if (particle.is_neutron())
            {
                return 2112;
            }
            if (particle.is_proton())
            {
                return 2212;
            }
            if (particle.is_alpha())
            {
                return 1000020040;
            }
            return 0;
        }

        bxdecay0::particle_code MdlParticleCode(const std::string& name)
        {
            if (name == "*" || name == "all")
            {
                return bxdecay0::INVALID_PARTICLE;
            }
            if (name == "g" || name == "gamma")
            {
                return bxdecay0::GAMMA;
            }
            if (name == "e+" || name == "positron")
            {
                return bxdecay0::POSITRON;
            }
            if (name == "e-" || name == "electron")
            {
                return bxdecay0::ELECTRON;
            }
            if (name == "n" || name == "neutron")
            {
                return bxdecay0::NEUTRON;
            }
            if (name == "p" || name == "proton")
            {
                return bxdecay0::PROTON;
            }
            if (name == "a" || name == "alpha")
            {
                return bxdecay0::ALPHA;
            }

            throw std::runtime_error("Unsupported BxDecay0 MDL target particle: " + name);
        }

        double DegToRad(double degrees)
        {
            return degrees * std::acos(-1.0) / 180.0;
        }

        void EnsureBxDecay0ResourceDirectory()
        {
#ifdef G4LARBOX_BXDECAY0_RESOURCE_DIR
            if (std::getenv("BXDECAY0_RESOURCE_DIR") == nullptr)
            {
                setenv("BXDECAY0_RESOURCE_DIR", G4LARBOX_BXDECAY0_RESOURCE_DIR, 0);
            }
#endif
        }
    }

    struct BxDecay0Generator::Impl
    {
        BxDecay0Config config;
        bool has_config = false;
        std::default_random_engine generator;
        std::unique_ptr<bxdecay0::std_random> random;
        std::unique_ptr<bxdecay0::decay0_generator> decay0;

        void Configure(const BxDecay0Config& new_config)
        {
            if (has_config && SameConfig(config, new_config) && decay0 != nullptr)
            {
                return;
            }

            EnsureBxDecay0ResourceDirectory();

            if (new_config.seed < 1)
            {
                throw std::runtime_error("BxDecay0 requires a positive random seed.");
            }
            if (new_config.nuclide.empty())
            {
                throw std::runtime_error("BxDecay0 requires a decaying nuclide.");
            }
            if (new_config.category != "background" && new_config.category != "dbd")
            {
                throw std::runtime_error("BxDecay0 category must be 'background' or 'dbd'.");
            }

            config = new_config;
            has_config = true;
            decay0.reset();
            random.reset();

            generator.seed(static_cast<unsigned int>(config.seed));
            random = std::make_unique<bxdecay0::std_random>(generator);
            decay0 = std::make_unique<bxdecay0::decay0_generator>();
            decay0->set_debug(config.debug);

            if (config.category == "background")
            {
                if (bxdecay0::background_isotopes().count(config.nuclide) == 0)
                {
                    throw std::runtime_error("BxDecay0 background isotope is not supported: " + config.nuclide);
                }
                decay0->set_decay_category(bxdecay0::decay0_generator::DECAY_CATEGORY_BACKGROUND);
            }
            else
            {
                if (config.dbd_mode < 1)
                {
                    throw std::runtime_error("BxDecay0 DBD mode must be positive.");
                }
                if (config.dbd_level < 0)
                {
                    throw std::runtime_error("BxDecay0 DBD daughter level cannot be negative.");
                }
                decay0->set_decay_category(bxdecay0::decay0_generator::DECAY_CATEGORY_DBD);
                decay0->set_decay_dbd_mode(static_cast<bxdecay0::dbd_mode_type>(config.dbd_mode));
                decay0->set_decay_dbd_level(config.dbd_level);
                if (config.dbd_min_energy_mev >= 0.0 || config.dbd_max_energy_mev >= 0.0)
                {
                    const double min_energy = config.dbd_min_energy_mev >= 0.0
                                                  ? config.dbd_min_energy_mev
                                                  : 0.0;
                    const double max_energy = config.dbd_max_energy_mev >= 0.0
                                                  ? config.dbd_max_energy_mev
                                                  : 5000.0;
                    decay0->set_decay_dbd_esum_range(min_energy, max_energy);
                }
            }

            decay0->set_decay_isotope(config.nuclide);

            if (config.use_mdl)
            {
                bxdecay0::event_op_ptr op(
                    new bxdecay0::momentum_direction_lock_event_op(config.debug));
                auto& mdl = dynamic_cast<bxdecay0::momentum_direction_lock_event_op&>(*op);
                const bxdecay0::particle_code code = MdlParticleCode(config.mdl_target_name);
                const double phi = DegToRad(config.mdl_cone_longitude_deg);
                const double theta = DegToRad(config.mdl_cone_colatitude_deg);
                const double aperture = DegToRad(config.mdl_cone_aperture_deg);
                if (config.mdl_cone_aperture2_deg >= 0.0)
                {
                    mdl.set_with_aperture_rectangular_cut(
                        code,
                        config.mdl_target_rank,
                        phi,
                        theta,
                        aperture,
                        DegToRad(config.mdl_cone_aperture2_deg),
                        config.mdl_error_on_missing_particle);
                }
                else
                {
                    mdl.set(code,
                            config.mdl_target_rank,
                            phi,
                            theta,
                            aperture,
                            config.mdl_error_on_missing_particle);
                }
                decay0->add_operation(op);
            }

            decay0->initialize(*random);
        }
    };

    BxDecay0Generator::BxDecay0Generator()
        : impl_(std::make_unique<Impl>())
    {}

    BxDecay0Generator::~BxDecay0Generator() = default;

    void BxDecay0Generator::Reset()
    {
        impl_ = std::make_unique<Impl>();
    }

    BxDecay0EventRecord BxDecay0Generator::Generate(const BxDecay0Config& config)
    {
        impl_->Configure(config);
        if (!impl_->decay0->has_next())
        {
            throw std::runtime_error("BxDecay0 generator has no next decay event.");
        }

        bxdecay0::event decay_event;
        impl_->decay0->shoot(*impl_->random, decay_event);

        BxDecay0EventRecord record;
        record.category = config.category;
        record.nuclide = config.nuclide;
        record.seed = config.seed;
        record.event_index = static_cast<int>(impl_->decay0->get_event_count());
        record.event_time_ns = decay_event.has_time()
                                   ? decay_event.get_time() * 1.0e9
                                   : 0.0;

        for (const auto& particle : decay_event.get_particles())
        {
            const int pdg = ParticlePdg(particle);
            if (pdg == 0)
            {
                continue;
            }

            record.pdg.push_back(pdg);
            record.px_mev.push_back(particle.get_px());
            record.py_mev.push_back(particle.get_py());
            record.pz_mev.push_back(particle.get_pz());
            record.time_ns.push_back(record.event_time_ns +
                                     (particle.has_time() ? particle.get_time() * 1.0e9 : 0.0));
        }

        return record;
    }
}

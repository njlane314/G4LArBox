#include "MarleyGenerator.hh"

#include "marley/Event.hh"
#include "marley/Generator.hh"
#include "marley/Level.hh"
#include "marley/Particle.hh"
#include "marley/RootEventFileReader.hh"
#include "marley/RootJSONConfig.hh"

#include <cstdlib>
#include <stdexcept>

namespace G4LArBox
{
    namespace
    {
        void EnsureMarleyEnvironment()
        {
            if (std::getenv("MARLEY") != nullptr)
            {
                return;
            }

#ifdef G4LARBOX_MARLEY_DIR
            setenv("MARLEY", G4LARBOX_MARLEY_DIR, 0);
#endif
        }
    }

    MarleyGenerator::MarleyGenerator()
        : cycle_events_(false),
          seed_(0),
          has_seed_(false),
          event_index_(0)
    {}

    MarleyGenerator::~MarleyGenerator() = default;

    void MarleyGenerator::Configure(const std::string& config_path,
                                    const std::string& event_file_path,
                                    bool cycle_events,
                                    unsigned long seed,
                                    bool has_seed)
    {
        if (config_path == config_path_ &&
            event_file_path == event_file_path_ &&
            cycle_events == cycle_events_ &&
            seed == seed_ &&
            has_seed == has_seed_)
        {
            return;
        }

        config_path_ = config_path;
        event_file_path_ = event_file_path;
        cycle_events_ = cycle_events;
        seed_ = seed;
        has_seed_ = has_seed;
        Close();
    }

    MarleyEventRecord MarleyGenerator::ReadNext()
    {
        EnsureOpen();

        marley::Event event;
        double flux_averaged_xsec = 0.0;
        if (event_reader_ != nullptr)
        {
            if (!event_reader_->next_event(event))
            {
                if (!cycle_events_)
                {
                    throw std::runtime_error(
                        "MARLEY event input exhausted. Enable /generator/marley/cycleEvents to reuse the file.");
                }

                event_reader_ = std::make_unique<marley::RootEventFileReader>(event_file_path_);
                if (!event_reader_->next_event(event))
                {
                    throw std::runtime_error("MARLEY event file contains no readable events: " +
                                             event_file_path_);
                }
            }
            flux_averaged_xsec = event_reader_->flux_averaged_xsec(false);
        }
        else
        {
            event = generator_->create_event();
        }

        MarleyEventRecord record = ConvertEvent(event);
        record.event_index = static_cast<int>(event_index_++);
        record.flux_averaged_xsec = flux_averaged_xsec;
        return record;
    }

    void MarleyGenerator::Close()
    {
        generator_.reset();
        event_reader_.reset();
        event_index_ = 0;
    }

    void MarleyGenerator::EnsureOpen()
    {
        if (!event_file_path_.empty())
        {
            if (event_reader_ == nullptr)
            {
                EnsureMarleyEnvironment();
                event_reader_ = std::make_unique<marley::RootEventFileReader>(event_file_path_);
            }
            return;
        }

        if (generator_ != nullptr)
        {
            return;
        }

        if (config_path_.empty())
        {
            throw std::runtime_error(
                "MARLEY mode requires /generator/marley/config or /generator/marley/file to be set.");
        }

        EnsureMarleyEnvironment();
        marley::RootJSONConfig config(config_path_);
        generator_ = std::make_unique<marley::Generator>(config.create_generator());
        if (has_seed_)
        {
            generator_->reseed(seed_);
        }
    }

    MarleyEventRecord MarleyGenerator::ConvertEvent(const marley::Event& event) const
    {
        MarleyEventRecord record;
        record.projectile_pdg = event.projectile().pdg_code();
        record.projectile_px_mev = event.projectile().px();
        record.projectile_py_mev = event.projectile().py();
        record.projectile_pz_mev = event.projectile().pz();
        record.target_pdg = event.target().pdg_code();
        record.ejectile_pdg = event.ejectile().pdg_code();
        record.residue_pdg = event.residue().pdg_code();
        record.excitation_energy_mev = event.Ex();

        for (const auto* particle : event.get_final_particles())
        {
            if (particle == nullptr)
            {
                continue;
            }

            record.pdg.push_back(particle->pdg_code());
            record.charge.push_back(static_cast<int>(particle->charge()));
            record.total_energy_mev.push_back(particle->total_energy());
            record.px_mev.push_back(particle->px());
            record.py_mev.push_back(particle->py());
            record.pz_mev.push_back(particle->pz());
        }

        for (const auto* level : event.get_cascade_levels())
        {
            if (level == nullptr)
            {
                continue;
            }

            record.cascade_level_energy_mev.push_back(level->energy());
        }

        return record;
    }
}

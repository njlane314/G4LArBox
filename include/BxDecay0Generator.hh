#ifndef BXDECAY0GENERATOR_HH
#define BXDECAY0GENERATOR_HH

#include "GeneratorConfig.hh"

#include <memory>
#include <string>
#include <vector>

namespace G4LArBox
{
    struct BxDecay0EventRecord
    {
        std::string category;
        std::string nuclide;
        int seed = 0;
        int event_index = -1;
        double event_time_ns = 0.0;
        std::vector<int> pdg;
        std::vector<double> px_mev;
        std::vector<double> py_mev;
        std::vector<double> pz_mev;
        std::vector<double> time_ns;
    };

    class BxDecay0Generator
    {
    public:
        BxDecay0Generator();
        ~BxDecay0Generator();

        BxDecay0EventRecord Generate(const BxDecay0Config& config);
        void Reset();

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}

#endif // BXDECAY0GENERATOR_HH

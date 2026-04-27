#include "TFile.h"
#include "TRandom3.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int kMaxPrimaries = 256;
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kActiveHalfX = 1.28175;
    constexpr double kActiveHalfY = 1.165;
    constexpr double kActiveHalfZ = 5.184;
    constexpr double kTopInjectionMinY = kActiveHalfY + 0.36;
    constexpr double kTopInjectionMaxY = kActiveHalfY + 0.92;

    double Clamp(double value, double lo, double hi)
    {
        return std::max(lo, std::min(hi, value));
    }

    int PickSecondaryPdg(TRandom3& rng)
    {
        const double u = rng.Rndm();
        if (u < 0.28) return rng.Rndm() < 0.58 ? 13 : -13;
        if (u < 0.52) return rng.Rndm() < 0.5 ? 11 : -11;
        if (u < 0.67) return 22;
        if (u < 0.86) return rng.Rndm() < 0.5 ? 211 : -211;
        if (u < 0.96) return 2212;
        return 2112;
    }

    double PickMomentumGeV(TRandom3& rng, int pdg)
    {
        switch (std::abs(pdg))
        {
            case 11: return rng.Uniform(0.10, 1.80);
            case 13: return rng.Uniform(0.90, 8.00);
            case 22: return rng.Uniform(0.06, 2.20);
            case 211: return rng.Uniform(0.20, 3.40);
            case 2112: return rng.Uniform(0.22, 4.20);
            case 2212: return rng.Uniform(0.40, 4.80);
            default: return rng.Uniform(0.20, 2.00);
        }
    }

    double TopInjectionY(TRandom3& rng)
    {
        return rng.Uniform(kTopInjectionMinY, kTopInjectionMaxY);
    }

    double TopInjectionX(TRandom3& rng)
    {
        return rng.Uniform(-1.06 * kActiveHalfX, 1.06 * kActiveHalfX);
    }

    double TopInjectionZ(TRandom3& rng)
    {
        return rng.Uniform(-1.08 * kActiveHalfZ, 1.08 * kActiveHalfZ);
    }

    double DistributedCoordinate(TRandom3& rng, double core, double sigma, double lo, double hi)
    {
        if (rng.Rndm() < 0.74)
        {
            return rng.Uniform(lo, hi);
        }
        return Clamp(rng.Gaus(core, sigma), lo, hi);
    }

    void FillDownwardMomentum(TRandom3& rng,
                              double p,
                              double cos_min,
                              double cos_max,
                              double& px,
                              double& py,
                              double& pz)
    {
        const double cos_from_down = rng.Uniform(cos_min, cos_max);
        const double transverse = p * std::sqrt(std::max(0.0, 1.0 - cos_from_down * cos_from_down));
        const double phi = rng.Uniform(0.0, 2.0 * kPi);
        px = transverse * std::cos(phi);
        py = -p * cos_from_down;
        pz = transverse * std::sin(phi);
    }

    void AddPrimary(TRandom3& rng,
                    int pdg_code,
                    double p,
                    double x0,
                    double y0,
                    double z0,
                    double cos_min,
                    double cos_max,
                    int& nprimary,
                    int* pdg,
                    double* px,
                    double* py,
                    double* pz,
                    double* x,
                    double* y,
                    double* z,
                    double* t)
    {
        if (nprimary >= kMaxPrimaries)
        {
            return;
        }

        pdg[nprimary] = pdg_code;
        x[nprimary] = x0;
        y[nprimary] = y0;
        z[nprimary] = z0;
        t[nprimary] = rng.Uniform(-1.2e-6, 1.2e-6);
        FillDownwardMomentum(rng, p, cos_min, cos_max, px[nprimary], py[nprimary], pz[nprimary]);
        ++nprimary;
    }
}

void make_corsika_cosmics(const char* output = "data/corsika_cosmics.root",
                          int n_events = 100,
                          unsigned int seed = 0)
{
    TRandom3 rng(seed);
    TFile file(output, "RECREATE");
    TTree tree("corsika", "Synthetic CORSIKA-like cosmic primary tree");

    Int_t iev = 0;
    Int_t nprimary = 0;
    Int_t pdg[kMaxPrimaries] = {};
    Double_t px[kMaxPrimaries] = {};
    Double_t py[kMaxPrimaries] = {};
    Double_t pz[kMaxPrimaries] = {};
    Double_t x[kMaxPrimaries] = {};
    Double_t y[kMaxPrimaries] = {};
    Double_t z[kMaxPrimaries] = {};
    Double_t t[kMaxPrimaries] = {};

    tree.Branch("iev", &iev, "iev/I");
    tree.Branch("nprimary", &nprimary, "nprimary/I");
    tree.Branch("pdg", pdg, "pdg[nprimary]/I");
    tree.Branch("px", px, "px[nprimary]/D");
    tree.Branch("py", py, "py[nprimary]/D");
    tree.Branch("pz", pz, "pz[nprimary]/D");
    tree.Branch("x", x, "x[nprimary]/D");
    tree.Branch("y", y, "y[nprimary]/D");
    tree.Branch("z", z, "z[nprimary]/D");
    tree.Branch("t", t, "t[nprimary]/D");

    const int events_to_write = std::max(1, n_events);
    for (int event = 0; event < events_to_write; ++event)
    {
        iev = event;
        nprimary = 0;

        std::fill_n(pdg, kMaxPrimaries, 0);
        std::fill_n(px, kMaxPrimaries, 0.0);
        std::fill_n(py, kMaxPrimaries, 0.0);
        std::fill_n(pz, kMaxPrimaries, 0.0);
        std::fill_n(x, kMaxPrimaries, 0.0);
        std::fill_n(y, kMaxPrimaries, 0.0);
        std::fill_n(z, kMaxPrimaries, 0.0);
        std::fill_n(t, kMaxPrimaries, 0.0);

        const double shower_x = TopInjectionX(rng);
        const double shower_z = TopInjectionZ(rng);
        const int leading_muon = rng.Rndm() < 0.56 ? 13 : -13;
        AddPrimary(rng,
                   leading_muon,
                   rng.Uniform(3.0, 11.0),
                   shower_x,
                   TopInjectionY(rng),
                   shower_z,
                   0.88,
                   0.998,
                   nprimary,
                   pdg,
                   px,
                   py,
                   pz,
                   x,
                   y,
                   z,
                   t);

        if (rng.Rndm() < 0.32)
        {
            AddPrimary(rng,
                       rng.Rndm() < 0.58 ? 13 : -13,
                       rng.Uniform(1.6, 7.5),
                       DistributedCoordinate(rng,
                                             shower_x,
                                             0.28,
                                             -1.06 * kActiveHalfX,
                                             1.06 * kActiveHalfX),
                       TopInjectionY(rng),
                       DistributedCoordinate(rng,
                                             shower_z,
                                             0.65,
                                             -1.08 * kActiveHalfZ,
                                             1.08 * kActiveHalfZ),
                       0.84,
                       0.996,
                       nprimary,
                       pdg,
                       px,
                       py,
                       pz,
                       x,
                       y,
                       z,
                       t);
        }

        const int secondary_count = 8 + rng.Integer(13);
        for (int i = 0; i < secondary_count; ++i)
        {
            const int secondary_pdg = PickSecondaryPdg(rng);
            const double core_fraction = rng.Uniform(1.0, 2.2);
            AddPrimary(rng,
                       secondary_pdg,
                       PickMomentumGeV(rng, secondary_pdg),
                       DistributedCoordinate(rng,
                                             shower_x,
                                             0.36 * core_fraction,
                                             -1.06 * kActiveHalfX,
                                             1.06 * kActiveHalfX),
                       TopInjectionY(rng),
                       DistributedCoordinate(rng,
                                             shower_z,
                                             0.90 * core_fraction,
                                             -1.08 * kActiveHalfZ,
                                             1.08 * kActiveHalfZ),
                       0.72,
                       0.992,
                       nprimary,
                       pdg,
                       px,
                       py,
                       pz,
                       x,
                       y,
                       z,
                       t);
        }

        tree.Fill();
    }

    tree.Write();
}

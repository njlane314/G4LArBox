#include "TFile.h"
#include "TRandom3.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int kMaxParticles = 60;
    constexpr double kPi = 3.14159265358979323846;

    double MassGeV(int pdg)
    {
        switch (std::abs(pdg))
        {
            case 13: return 0.105658;
            case 22: return 0.0;
            case 111: return 0.134977;
            case 211: return 0.139570;
            case 321: return 0.493677;
            case 2112: return 0.939565;
            case 2212: return 0.938272;
            case 3122: return 1.115683;
            default: return 0.0;
        }
    }

    double EnergyGeV(int pdg, double px, double py, double pz)
    {
        const double mass = MassGeV(pdg);
        return std::sqrt(mass * mass + px * px + py * py + pz * pz);
    }

    void FillMomentum(TRandom3& rng,
                      double p,
                      double cos_min,
                      double cos_max,
                      double& px,
                      double& py,
                      double& pz)
    {
        const double cos_theta = rng.Uniform(cos_min, cos_max);
        const double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
        const double phi = rng.Uniform(0.0, 2.0 * kPi);
        px = p * sin_theta * std::cos(phi);
        py = p * sin_theta * std::sin(phi);
        pz = p * cos_theta;
    }

    void AddParticle(TRandom3& rng,
                     int pdg,
                     double pmin,
                     double pmax,
                     double cos_min,
                     double cos_max,
                     int& nf,
                     int* pdgf,
                     double* Ef,
                     double* pxf,
                     double* pyf,
                     double* pzf)
    {
        if (nf >= kMaxParticles)
        {
            return;
        }

        pdgf[nf] = pdg;
        FillMomentum(rng, rng.Uniform(pmin, pmax), cos_min, cos_max, pxf[nf], pyf[nf], pzf[nf]);
        Ef[nf] = EnergyGeV(pdg, pxf[nf], pyf[nf], pzf[nf]);
        ++nf;
    }

    int PickPion(TRandom3& rng)
    {
        const double u = rng.Rndm();
        if (u < 0.45) return 211;
        if (u < 0.90) return -211;
        return 111;
    }
}

void make_lambda_cc_good_gst(const char* output = "data/lambda_cc_good_gst.root",
                             int n_events = 100,
                             unsigned int seed = 0)
{
    TRandom3 rng(seed);
    TFile file(output, "RECREATE");
    TTree tree("gst", "Synthetic good-quality nu_mu CC Lambda GENIE gst tree");

    Int_t iev = 0;
    Int_t neu = 14;
    Int_t tgt = 1000180400;
    Int_t Z = 18;
    Int_t A = 40;
    Bool_t cc = true;
    Bool_t nc = false;
    Bool_t qel = false;
    Bool_t res = true;
    Bool_t dis = false;
    Bool_t coh = false;
    Bool_t nuel = false;
    Bool_t imd = false;
    Bool_t em = false;
    Double_t wght = 1.0;
    Double_t xs = 1.0e-39;
    Double_t Ev = 4.0;
    Double_t El = 1.2;
    Double_t pxl = 0.0;
    Double_t pyl = 0.0;
    Double_t pzl = 1.2;
    Double_t vtxx = 0.0;
    Double_t vtxy = 0.0;
    Double_t vtxz = 0.0;
    Double_t vtxt = 0.0;
    Int_t nf = 0;
    Int_t pdgf[kMaxParticles] = {};
    Double_t Ef[kMaxParticles] = {};
    Double_t pxf[kMaxParticles] = {};
    Double_t pyf[kMaxParticles] = {};
    Double_t pzf[kMaxParticles] = {};

    tree.Branch("iev", &iev, "iev/I");
    tree.Branch("neu", &neu, "neu/I");
    tree.Branch("tgt", &tgt, "tgt/I");
    tree.Branch("Z", &Z, "Z/I");
    tree.Branch("A", &A, "A/I");
    tree.Branch("cc", &cc, "cc/O");
    tree.Branch("nc", &nc, "nc/O");
    tree.Branch("qel", &qel, "qel/O");
    tree.Branch("res", &res, "res/O");
    tree.Branch("dis", &dis, "dis/O");
    tree.Branch("coh", &coh, "coh/O");
    tree.Branch("nuel", &nuel, "nuel/O");
    tree.Branch("imd", &imd, "imd/O");
    tree.Branch("em", &em, "em/O");
    tree.Branch("wght", &wght, "wght/D");
    tree.Branch("xs", &xs, "xs/D");
    tree.Branch("Ev", &Ev, "Ev/D");
    tree.Branch("El", &El, "El/D");
    tree.Branch("pxl", &pxl, "pxl/D");
    tree.Branch("pyl", &pyl, "pyl/D");
    tree.Branch("pzl", &pzl, "pzl/D");
    tree.Branch("vtxx", &vtxx, "vtxx/D");
    tree.Branch("vtxy", &vtxy, "vtxy/D");
    tree.Branch("vtxz", &vtxz, "vtxz/D");
    tree.Branch("vtxt", &vtxt, "vtxt/D");
    tree.Branch("nf", &nf, "nf/I");
    tree.Branch("pdgf", pdgf, "pdgf[60]/I");
    tree.Branch("Ef", Ef, "Ef[60]/D");
    tree.Branch("pxf", pxf, "pxf[60]/D");
    tree.Branch("pyf", pyf, "pyf[60]/D");
    tree.Branch("pzf", pzf, "pzf[60]/D");

    const int events_to_write = std::max(1, n_events);
    for (int event = 0; event < events_to_write; ++event)
    {
        iev = event;
        neu = 14;
        tgt = 1000180400;
        Z = 18;
        A = 40;
        cc = true;
        nc = false;
        qel = false;
        res = rng.Rndm() < 0.70;
        dis = !res;
        coh = false;
        nuel = false;
        imd = false;
        em = false;
        wght = 1.0;
        xs = rng.Uniform(0.2e-39, 1.8e-39);
        Ev = rng.Uniform(2.8, 6.2);
        nf = 0;

        std::fill_n(pdgf, kMaxParticles, 0);
        std::fill_n(Ef, kMaxParticles, 0.0);
        std::fill_n(pxf, kMaxParticles, 0.0);
        std::fill_n(pyf, kMaxParticles, 0.0);
        std::fill_n(pzf, kMaxParticles, 0.0);

        const double muon_pmax = std::max(0.75, std::min(2.65, 0.55 * Ev));
        FillMomentum(rng, rng.Uniform(0.55, muon_pmax), 0.70, 0.995, pxl, pyl, pzl);
        El = EnergyGeV(13, pxl, pyl, pzl);

        vtxx = rng.Uniform(-0.78, 0.78);
        vtxy = rng.Uniform(-0.68, 0.68);
        vtxz = rng.Uniform(-3.85, 3.85);
        vtxt = 0.0;

        AddParticle(rng, 3122, 0.32, 1.18, 0.05, 0.98, nf, pdgf, Ef, pxf, pyf, pzf);

        if (rng.Rndm() < 0.58)
        {
            AddParticle(rng, 321, 0.18, 0.95, -0.20, 0.95, nf, pdgf, Ef, pxf, pyf, pzf);
        }
        if (rng.Rndm() < 0.70)
        {
            AddParticle(rng, rng.Rndm() < 0.82 ? 2212 : 2112,
                        0.16, 0.82, -0.45, 0.98, nf, pdgf, Ef, pxf, pyf, pzf);
        }
        if (rng.Rndm() < 0.42)
        {
            AddParticle(rng, PickPion(rng), 0.10, 0.78, -0.55, 0.95, nf, pdgf, Ef, pxf, pyf, pzf);
        }
        if (rng.Rndm() < 0.15)
        {
            AddParticle(rng, 22, 0.05, 0.45, -0.40, 0.98, nf, pdgf, Ef, pxf, pyf, pzf);
        }

        double visible_energy = El;
        for (int i = 0; i < nf; ++i)
        {
            visible_energy += Ef[i];
        }
        Ev = std::max(Ev, std::min(7.0, visible_energy - 0.75 + rng.Uniform(0.10, 0.45)));

        tree.Fill();
    }

    tree.Write();
}

#include "TFile.h"
#include "TRandom3.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
    constexpr int kMaxParticles = 60;

    struct ParticleSpec
    {
        int pdg;
        double mass;
        double pmin;
        double pmax;
    };

    double MassGeV(int pdg)
    {
        switch (std::abs(pdg))
        {
            case 11: return 0.000511;
            case 13: return 0.105658;
            case 22: return 0.0;
            case 111: return 0.134977;
            case 211: return 0.139570;
            case 321: return 0.493677;
            case 2112: return 0.939565;
            case 2212: return 0.938272;
            default: return 0.0;
        }
    }

    double EnergyGeV(double mass, double px, double py, double pz)
    {
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
        const double phi = rng.Uniform(0.0, 6.283185307179586);
        px = p * sin_theta * std::cos(phi);
        py = p * sin_theta * std::sin(phi);
        pz = p * cos_theta;
    }

    void AddParticle(TRandom3& rng,
                     int pdg,
                     double momentum_min,
                     double momentum_max,
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

        const double momentum = rng.Uniform(momentum_min, momentum_max);
        double px = 0.0;
        double py = 0.0;
        double pz = 0.0;
        FillMomentum(rng, momentum, cos_min, cos_max, px, py, pz);

        pdgf[nf] = pdg;
        pxf[nf] = px;
        pyf[nf] = py;
        pzf[nf] = pz;
        Ef[nf] = EnergyGeV(MassGeV(pdg), px, py, pz);
        ++nf;
    }

    int PickPion(TRandom3& rng)
    {
        const double u = rng.Rndm();
        if (u < 0.42) return 211;
        if (u < 0.84) return -211;
        return 111;
    }

    int PickNeutrino(TRandom3& rng)
    {
        const double u = rng.Rndm();
        if (u < 0.80) return 14;
        if (u < 0.93) return -14;
        if (u < 0.985) return 12;
        return -12;
    }

    double PickFewGeVEnergy(TRandom3& rng)
    {
        if (rng.Rndm() < 0.70)
        {
            return rng.Uniform(1.0, 3.5);
        }
        return rng.Uniform(3.5, 5.5);
    }
}

void make_thesis_bnb_gst(const char* output = "data/thesis_bnb_like_gst.root",
                         int n_events = 100,
                         unsigned int seed = 0)
{
    TRandom3 rng(seed);
    TFile file(output, "RECREATE");
    TTree tree("gst", "Synthetic few-GeV random neutrino-argon GENIE gst tree");

    Int_t iev = 0;
    Int_t neu = 14;
    Int_t tgt = 1000180400;
    Int_t Z = 18;
    Int_t A = 40;
    Bool_t cc = true;
    Bool_t nc = false;
    Bool_t qel = false;
    Bool_t res = false;
    Bool_t dis = true;
    Bool_t coh = false;
    Bool_t nuel = false;
    Bool_t imd = false;
    Bool_t em = false;
    Double_t wght = 1.0;
    Double_t xs = 1.0e-38;
    Double_t Ev = 2.0;
    Double_t El = 0.0;
    Double_t pxl = 0.0;
    Double_t pyl = 0.0;
    Double_t pzl = 0.0;
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
        neu = PickNeutrino(rng);
        tgt = 1000180400;
        Z = 18;
        A = 40;
        cc = rng.Rndm() < 0.78;
        nc = !cc;
        qel = false;
        res = false;
        dis = false;
        coh = false;
        nuel = false;
        imd = false;
        em = false;
        wght = 1.0;
        xs = rng.Uniform(0.3e-38, 3.5e-38);
        Ev = PickFewGeVEnergy(rng);
        El = 0.0;
        pxl = 0.0;
        pyl = 0.0;
        pzl = 0.0;
        vtxx = rng.Uniform(-1.10, 1.10);
        vtxy = rng.Uniform(-0.95, 0.95);
        vtxz = rng.Uniform(-4.65, 4.65);
        vtxt = 0.0;
        nf = 0;

        std::fill_n(pdgf, kMaxParticles, 0);
        std::fill_n(Ef, kMaxParticles, 0.0);
        std::fill_n(pxf, kMaxParticles, 0.0);
        std::fill_n(pyf, kMaxParticles, 0.0);
        std::fill_n(pzf, kMaxParticles, 0.0);

        const double mode = rng.Rndm();
        if (mode < 0.22)
        {
            qel = true;
        }
        else if (mode < 0.60)
        {
            res = true;
        }
        else if (mode < 0.96)
        {
            dis = true;
        }
        else
        {
            coh = true;
        }

        if (cc)
        {
            const int lepton_pdg = std::abs(neu) == 12 ? 11 : 13;
            const double lepton_mass = MassGeV(lepton_pdg);
            El = rng.Uniform(std::max(lepton_mass + 0.05, 0.22 * Ev), 0.78 * Ev);
            const double lepton_p = std::sqrt(std::max(0.0, El * El - lepton_mass * lepton_mass));
            FillMomentum(rng, lepton_p, 0.72, 1.0, pxl, pyl, pzl);
        }

        if (qel)
        {
            const int nucleon = rng.Rndm() < 0.82 ? 2212 : 2112;
            AddParticle(rng, nucleon, 0.25, 1.25, -0.15, 1.0, nf, pdgf, Ef, pxf, pyf, pzf);
            if (rng.Rndm() < 0.18)
            {
                AddParticle(rng, 2212, 0.18, 0.75, -0.45, 0.95, nf, pdgf, Ef, pxf, pyf, pzf);
            }
        }
        else if (res)
        {
            AddParticle(rng, rng.Rndm() < 0.75 ? 2212 : 2112,
                        0.20, 1.10, -0.25, 1.0, nf, pdgf, Ef, pxf, pyf, pzf);
            AddParticle(rng, PickPion(rng), 0.12, 1.25, -0.35, 1.0, nf, pdgf, Ef, pxf, pyf, pzf);
            if (rng.Rndm() < 0.40)
            {
                AddParticle(rng, PickPion(rng), 0.10, 0.85, -0.55, 0.95, nf, pdgf, Ef, pxf, pyf, pzf);
            }
            if (rng.Rndm() < 0.22)
            {
                AddParticle(rng, 22, 0.05, 0.60, -0.30, 1.0, nf, pdgf, Ef, pxf, pyf, pzf);
            }
        }
        else if (dis)
        {
            const int multiplicity = 3 + rng.Integer(6);
            for (int i = 0; i < multiplicity; ++i)
            {
                const double choice = rng.Rndm();
                if (choice < 0.26)
                {
                    AddParticle(rng, 2212, 0.18, 1.35, -0.40, 1.0, nf, pdgf, Ef, pxf, pyf, pzf);
                }
                else if (choice < 0.38)
                {
                    AddParticle(rng, 2112, 0.15, 1.15, -0.55, 1.0, nf, pdgf, Ef, pxf, pyf, pzf);
                }
                else if (choice < 0.83)
                {
                    AddParticle(rng, PickPion(rng), 0.10, 1.50, -0.50, 1.0, nf, pdgf, Ef, pxf, pyf, pzf);
                }
                else if (choice < 0.93)
                {
                    AddParticle(rng, 22, 0.05, 0.90, -0.40, 1.0, nf, pdgf, Ef, pxf, pyf, pzf);
                }
                else
                {
                    AddParticle(rng, rng.Rndm() < 0.5 ? 321 : -321,
                                0.20, 1.00, -0.35, 1.0, nf, pdgf, Ef, pxf, pyf, pzf);
                }
            }
        }
        else
        {
            AddParticle(rng, PickPion(rng), 0.25, 1.60, 0.65, 1.0, nf, pdgf, Ef, pxf, pyf, pzf);
            if (rng.Rndm() < 0.25)
            {
                AddParticle(rng, 22, 0.05, 0.45, 0.40, 1.0, nf, pdgf, Ef, pxf, pyf, pzf);
            }
        }

        if (rng.Rndm() < 0.08)
        {
            AddParticle(rng, 22, 0.05, 0.40, -0.60, 0.90, nf, pdgf, Ef, pxf, pyf, pzf);
        }

        const double activity_slack = rng.Uniform(0.05, 0.50);
        double visible_energy = El + activity_slack;
        for (int i = 0; i < nf; ++i)
        {
            visible_energy += std::max(0.0, Ef[i] - MassGeV(pdgf[i]));
        }
        Ev = std::max(Ev, std::min(5.8, visible_energy));

        tree.Fill();
    }

    tree.Write();
}

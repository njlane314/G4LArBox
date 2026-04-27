#include "TFile.h"
#include "TRandom3.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int kMaxParticles = 60;
    constexpr double kMuonMassGeV = 0.105658;
    constexpr double kLambdaMassGeV = 1.115683;

    double Energy(double mass, double px, double py, double pz)
    {
        return std::sqrt(mass * mass + px * px + py * py + pz * pz);
    }
}

void make_lambda_cc_gst(const char* output = "data/lambda_cc_gst.root",
                        int n_events = 12,
                        unsigned int seed = 0)
{
    TRandom3 rng(seed);
    TFile file(output, "RECREATE");
    TTree tree("gst", "Synthetic nu_mu CC Lambda GENIE gst tree");

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
    Double_t Ev = 3.0;
    Double_t El = 1.4;
    Double_t pxl = 0.0;
    Double_t pyl = 0.0;
    Double_t pzl = 1.35;
    Double_t vtxx = 0.0;
    Double_t vtxy = 0.0;
    Double_t vtxz = 0.0;
    Double_t vtxt = 0.0;
    Int_t nf = 1;
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
        res = true;
        dis = false;
        coh = false;
        nuel = false;
        imd = false;
        em = false;
        wght = 1.0;
        xs = 1.0e-39;
        Ev = 3.0;
        nf = 1;

        std::fill_n(pdgf, kMaxParticles, 0);
        std::fill_n(Ef, kMaxParticles, 0.0);
        std::fill_n(pxf, kMaxParticles, 0.0);
        std::fill_n(pyf, kMaxParticles, 0.0);
        std::fill_n(pzf, kMaxParticles, 0.0);

        pxl = rng.Gaus(0.04, 0.05);
        pyl = rng.Gaus(0.02, 0.04);
        pzl = rng.Uniform(1.10, 1.70);
        El = Energy(kMuonMassGeV, pxl, pyl, pzl);

        vtxx = rng.Uniform(-0.70, 0.70);
        vtxy = rng.Uniform(-0.65, 0.65);
        vtxz = rng.Uniform(-3.75, 3.75);

        pdgf[0] = 3122;
        pxf[0] = rng.Uniform(-0.20, 0.35);
        pyf[0] = rng.Uniform(-0.18, 0.18);
        pzf[0] = rng.Uniform(0.55, 1.15);

        switch (event % 6)
        {
            case 0:
                vtxx = rng.Uniform(-0.75, 0.75);
                vtxy = rng.Uniform(-0.65, 0.65);
                vtxz = rng.Uniform(-3.75, 3.75);
                pxf[0] = rng.Uniform(-0.20, 0.35);
                pyf[0] = rng.Uniform(-0.18, 0.18);
                pzf[0] = rng.Uniform(0.55, 1.15);
                break;
            case 1:
                vtxx = rng.Uniform(1.18, 1.279);
                vtxy = rng.Uniform(-0.80, 0.80);
                vtxz = rng.Uniform(-4.40, 4.40);
                pxf[0] = rng.Uniform(1.90, 3.10);
                pyf[0] = rng.Uniform(-0.18, 0.18);
                pzf[0] = rng.Uniform(-0.20, 0.65);
                break;
            case 2:
                vtxx = rng.Uniform(-1.279, -1.18);
                vtxy = rng.Uniform(-0.80, 0.80);
                vtxz = rng.Uniform(-4.40, 4.40);
                pxf[0] = rng.Uniform(-3.10, -1.90);
                pyf[0] = rng.Uniform(-0.18, 0.18);
                pzf[0] = rng.Uniform(-0.20, 0.65);
                break;
            case 3:
                vtxx = rng.Uniform(-0.90, 0.90);
                vtxy = rng.Uniform(1.05, 1.162);
                vtxz = rng.Uniform(-4.20, 4.20);
                pxf[0] = rng.Uniform(-0.25, 0.25);
                pyf[0] = rng.Uniform(1.60, 2.60);
                pzf[0] = rng.Uniform(-0.15, 0.75);
                break;
            case 4:
                vtxx = rng.Uniform(-0.90, 0.90);
                vtxy = rng.Uniform(-0.80, 0.80);
                vtxz = rng.Uniform(4.95, 5.174);
                pxf[0] = rng.Uniform(-0.28, 0.28);
                pyf[0] = rng.Uniform(-0.18, 0.18);
                pzf[0] = rng.Uniform(2.20, 3.60);
                break;
            default:
                vtxx = rng.Uniform(-0.90, 0.90);
                vtxy = rng.Uniform(-0.80, 0.80);
                vtxz = rng.Uniform(-5.174, -4.95);
                pxf[0] = rng.Uniform(-0.28, 0.28);
                pyf[0] = rng.Uniform(-0.18, 0.18);
                pzf[0] = rng.Uniform(-3.60, -2.20);
                break;
        }

        Ef[0] = Energy(kLambdaMassGeV, pxf[0], pyf[0], pzf[0]);
        Ev = El + Ef[0] + rng.Uniform(0.15, 0.45);
        tree.Fill();
    }

    tree.Write();
}

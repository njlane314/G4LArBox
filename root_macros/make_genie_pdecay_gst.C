#include "TFile.h"
#include "TRandom3.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int kMaxParticles = 16;
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kProtonMassGeV = 0.9382720813;

    double MassGeV(int pdg)
    {
        switch (std::abs(pdg))
        {
            case 11: return 0.000510999;
            case 12: return 0.0;
            case 13: return 0.105658;
            case 14: return 0.0;
            case 111: return 0.134977;
            case 321: return 0.493677;
            default: return 0.0;
        }
    }

    double TwoBodyMomentum(double parent_mass, double mass1, double mass2)
    {
        const double sum = mass1 + mass2;
        const double diff = mass1 - mass2;
        const double term = (parent_mass * parent_mass - sum * sum) *
                            (parent_mass * parent_mass - diff * diff);
        return term > 0.0 ? std::sqrt(term) / (2.0 * parent_mass) : 0.0;
    }

    void Direction(TRandom3& rng, double p, double& px, double& py, double& pz)
    {
        const double costheta = rng.Uniform(-1.0, 1.0);
        const double sintheta = std::sqrt(std::max(0.0, 1.0 - costheta * costheta));
        const double phi = rng.Uniform(0.0, 2.0 * kPi);
        px = p * sintheta * std::cos(phi);
        py = p * sintheta * std::sin(phi);
        pz = p * costheta;
    }

    void AddParticle(int pdg,
                     double px,
                     double py,
                     double pz,
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

        const double mass = MassGeV(pdg);
        pdgf[nf] = pdg;
        pxf[nf] = px;
        pyf[nf] = py;
        pzf[nf] = pz;
        Ef[nf] = std::sqrt(mass * mass + px * px + py * py + pz * pz);
        ++nf;
    }

    void FillTwoBody(TRandom3& rng,
                     int pdg1,
                     int pdg2,
                     int& nf,
                     int* pdgf,
                     double* Ef,
                     double* pxf,
                     double* pyf,
                     double* pzf)
    {
        double px = 0.0;
        double py = 0.0;
        double pz = 0.0;
        const double p = TwoBodyMomentum(kProtonMassGeV, MassGeV(pdg1), MassGeV(pdg2));
        Direction(rng, p, px, py, pz);
        AddParticle(pdg1, px, py, pz, nf, pdgf, Ef, pxf, pyf, pzf);
        AddParticle(pdg2, -px, -py, -pz, nf, pdgf, Ef, pxf, pyf, pzf);
    }
}

void make_genie_pdecay_gst(const char* output = "data/genie_pdecay_gst.root",
                           int n_events = 100,
                           unsigned int seed = 0)
{
    TRandom3 rng(seed);
    TFile file(output, "RECREATE");
    TTree tree("gst", "Synthetic GENIE-like proton decay gst tree");

    Int_t iev = 0;
    Int_t neu = 0;
    Int_t tgt = 1000180400;
    Int_t Z = 18;
    Int_t A = 40;
    Bool_t cc = false;
    Bool_t nc = false;
    Bool_t qel = false;
    Bool_t res = false;
    Bool_t dis = false;
    Bool_t coh = false;
    Bool_t nuel = false;
    Bool_t imd = false;
    Bool_t em = false;
    Double_t wght = 1.0;
    Double_t xs = 0.0;
    Double_t Ev = 0.0;
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
    tree.Branch("pdgf", pdgf, "pdgf[16]/I");
    tree.Branch("Ef", Ef, "Ef[16]/D");
    tree.Branch("pxf", pxf, "pxf[16]/D");
    tree.Branch("pyf", pyf, "pyf[16]/D");
    tree.Branch("pzf", pzf, "pzf[16]/D");

    const int events_to_write = std::max(1, n_events);
    for (int event = 0; event < events_to_write; ++event)
    {
        iev = event;
        nf = 0;
        std::fill_n(pdgf, kMaxParticles, 0);
        std::fill_n(Ef, kMaxParticles, 0.0);
        std::fill_n(pxf, kMaxParticles, 0.0);
        std::fill_n(pyf, kMaxParticles, 0.0);
        std::fill_n(pzf, kMaxParticles, 0.0);

        vtxx = rng.Uniform(-1.0, 1.0);
        vtxy = rng.Uniform(-0.9, 0.9);
        vtxz = rng.Uniform(-4.8, 4.8);
        vtxt = 0.0;

        const double channel = rng.Rndm();
        if (channel < 0.60)
        {
            FillTwoBody(rng, 321, -14, nf, pdgf, Ef, pxf, pyf, pzf);
        }
        else if (channel < 0.82)
        {
            FillTwoBody(rng, -11, 111, nf, pdgf, Ef, pxf, pyf, pzf);
        }
        else
        {
            FillTwoBody(rng, -13, 111, nf, pdgf, Ef, pxf, pyf, pzf);
        }

        tree.Fill();
    }

    tree.Write();
}

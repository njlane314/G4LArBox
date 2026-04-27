#include "TCanvas.h"
#include "TColor.h"
#include "TFile.h"
#include "TH2D.h"
#include "TPad.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    constexpr double kHalfWidthCm = 128.175;
    constexpr double kHalfLengthCm = 518.4;

    void FillXZProjection(TH2D& hist,
                          const std::vector<double>& zs,
                          const std::vector<double>& ze,
                          const std::vector<double>& xs,
                          const std::vector<double>& xe,
                          const std::vector<double>& weights)
    {
        const std::size_t count = std::min({zs.size(), ze.size(), xs.size(), xe.size(), weights.size()});
        for (std::size_t i = 0; i < count; ++i)
        {
            const double z_cm = 0.05 * (zs[i] + ze[i]);
            const double x_cm = 0.05 * (xs[i] + xe[i]);
            hist.Fill(z_cm, x_cm, weights[i]);
        }
    }

    std::vector<double> ToDouble(const std::vector<int>& values)
    {
        std::vector<double> converted(values.begin(), values.end());
        return converted;
    }

    std::vector<double> Difference(const std::vector<int>& lhs, const std::vector<int>& rhs)
    {
        const std::size_t count = std::min(lhs.size(), rhs.size());
        std::vector<double> diff(count, 0.0);
        for (std::size_t i = 0; i < count; ++i)
        {
            diff[i] = lhs[i] - rhs[i];
        }
        return diff;
    }

    double Sum(const std::vector<int>& values)
    {
        double total = 0.0;
        for (int value : values)
        {
            total += value;
        }
        return total;
    }
}

void recombination_view(const char* input_file = "data/output.root",
                        int event_index = 0,
                        const char* output_file = "data/recombination_view_event0.pdf")
{
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kBird);

    TFile file(input_file, "READ");
    if (file.IsZombie())
    {
        std::cerr << "Failed to open " << input_file << std::endl;
        return;
    }

    auto* step_tree = dynamic_cast<TTree*>(file.Get("stepTree"));
    if (step_tree == nullptr)
    {
        std::cerr << "stepTree was not found in " << input_file << std::endl;
        return;
    }

    if (event_index < 0 || event_index >= step_tree->GetEntries())
    {
        std::cerr << "Requested event " << event_index
                  << " but file contains " << step_tree->GetEntries()
                  << " events." << std::endl;
        return;
    }

    std::vector<double>* xs = nullptr;
    std::vector<double>* zs = nullptr;
    std::vector<double>* xe = nullptr;
    std::vector<double>* ze = nullptr;
    std::vector<int>* nexc = nullptr;
    std::vector<int>* nion = nullptr;
    std::vector<int>* nopt = nullptr;
    std::vector<int>* ntherm = nullptr;

    step_tree->SetBranchAddress("xs", &xs);
    step_tree->SetBranchAddress("zs", &zs);
    step_tree->SetBranchAddress("xe", &xe);
    step_tree->SetBranchAddress("ze", &ze);
    step_tree->SetBranchAddress("nexc", &nexc);
    step_tree->SetBranchAddress("nion", &nion);
    step_tree->SetBranchAddress("nopt", &nopt);
    step_tree->SetBranchAddress("ntherm", &ntherm);
    step_tree->GetEntry(event_index);

    const std::vector<double> nexc_d = ToDouble(*nexc);
    const std::vector<double> nion_d = ToDouble(*nion);
    const std::vector<double> nopt_d = ToDouble(*nopt);
    const std::vector<double> ntherm_d = ToDouble(*ntherm);
    const std::vector<double> recombined_d = Difference(*nion, *ntherm);

    TH2D h_charge_before("h_charge_before",
                         "Charge Without Recombination;z [cm];x [cm]",
                         220, -kHalfLengthCm, kHalfLengthCm,
                         120, -kHalfWidthCm, kHalfWidthCm);
    TH2D h_charge_after("h_charge_after",
                        "Charge After Recombination;z [cm];x [cm]",
                        220, -kHalfLengthCm, kHalfLengthCm,
                        120, -kHalfWidthCm, kHalfWidthCm);
    TH2D h_charge_lost("h_charge_lost",
                       "Charge Shifted Into Light;z [cm];x [cm]",
                       220, -kHalfLengthCm, kHalfLengthCm,
                       120, -kHalfWidthCm, kHalfWidthCm);
    TH2D h_light_before("h_light_before",
                        "Light Without Recombination;z [cm];x [cm]",
                        220, -kHalfLengthCm, kHalfLengthCm,
                        120, -kHalfWidthCm, kHalfWidthCm);
    TH2D h_light_after("h_light_after",
                       "Light After Recombination;z [cm];x [cm]",
                       220, -kHalfLengthCm, kHalfLengthCm,
                       120, -kHalfWidthCm, kHalfWidthCm);
    TH2D h_light_gain("h_light_gain",
                      "Extra Light From Recombination;z [cm];x [cm]",
                      220, -kHalfLengthCm, kHalfLengthCm,
                      120, -kHalfWidthCm, kHalfWidthCm);

    FillXZProjection(h_charge_before, *zs, *ze, *xs, *xe, nion_d);
    FillXZProjection(h_charge_after, *zs, *ze, *xs, *xe, ntherm_d);
    FillXZProjection(h_charge_lost, *zs, *ze, *xs, *xe, recombined_d);
    FillXZProjection(h_light_before, *zs, *ze, *xs, *xe, nexc_d);
    FillXZProjection(h_light_after, *zs, *ze, *xs, *xe, nopt_d);
    FillXZProjection(h_light_gain, *zs, *ze, *xs, *xe, recombined_d);

    const std::string output_path(output_file);
    const char* output_dir = gSystem->DirName(output_path.c_str());
    if (output_dir != nullptr && std::string(output_dir) != ".")
    {
        gSystem->mkdir(output_dir, true);
    }

    TCanvas canvas("canvas", "Recombination Views", 1800, 1000);
    canvas.Divide(3, 2);

    canvas.cd(1);
    gPad->SetRightMargin(0.14);
    h_charge_before.Draw("COLZ");

    canvas.cd(2);
    gPad->SetRightMargin(0.14);
    h_charge_after.Draw("COLZ");

    canvas.cd(3);
    gPad->SetRightMargin(0.14);
    h_charge_lost.Draw("COLZ");

    canvas.cd(4);
    gPad->SetRightMargin(0.14);
    h_light_before.Draw("COLZ");

    canvas.cd(5);
    gPad->SetRightMargin(0.14);
    h_light_after.Draw("COLZ");

    canvas.cd(6);
    gPad->SetRightMargin(0.14);
    h_light_gain.Draw("COLZ");

    canvas.SaveAs(output_file);

    std::cout << "Saved " << output_file
              << " for event " << event_index
              << " | excitation-only light=" << Sum(*nexc)
              << " quanta"
              << ", charge before=" << Sum(*nion)
              << ", charge after=" << Sum(*ntherm)
              << ", extra recombination light=" << Sum(*nion) - Sum(*ntherm)
              << std::endl;
}

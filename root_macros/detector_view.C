#include "TCanvas.h"
#include "TColor.h"
#include "TFile.h"
#include "TH2D.h"
#include "TPad.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"
#include "TString.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    constexpr double kHalfWidthCm = 128.175;
    constexpr double kHalfHeightCm = 116.5;
    constexpr double kHalfLengthCm = 518.4;

    void FillProjection(TH2D& hist,
                        const std::vector<double>& axis_a_start,
                        const std::vector<double>& axis_a_end,
                        const std::vector<double>& axis_b_start,
                        const std::vector<double>& axis_b_end,
                        const std::vector<double>& weights)
    {
        const std::size_t count = std::min({axis_a_start.size(),
                                            axis_a_end.size(),
                                            axis_b_start.size(),
                                            axis_b_end.size(),
                                            weights.size()});

        for (std::size_t i = 0; i < count; ++i)
        {
            const double axis_a_cm = 0.05 * (axis_a_start[i] + axis_a_end[i]);
            const double axis_b_cm = 0.05 * (axis_b_start[i] + axis_b_end[i]);
            hist.Fill(axis_a_cm, axis_b_cm, weights[i]);
        }
    }
}

void detector_view(const char* input_file = "data/output.root",
                   int event_index = 0,
                   const char* output_file = "data/detector_view_event0.pdf")
{
    gStyle->SetOptStat(0);
    gStyle->SetPalette(kViridis);

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

    std::vector<double>* edep = nullptr;
    std::vector<double>* xs = nullptr;
    std::vector<double>* ys = nullptr;
    std::vector<double>* zs = nullptr;
    std::vector<double>* xe = nullptr;
    std::vector<double>* ye = nullptr;
    std::vector<double>* ze = nullptr;

    step_tree->SetBranchAddress("edep", &edep);
    step_tree->SetBranchAddress("xs", &xs);
    step_tree->SetBranchAddress("ys", &ys);
    step_tree->SetBranchAddress("zs", &zs);
    step_tree->SetBranchAddress("xe", &xe);
    step_tree->SetBranchAddress("ye", &ye);
    step_tree->SetBranchAddress("ze", &ze);
    step_tree->GetEntry(event_index);

    TH2D hxz("hxz",
             Form("Event %d: XZ View;z [cm];x [cm]", event_index),
             220, -kHalfLengthCm, kHalfLengthCm,
             120, -kHalfWidthCm, kHalfWidthCm);
    TH2D hyz("hyz",
             Form("Event %d: YZ View;z [cm];y [cm]", event_index),
             220, -kHalfLengthCm, kHalfLengthCm,
             120, -kHalfHeightCm, kHalfHeightCm);
    TH2D hxy("hxy",
             Form("Event %d: XY View;x [cm];y [cm]", event_index),
             120, -kHalfWidthCm, kHalfWidthCm,
             120, -kHalfHeightCm, kHalfHeightCm);

    FillProjection(hxz, *zs, *ze, *xs, *xe, *edep);
    FillProjection(hyz, *zs, *ze, *ys, *ye, *edep);
    FillProjection(hxy, *xs, *xe, *ys, *ye, *edep);

    const std::string output_path(output_file);
    const char* output_dir = gSystem->DirName(output_path.c_str());
    if (output_dir != nullptr && std::string(output_dir) != ".")
    {
        gSystem->mkdir(output_dir, true);
    }

    TCanvas canvas("canvas", "Detector Views", 1800, 600);
    canvas.Divide(3, 1);

    canvas.cd(1);
    gPad->SetRightMargin(0.14);
    hxz.Draw("COLZ");

    canvas.cd(2);
    gPad->SetRightMargin(0.14);
    hyz.Draw("COLZ");

    canvas.cd(3);
    gPad->SetRightMargin(0.14);
    hxy.Draw("COLZ");

    canvas.SaveAs(output_file);

    double total_edep = 0.0;
    for (double value : *edep)
    {
        total_edep += value;
    }

    std::cout << "Saved " << output_file
              << " for event " << event_index
              << " with total visible energy deposit " << total_edep
              << " MeV." << std::endl;
}

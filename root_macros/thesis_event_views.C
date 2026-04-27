#include "TArrow.h"
#include "TBox.h"
#include "TCanvas.h"
#include "TColor.h"
#include "TFile.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TPad.h"
#include "TPolyMarker.h"
#include "TPolyMarker3D.h"
#include "TPolyLine.h"
#include "TPolyLine3D.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace
{
    constexpr double kHalfWidthCm = 128.175;
    constexpr double kHalfHeightCm = 116.5;
    constexpr double kHalfLengthCm = 518.4;

    struct TrackPath
    {
        int pdg = 0;
        int parent_id = -1;
        std::vector<std::array<double, 3>> points;
    };

    int PdgColor(int pdg)
    {
        switch (pdg)
        {
            case 13:
            case -13:
                return kAzure + 2;
            case 2212:
                return kOrange + 7;
            case 211:
                return kRed + 1;
            case -211:
                return kMagenta + 2;
            case 11:
            case -11:
                return kViolet + 1;
            default:
                return kGray + 2;
        }
    }

    std::string PdgLabel(int pdg)
    {
        switch (pdg)
        {
            case 13:
                return "#mu^{-}";
            case -13:
                return "#mu^{+}";
            case 2212:
                return "p";
            case 211:
                return "#pi^{+}";
            case -211:
                return "#pi^{-}";
            case 11:
                return "e^{-}";
            case -11:
                return "e^{+}";
            default:
                return Form("PDG %d", pdg);
        }
    }

    bool IsVisiblePrimary(int pdg)
    {
        switch (std::abs(pdg))
        {
            case 12:
            case 14:
            case 16:
            case 22:
            case 111:
            case 2112:
                return false;
            default:
                return true;
        }
    }

    std::map<int, TrackPath> LoadPrimaryTracks(TTree& step_tree, int event_index)
    {
        std::vector<double>* xs = nullptr;
        std::vector<double>* ys = nullptr;
        std::vector<double>* zs = nullptr;
        std::vector<double>* xe = nullptr;
        std::vector<double>* ye = nullptr;
        std::vector<double>* ze = nullptr;
        std::vector<int>* parid = nullptr;
        std::vector<int>* trkid = nullptr;
        std::vector<int>* step_pdg = nullptr;

        step_tree.SetBranchAddress("xs", &xs);
        step_tree.SetBranchAddress("ys", &ys);
        step_tree.SetBranchAddress("zs", &zs);
        step_tree.SetBranchAddress("xe", &xe);
        step_tree.SetBranchAddress("ye", &ye);
        step_tree.SetBranchAddress("ze", &ze);
        step_tree.SetBranchAddress("parid", &parid);
        step_tree.SetBranchAddress("trkid", &trkid);
        step_tree.SetBranchAddress("step_pdg", &step_pdg);
        step_tree.GetEntry(event_index);

        std::map<int, TrackPath> tracks;
        const std::size_t count = std::min({xs->size(),
                                            ys->size(),
                                            zs->size(),
                                            xe->size(),
                                            ye->size(),
                                            ze->size(),
                                            parid->size(),
                                            trkid->size(),
                                            step_pdg->size()});

        for (std::size_t i = 0; i < count; ++i)
        {
            if ((*parid)[i] != 0)
            {
                continue;
            }

            if (!IsVisiblePrimary((*step_pdg)[i]))
            {
                continue;
            }

            TrackPath& path = tracks[(*trkid)[i]];
            path.pdg = (*step_pdg)[i];
            path.parent_id = (*parid)[i];
            if (path.points.empty())
            {
                path.points.push_back({0.1 * (*zs)[i], 0.1 * (*xs)[i], 0.1 * (*ys)[i]});
            }
            path.points.push_back({0.1 * (*ze)[i], 0.1 * (*xe)[i], 0.1 * (*ye)[i]});
        }

        return tracks;
    }

    std::array<double, 3> LoadVertex(TTree& truth_tree, int event_index)
    {
        double vertex_x = 0.0;
        double vertex_y = 0.0;
        double vertex_z = 0.0;
        truth_tree.SetBranchAddress("vertex_x", &vertex_x);
        truth_tree.SetBranchAddress("vertex_y", &vertex_y);
        truth_tree.SetBranchAddress("vertex_z", &vertex_z);
        truth_tree.GetEntry(event_index);
        return {0.1 * vertex_z, 0.1 * vertex_x, 0.1 * vertex_y};
    }

    void DrawDetectorBox3D()
    {
        const std::array<std::array<double, 3>, 8> corners = {{
            {-kHalfLengthCm, -kHalfWidthCm, -kHalfHeightCm},
            { kHalfLengthCm, -kHalfWidthCm, -kHalfHeightCm},
            { kHalfLengthCm,  kHalfWidthCm, -kHalfHeightCm},
            {-kHalfLengthCm,  kHalfWidthCm, -kHalfHeightCm},
            {-kHalfLengthCm, -kHalfWidthCm,  kHalfHeightCm},
            { kHalfLengthCm, -kHalfWidthCm,  kHalfHeightCm},
            { kHalfLengthCm,  kHalfWidthCm,  kHalfHeightCm},
            {-kHalfLengthCm,  kHalfWidthCm,  kHalfHeightCm}
        }};

        const int edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };

        for (const auto& edge : edges)
        {
            auto* line = new TPolyLine3D(2);
            line->SetPoint(0, corners[edge[0]][0], corners[edge[0]][1], corners[edge[0]][2]);
            line->SetPoint(1, corners[edge[1]][0], corners[edge[1]][1], corners[edge[1]][2]);
            line->SetLineColor(kGray + 1);
            line->SetLineStyle(1);
            line->SetLineWidth(1);
            line->Draw();
        }
    }

    void EnsureDirectory(const std::string& path)
    {
        const char* dir_name = gSystem->DirName(path.c_str());
        if (dir_name != nullptr && std::string(dir_name) != ".")
        {
            gSystem->mkdir(dir_name, true);
        }
    }
}

void thesis_event_views(const char* input_file = "data/output.root",
                        int event_index = 0,
                        const char* output_prefix = "data/thesis_bnb_like_event")
{
    gStyle->SetOptStat(0);

    TFile file(input_file, "READ");
    if (file.IsZombie())
    {
        std::cerr << "Failed to open " << input_file << std::endl;
        return;
    }

    auto* step_tree = dynamic_cast<TTree*>(file.Get("stepTree"));
    auto* truth_tree = dynamic_cast<TTree*>(file.Get("truthTree"));
    if (step_tree == nullptr || truth_tree == nullptr)
    {
        std::cerr << "Missing stepTree or truthTree in " << input_file << std::endl;
        return;
    }

    const auto tracks = LoadPrimaryTracks(*step_tree, event_index);
    const auto vertex = LoadVertex(*truth_tree, event_index);

    const std::string prefix(output_prefix);
    const std::string event3d_file = prefix + "_3d.pdf";
    const std::string eventxz_file = prefix + "_xz.pdf";
    EnsureDirectory(event3d_file);

    TCanvas c3d("c3d", "3D Event View", 1200, 700);
    auto* frame3d = new TH3D("frame3d",
                             Form("BNB-like Event %d in a MicroBooNE-scale active volume;z [cm];x [cm];y [cm]", event_index),
                             2, -kHalfLengthCm, kHalfLengthCm,
                             2, -kHalfWidthCm, kHalfWidthCm,
                             2, -kHalfHeightCm, kHalfHeightCm);
    frame3d->SetMinimum(0.0);
    frame3d->SetMaximum(1.0);
    frame3d->GetXaxis()->SetTitleOffset(1.3);
    frame3d->GetYaxis()->SetTitleOffset(1.4);
    frame3d->GetZaxis()->SetTitleOffset(1.2);
    frame3d->Draw();
    gPad->SetTheta(22.0);
    gPad->SetPhi(-38.0);
    DrawDetectorBox3D();

    for (const auto& [track_id, path] : tracks)
    {
        if (path.points.size() < 2 || !IsVisiblePrimary(path.pdg))
        {
            continue;
        }

        auto* line = new TPolyLine3D(static_cast<int>(path.points.size()));
        for (std::size_t i = 0; i < path.points.size(); ++i)
        {
            line->SetPoint(i, path.points[i][0], path.points[i][1], path.points[i][2]);
        }
        line->SetLineColor(PdgColor(path.pdg));
        line->SetLineWidth(3);
        line->Draw();
    }

    auto* vertex_marker = new TPolyMarker3D(1);
    vertex_marker->SetPoint(0, vertex[0], vertex[1], vertex[2]);
    vertex_marker->SetMarkerStyle(20);
    vertex_marker->SetMarkerSize(1.2);
    vertex_marker->SetMarkerColor(kBlack);
    vertex_marker->Draw();

    TLatex label3d;
    label3d.SetNDC();
    label3d.SetTextSize(0.032);
    label3d.DrawLatex(0.14, 0.92, "Synthetic #nu_{#mu}-Ar interaction, E_{#nu} = 2.6 GeV");
    c3d.SaveAs(event3d_file.c_str());

    TCanvas cxz("cxz", "XZ Event View", 1400, 430);
    auto* frame2d = new TH2D("frame2d",
                             Form("Event %d primary-track projection in the beam plane;z [cm];x [cm]", event_index),
                             2, -kHalfLengthCm, kHalfLengthCm,
                             2, -kHalfWidthCm, kHalfWidthCm);
    frame2d->GetXaxis()->SetTitleOffset(1.0);
    frame2d->GetYaxis()->SetTitleOffset(1.2);
    frame2d->Draw();

    auto* active_box = new TBox(-kHalfLengthCm, -kHalfWidthCm, kHalfLengthCm, kHalfWidthCm);
    active_box->SetLineColor(kGray + 1);
    active_box->SetLineStyle(1);
    active_box->SetFillStyle(0);
    active_box->Draw("same");

    TLegend legend(0.70, 0.67, 0.89, 0.88);
    legend.SetBorderSize(0);
    legend.SetFillStyle(0);
    std::vector<int> legend_pdgs;

    for (const auto& [track_id, path] : tracks)
    {
        if (path.points.size() < 2 || !IsVisiblePrimary(path.pdg))
        {
            continue;
        }

        auto* line = new TPolyLine(static_cast<int>(path.points.size()));
        for (std::size_t i = 0; i < path.points.size(); ++i)
        {
            line->SetPoint(i, path.points[i][0], path.points[i][1]);
        }
        line->SetLineColor(PdgColor(path.pdg));
        line->SetLineWidth(3);
        line->Draw();

        if (std::find(legend_pdgs.begin(), legend_pdgs.end(), path.pdg) == legend_pdgs.end())
        {
            legend.AddEntry(line, PdgLabel(path.pdg).c_str(), "l");
            legend_pdgs.push_back(path.pdg);
        }
    }

    auto* beam_arrow = new TArrow(-490.0, 112.0, -330.0, 112.0, 0.02, "|>");
    beam_arrow->SetLineColor(kBlack);
    beam_arrow->SetFillColor(kBlack);
    beam_arrow->SetLineWidth(2);
    beam_arrow->Draw();

    auto* vertex2d = new TPolyMarker(1);
    vertex2d->SetPoint(0, vertex[0], vertex[1]);
    vertex2d->SetMarkerStyle(20);
    vertex2d->SetMarkerSize(1.2);
    vertex2d->SetMarkerColor(kBlack);
    vertex2d->Draw();

    legend.Draw();

    TLatex label2d;
    label2d.SetTextSize(0.032);
    label2d.DrawLatex(-485.0, 117.0, "BNB-like #nu_{#mu} beam");
    label2d.DrawLatex(-505.0, -118.0, "MicroBooNE active volume");
    cxz.SaveAs(eventxz_file.c_str());
}

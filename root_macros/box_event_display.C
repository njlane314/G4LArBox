#include "TBox.h"
#include "TCanvas.h"
#include "TColor.h"
#include "TFile.h"
#include "TLatex.h"
#include "TLine.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"
#include "TString.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    constexpr double kHalfWidthCm = 128.175;
    constexpr double kHalfHeightCm = 116.5;
    constexpr double kHalfLengthCm = 518.4;
    constexpr double kCanvasWidth = 1600.0;
    constexpr double kCanvasHeight = 1120.0;
    constexpr double kTopHeight = 720.0;
    constexpr double kBottomHeight = kCanvasHeight - kTopHeight;

    struct StepSegment
    {
        int pdg = 0;
        int parent = -1;
        int track = -1;
        double edep = 0.0;
        double x1 = 0.0;
        double y1 = 0.0;
        double z1 = 0.0;
        double x2 = 0.0;
        double y2 = 0.0;
        double z2 = 0.0;
    };

    struct TruthInfo
    {
        bool has_truth = false;
        std::string source = "unknown";
        double vertex_x = 0.0;
        double vertex_y = 0.0;
        double vertex_z = 0.0;
        int genie_neu = 0;
        bool genie_cc = false;
        bool genie_nc = false;
        bool genie_qel = false;
        bool genie_res = false;
        bool genie_dis = false;
        bool genie_coh = false;
        double genie_ev = 0.0;
        std::vector<int> primary_pdg;
    };

    struct DrawLine
    {
        double x1 = 0.0;
        double y1 = 0.0;
        double x2 = 0.0;
        double y2 = 0.0;
        double depth = 0.0;
        double width = 1.0;
        double alpha = 1.0;
        int style = 1;
        Color_t color = kBlack;
    };

    double Clamp(double value, double lo, double hi)
    {
        return std::max(lo, std::min(value, hi));
    }

    bool IsFinite(const StepSegment& s)
    {
        return std::isfinite(s.x1) && std::isfinite(s.y1) && std::isfinite(s.z1) &&
               std::isfinite(s.x2) && std::isfinite(s.y2) && std::isfinite(s.z2);
    }

    bool IsInvisiblePdg(int pdg)
    {
        const int apdg = std::abs(pdg);
        return pdg == 0 || apdg == 12 || apdg == 14 || apdg == 16;
    }

    double LengthCm(const StepSegment& s)
    {
        const double dx = s.x2 - s.x1;
        const double dy = s.y2 - s.y1;
        const double dz = s.z2 - s.z1;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    Color_t ColorForPdg(int pdg)
    {
        switch (pdg)
        {
            case 2212:
                return TColor::GetColor(0, 70, 210);
            case 211:
                return TColor::GetColor(0, 210, 40);
            case -211:
                return TColor::GetColor(240, 45, 45);
            case 321:
                return TColor::GetColor(255, 145, 0);
            case -321:
                return TColor::GetColor(150, 50, 205);
            case 130:
                return TColor::GetColor(125, 75, 25);
            case 13:
                return TColor::GetColor(245, 0, 170);
            case -13:
                return TColor::GetColor(0, 185, 210);
            case 11:
                return TColor::GetColor(80, 40, 210);
            case -11:
                return TColor::GetColor(40, 125, 210);
            case 22:
                return TColor::GetColor(225, 175, 0);
            case 2112:
                return TColor::GetColor(95, 95, 95);
            default:
                break;
        }

        if (std::abs(pdg) >= 1000000000)
        {
            return TColor::GetColor(70, 145, 135);
        }
        return TColor::GetColor(60, 60, 60);
    }

    std::string PdgLabel(int pdg)
    {
        switch (pdg)
        {
            case 2212:
                return "p";
            case 211:
                return "#pi^{+}";
            case -211:
                return "#pi^{-}";
            case 321:
                return "K^{+}";
            case -321:
                return "K^{-}";
            case 130:
                return "K^{0}_{L}";
            case 13:
                return "#mu^{-}";
            case -13:
                return "#mu^{+}";
            case 11:
                return "e^{-}";
            case -11:
                return "e^{+}";
            case 22:
                return "#gamma";
            case 2112:
                return "n";
            default:
                break;
        }

        if (std::abs(pdg) >= 1000000000)
        {
            return "ion";
        }
        return Form("PDG %d", pdg);
    }

    std::string NeutrinoLabel(int pdg)
    {
        switch (pdg)
        {
            case 12:
                return "#nu_{e}";
            case -12:
                return "#bar{#nu}_{e}";
            case 14:
                return "#nu_{#mu}";
            case -14:
                return "#bar{#nu}_{#mu}";
            case 16:
                return "#nu_{#tau}";
            case -16:
                return "#bar{#nu}_{#tau}";
            default:
                return "#nu";
        }
    }

    std::string GenieModeLabel(const TruthInfo& truth)
    {
        if (truth.genie_qel)
        {
            return "QEL";
        }
        if (truth.genie_res)
        {
            return "RES";
        }
        if (truth.genie_dis)
        {
            return "DIS";
        }
        if (truth.genie_coh)
        {
            return "COH";
        }
        if (truth.genie_cc)
        {
            return "CC";
        }
        if (truth.genie_nc)
        {
            return "NC";
        }
        return "interaction";
    }

    std::string EventLabel(const TruthInfo& truth)
    {
        if (truth.source == "genie_gst")
        {
            std::ostringstream label;
            label << "GENIE " << NeutrinoLabel(truth.genie_neu) << "-Ar "
                  << GenieModeLabel(truth);
            if (truth.genie_ev > 0.0)
            {
                label << ", E_{#nu} = " << Form("%.2f", truth.genie_ev) << " GeV";
            }
            return label.str();
        }

        std::string source_lower = truth.source;
        std::transform(source_lower.begin(), source_lower.end(), source_lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (source_lower.find("marley") != std::string::npos)
        {
            return "MARLEY #nu_{e}-Ar event";
        }

        if (truth.source == "gps")
        {
            return "Geant4 GPS event";
        }

        return "G4LArBox event";
    }

    void EnsureDirectory(const std::string& path)
    {
        const char* dir_name = gSystem->DirName(path.c_str());
        if (dir_name != nullptr && std::string(dir_name) != ".")
        {
            gSystem->mkdir(dir_name, true);
        }
    }

    std::vector<std::string> SplitFormats(const char* format_list)
    {
        std::vector<std::string> formats;
        std::stringstream input(format_list == nullptr ? "" : format_list);
        std::string item;

        while (std::getline(input, item, ','))
        {
            item.erase(std::remove_if(item.begin(),
                                      item.end(),
                                      [](unsigned char c) { return std::isspace(c); }),
                       item.end());
            if (!item.empty())
            {
                formats.push_back(item);
            }
        }

        if (formats.empty())
        {
            formats.push_back("pdf");
        }
        return formats;
    }

    void ProjectPerspective(double x,
                            double y,
                            double z,
                            double& px,
                            double& py,
                            double& depth)
    {
        depth = z + 0.35 * x - 0.20 * y;
        px = 135.0 + (z + kHalfLengthCm) * 1.18 + x * 0.82;
        py = 505.0 - y * 1.35 - (z + kHalfLengthCm) * 0.10 + x * 0.25;
    }

    void ProjectXZ(double x,
                   double y,
                   double z,
                   double& px,
                   double& py,
                   double& depth)
    {
        (void)y;
        depth = z;
        px = 92.0 + (z + kHalfLengthCm) * ((kCanvasWidth - 184.0) / (2.0 * kHalfLengthCm));
        py = 220.0 + x * ((kBottomHeight - 150.0) / (2.0 * kHalfWidthCm));
    }

    void DrawPadBackground(double width, double height)
    {
        auto* bg = new TBox(0.0, 0.0, width, height);
        bg->SetFillColor(kWhite);
        bg->SetLineColor(kWhite);
        bg->Draw();
    }

    void DrawDetectorBox(bool xz_view)
    {
        if (xz_view)
        {
            auto* active = new TBox(92.0,
                                    220.0 - (kBottomHeight - 150.0) / 2.0,
                                    kCanvasWidth - 92.0,
                                    220.0 + (kBottomHeight - 150.0) / 2.0);
            active->SetFillStyle(0);
            active->SetLineColorAlpha(kGray + 2, 0.70);
            active->SetLineWidth(2);
            active->Draw();
            return;
        }

        const std::array<std::array<double, 3>, 8> corners = {{
            {-kHalfWidthCm, -kHalfHeightCm, -kHalfLengthCm},
            { kHalfWidthCm, -kHalfHeightCm, -kHalfLengthCm},
            { kHalfWidthCm,  kHalfHeightCm, -kHalfLengthCm},
            {-kHalfWidthCm,  kHalfHeightCm, -kHalfLengthCm},
            {-kHalfWidthCm, -kHalfHeightCm,  kHalfLengthCm},
            { kHalfWidthCm, -kHalfHeightCm,  kHalfLengthCm},
            { kHalfWidthCm,  kHalfHeightCm,  kHalfLengthCm},
            {-kHalfWidthCm,  kHalfHeightCm,  kHalfLengthCm}
        }};
        const int edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };

        for (const auto& edge : edges)
        {
            double x1 = 0.0;
            double y1 = 0.0;
            double d1 = 0.0;
            double x2 = 0.0;
            double y2 = 0.0;
            double d2 = 0.0;
            ProjectPerspective(corners[edge[0]][0],
                               corners[edge[0]][1],
                               corners[edge[0]][2],
                               x1,
                               y1,
                               d1);
            ProjectPerspective(corners[edge[1]][0],
                               corners[edge[1]][1],
                               corners[edge[1]][2],
                               x2,
                               y2,
                               d2);

            auto* line = new TLine(x1, y1, x2, y2);
            line->SetLineColorAlpha(TColor::GetColor(42, 95, 170), 0.72);
            line->SetLineStyle(1);
            line->SetLineWidth(2);
            line->Draw();
        }
    }

    void AddTrackLines(std::vector<DrawLine>& lines,
                       const std::vector<StepSegment>& segments,
                       bool xz_view)
    {
        for (const auto& s : segments)
        {
            if (IsInvisiblePdg(s.pdg) || !IsFinite(s) || LengthCm(s) < 0.01)
            {
                continue;
            }

            DrawLine line;
            double d1 = 0.0;
            double d2 = 0.0;
            if (xz_view)
            {
                ProjectXZ(s.x1, s.y1, s.z1, line.x1, line.y1, d1);
                ProjectXZ(s.x2, s.y2, s.z2, line.x2, line.y2, d2);
            }
            else
            {
                ProjectPerspective(s.x1, s.y1, s.z1, line.x1, line.y1, d1);
                ProjectPerspective(s.x2, s.y2, s.z2, line.x2, line.y2, d2);
            }

            line.depth = 0.5 * (d1 + d2);
            line.color = ColorForPdg(s.pdg);
            line.width = s.parent == 0 ? 4.0 : 2.0;
            line.alpha = s.parent == 0 ? 0.96 : 0.58;
            if (std::abs(s.pdg) == 22 || s.pdg == 2112)
            {
                line.alpha = s.parent == 0 ? 0.70 : 0.42;
                line.style = 7;
            }
            lines.push_back(line);
        }
    }

    void DrawLines(std::vector<DrawLine>& lines)
    {
        std::sort(lines.begin(), lines.end(), [](const DrawLine& a, const DrawLine& b) {
            return a.depth < b.depth;
        });

        for (const auto& line : lines)
        {
            auto* drawable = new TLine(line.x1, line.y1, line.x2, line.y2);
            drawable->SetLineColorAlpha(line.color, Clamp(line.alpha, 0.0, 1.0));
            drawable->SetLineWidth(std::max(1, static_cast<int>(std::lround(line.width))));
            drawable->SetLineStyle(line.style);
            drawable->Draw();
        }
    }

    void DrawVertex(const TruthInfo& truth, bool xz_view)
    {
        if (!truth.has_truth)
        {
            return;
        }

        double px = 0.0;
        double py = 0.0;
        double depth = 0.0;
        if (xz_view)
        {
            ProjectXZ(truth.vertex_x, truth.vertex_y, truth.vertex_z, px, py, depth);
        }
        else
        {
            ProjectPerspective(truth.vertex_x, truth.vertex_y, truth.vertex_z, px, py, depth);
        }

        auto* cross_h = new TLine(px - 7.0, py, px + 7.0, py);
        auto* cross_v = new TLine(px, py - 7.0, px, py + 7.0);
        cross_h->SetLineColor(kBlack);
        cross_v->SetLineColor(kBlack);
        cross_h->SetLineWidth(2);
        cross_v->SetLineWidth(2);
        cross_h->Draw();
        cross_v->Draw();
    }

    void DrawLegend(const std::vector<StepSegment>& segments, double x, double y)
    {
        std::set<int> present;
        for (const auto& s : segments)
        {
            if (!IsInvisiblePdg(s.pdg))
            {
                present.insert(s.pdg);
            }
        }

        const std::vector<int> preferred = {
            2212, 211, -211, 321, -321, 130, 13, -13, 11, -11, 22, 2112
        };

        TLatex label;
        label.SetTextFont(42);
        label.SetTextSize(0.030);
        label.SetTextAlign(12);

        int rows = 0;
        for (int pdg : preferred)
        {
            if (present.find(pdg) == present.end())
            {
                continue;
            }

            const double yy = y - rows * 30.0;
            auto* line = new TLine(x, yy, x + 42.0, yy);
            line->SetLineColor(ColorForPdg(pdg));
            line->SetLineWidth(4);
            if (std::abs(pdg) == 22 || pdg == 2112)
            {
                line->SetLineStyle(7);
            }
            line->Draw();
            label.DrawLatex(x + 54.0, yy, PdgLabel(pdg).c_str());
            ++rows;

            if (rows >= 8)
            {
                break;
            }
        }
    }

    bool LoadSegments(TTree& step_tree,
                      int event_index,
                      std::vector<StepSegment>& segments,
                      int max_segments)
    {
        if (event_index < 0 || event_index >= step_tree.GetEntries())
        {
            std::cerr << "Requested event " << event_index
                      << " but stepTree contains " << step_tree.GetEntries()
                      << " events." << std::endl;
            return false;
        }

        std::vector<double>* edep = nullptr;
        std::vector<double>* xs = nullptr;
        std::vector<double>* ys = nullptr;
        std::vector<double>* zs = nullptr;
        std::vector<double>* xe = nullptr;
        std::vector<double>* ye = nullptr;
        std::vector<double>* ze = nullptr;
        std::vector<int>* parid = nullptr;
        std::vector<int>* trkid = nullptr;
        std::vector<int>* step_pdg = nullptr;

        step_tree.SetBranchAddress("edep", &edep);
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

        const std::size_t count = std::min({edep->size(),
                                            xs->size(),
                                            ys->size(),
                                            zs->size(),
                                            xe->size(),
                                            ye->size(),
                                            ze->size(),
                                            parid->size(),
                                            trkid->size(),
                                            step_pdg->size()});
        segments.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            if (max_segments > 0 && static_cast<int>(segments.size()) >= max_segments)
            {
                break;
            }

            StepSegment s;
            s.pdg = (*step_pdg)[i];
            s.parent = (*parid)[i];
            s.track = (*trkid)[i];
            s.edep = (*edep)[i];
            s.x1 = 0.1 * (*xs)[i];
            s.y1 = 0.1 * (*ys)[i];
            s.z1 = 0.1 * (*zs)[i];
            s.x2 = 0.1 * (*xe)[i];
            s.y2 = 0.1 * (*ye)[i];
            s.z2 = 0.1 * (*ze)[i];
            segments.push_back(s);
        }

        return true;
    }

    TruthInfo LoadTruth(TTree* truth_tree, int event_index)
    {
        TruthInfo truth;
        if (truth_tree == nullptr || event_index < 0 || event_index >= truth_tree->GetEntries())
        {
            return truth;
        }

        std::string* source = nullptr;
        std::vector<int>* primary_pdg = nullptr;

        if (truth_tree->GetBranch("source"))
        {
            truth_tree->SetBranchAddress("source", &source);
        }
        if (truth_tree->GetBranch("vertex_x"))
        {
            truth_tree->SetBranchAddress("vertex_x", &truth.vertex_x);
        }
        if (truth_tree->GetBranch("vertex_y"))
        {
            truth_tree->SetBranchAddress("vertex_y", &truth.vertex_y);
        }
        if (truth_tree->GetBranch("vertex_z"))
        {
            truth_tree->SetBranchAddress("vertex_z", &truth.vertex_z);
        }
        if (truth_tree->GetBranch("genie_neu"))
        {
            truth_tree->SetBranchAddress("genie_neu", &truth.genie_neu);
        }
        if (truth_tree->GetBranch("genie_cc"))
        {
            truth_tree->SetBranchAddress("genie_cc", &truth.genie_cc);
        }
        if (truth_tree->GetBranch("genie_nc"))
        {
            truth_tree->SetBranchAddress("genie_nc", &truth.genie_nc);
        }
        if (truth_tree->GetBranch("genie_qel"))
        {
            truth_tree->SetBranchAddress("genie_qel", &truth.genie_qel);
        }
        if (truth_tree->GetBranch("genie_res"))
        {
            truth_tree->SetBranchAddress("genie_res", &truth.genie_res);
        }
        if (truth_tree->GetBranch("genie_dis"))
        {
            truth_tree->SetBranchAddress("genie_dis", &truth.genie_dis);
        }
        if (truth_tree->GetBranch("genie_coh"))
        {
            truth_tree->SetBranchAddress("genie_coh", &truth.genie_coh);
        }
        if (truth_tree->GetBranch("genie_ev"))
        {
            truth_tree->SetBranchAddress("genie_ev", &truth.genie_ev);
        }
        if (truth_tree->GetBranch("primary_pdg"))
        {
            truth_tree->SetBranchAddress("primary_pdg", &primary_pdg);
        }

        truth_tree->GetEntry(event_index);
        truth.has_truth = true;
        truth.vertex_x *= 0.1;
        truth.vertex_y *= 0.1;
        truth.vertex_z *= 0.1;

        if (source != nullptr)
        {
            truth.source = *source;
        }
        if (primary_pdg != nullptr)
        {
            truth.primary_pdg = *primary_pdg;
        }
        return truth;
    }

    void DrawBeamArrow()
    {
        auto* line = new TLine(120.0, 335.0, 270.0, 335.0);
        line->SetLineColor(kBlack);
        line->SetLineWidth(3);
        line->Draw();

        auto* head1 = new TLine(270.0, 335.0, 247.0, 348.0);
        auto* head2 = new TLine(270.0, 335.0, 247.0, 322.0);
        head1->SetLineColor(kBlack);
        head2->SetLineColor(kBlack);
        head1->SetLineWidth(3);
        head2->SetLineWidth(3);
        head1->Draw();
        head2->Draw();
    }

    void DrawTopLabels(const TruthInfo& truth, int event_index)
    {
        TLatex latex;
        latex.SetTextFont(42);
        latex.SetTextAlign(12);
        latex.SetTextSize(0.034);
        latex.DrawLatex(74.0, 675.0, Form("Event %d: %s", event_index, EventLabel(truth).c_str()));
        latex.SetTextSize(0.026);
        latex.DrawLatex(74.0, 640.0, "MicroBooNE TPCActive: 2.5635 m #times 2.33 m #times 10.368 m");
        latex.DrawLatex(1220.0, 640.0, "3D perspective");
    }

    void DrawBottomLabels()
    {
        TLatex latex;
        latex.SetTextFont(42);
        latex.SetTextAlign(12);

        latex.SetTextSize(0.030);
        latex.DrawLatex(74.0, 356.0, "Beam-plane projection");
        latex.SetTextSize(0.026);
        latex.DrawLatex(285.0, 335.0, "+z");
        latex.DrawLatex(88.0, 62.0, "box width x [cm]");
        latex.DrawLatex(1260.0, 62.0, "box length z [cm]");
    }

    void RenderDisplay(const std::vector<StepSegment>& segments,
                       const TruthInfo& truth,
                       int event_index,
                       const std::string& output_prefix,
                       const std::vector<std::string>& formats)
    {
        auto* canvas = new TCanvas("box_event_display",
                                   "G4LArBox event display",
                                   static_cast<int>(kCanvasWidth),
                                   static_cast<int>(kCanvasHeight));
        canvas->SetMargin(0.0, 0.0, 0.0, 0.0);
        canvas->Range(0.0, 0.0, kCanvasWidth, kCanvasHeight);

        auto* top = new TPad("top", "top", 0.0, kBottomHeight / kCanvasHeight, 1.0, 1.0);
        auto* bottom = new TPad("bottom", "bottom", 0.0, 0.0, 1.0, kBottomHeight / kCanvasHeight);
        top->SetMargin(0.0, 0.0, 0.0, 0.0);
        bottom->SetMargin(0.0, 0.0, 0.0, 0.0);
        top->Draw();
        bottom->Draw();

        top->cd();
        top->Range(0.0, 0.0, kCanvasWidth, kTopHeight);
        DrawPadBackground(kCanvasWidth, kTopHeight);
        DrawDetectorBox(false);
        std::vector<DrawLine> perspective_lines;
        AddTrackLines(perspective_lines, segments, false);
        DrawLines(perspective_lines);
        DrawVertex(truth, false);
        DrawLegend(segments, 1260.0, 565.0);
        DrawTopLabels(truth, event_index);

        bottom->cd();
        bottom->Range(0.0, 0.0, kCanvasWidth, kBottomHeight);
        DrawPadBackground(kCanvasWidth, kBottomHeight);
        DrawDetectorBox(true);
        std::vector<DrawLine> xz_lines;
        AddTrackLines(xz_lines, segments, true);
        DrawLines(xz_lines);
        DrawVertex(truth, true);
        DrawBeamArrow();
        DrawBottomLabels();

        for (const auto& format : formats)
        {
            const std::string output = output_prefix + "_display." + format;
            EnsureDirectory(output);
            gSystem->Unlink(output.c_str());
            canvas->Print(output.c_str());
            if (gSystem->AccessPathName(output.c_str()))
            {
                std::cerr << "ROOT did not create " << output
                          << "; this build may not support that graphics backend." << std::endl;
            }
            else
            {
                std::cout << "Saved " << output << std::endl;
            }
        }

        delete canvas;
    }
}

void box_event_display(const char* input_file = "data/output.root",
                       int event_index = 0,
                       const char* output_prefix = "data/event_displays/box_event0",
                       const char* output_formats = "pdf",
                       int max_segments = 60000)
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);
    gStyle->SetLineScalePS(1.0);

    TFile file(input_file, "READ");
    if (file.IsZombie())
    {
        std::cerr << "Failed to open " << input_file << std::endl;
        return;
    }

    auto* step_tree = dynamic_cast<TTree*>(file.Get("stepTree"));
    if (step_tree == nullptr)
    {
        std::cerr << "Missing stepTree in " << input_file << std::endl;
        return;
    }

    std::vector<StepSegment> segments;
    if (!LoadSegments(*step_tree, event_index, segments, max_segments))
    {
        return;
    }

    auto* truth_tree = dynamic_cast<TTree*>(file.Get("truthTree"));
    const TruthInfo truth = LoadTruth(truth_tree, event_index);

    if (segments.empty())
    {
        std::cerr << "Event " << event_index << " contains no step segments." << std::endl;
        return;
    }

    RenderDisplay(segments,
                  truth,
                  event_index,
                  output_prefix,
                  SplitFormats(output_formats));
}

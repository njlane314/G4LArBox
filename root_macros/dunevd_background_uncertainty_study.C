#include "TArrow.h"
#include "TBox.h"
#include "TCanvas.h"
#include "TColor.h"
#include "TEllipse.h"
#include "TFile.h"
#include "TGraph.h"
#include "TGraphAsymmErrors.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMarker.h"
#include "TRandom3.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

namespace
{
    constexpr double kZMinCm = -104.0305;
    constexpr double kZMaxCm = 2195.6405;
    constexpr double kYMinCm = -781.26;
    constexpr double kYMaxCm = 781.26;
    constexpr double kXMinCm = -425.0;
    constexpr double kXMaxCm = 425.0;
    constexpr double kPadLeftMargin = 0.12;
    constexpr double kPadRightMargin = 0.06;
    constexpr double kPadBottomMargin = 0.12;
    constexpr double kPadTopMargin = 0.06;
    constexpr double kEventDisplayXMinCm = kZMinCm - 180.0;
    constexpr double kEventDisplayXMaxCm = kZMaxCm + 220.0;
    constexpr double kEventDisplayYMinCm = kYMinCm - 130.0;
    constexpr double kEventDisplayYMaxCm = kYMaxCm + 130.0;
    constexpr int kEventDisplayCanvasWidthPx = 980;

    int Color(const char* hex)
    {
        return TColor::GetColor(hex);
    }

    struct Scenario
    {
        std::string name;
        double rock_scale;
        double rock_hardness;
        double radio_scale;
        double dark_scale;
        double light_scale;
        double baseline_sigma_adc;
        double prior_width;
    };

    struct SampleDef
    {
        std::string name;
        double signal_prompt_adc;
        double signal_delayed_adc;
        double rock_mean;
        double radio_mean;
        double neutron_energy_mean_mev;
        bool has_true_delay;
    };

    void SetRootPaperStyle(bool show_stats = false)
    {
        gStyle->SetCanvasColor(kWhite);
        gStyle->SetFrameFillColor(kWhite);
        gStyle->SetPadColor(kWhite);
        gStyle->SetTitleFillColor(kWhite);
        gStyle->SetTitleBorderSize(0);
        gStyle->SetOptTitle(0);
        gStyle->SetOptStat(show_stats ? 1110 : 0);
        gStyle->SetStatColor(kWhite);
        gStyle->SetStatBorderSize(1);
        gStyle->SetStatFont(42);
        gStyle->SetStatTextColor(Color("#111827"));
        gStyle->SetLabelFont(42, "XYZ");
        gStyle->SetTitleFont(42, "XYZ");
        gStyle->SetTextFont(42);
        gStyle->SetLabelSize(0.038, "XYZ");
        gStyle->SetTitleSize(0.042, "XYZ");
        gStyle->SetTitleOffset(1.15, "X");
        gStyle->SetTitleOffset(1.22, "Y");
        gStyle->SetPadLeftMargin(kPadLeftMargin);
        gStyle->SetPadRightMargin(kPadRightMargin);
        gStyle->SetPadBottomMargin(kPadBottomMargin);
        gStyle->SetPadTopMargin(kPadTopMargin);
        gStyle->SetGridColor(Color("#cbd5e1"));
        gStyle->SetGridStyle(1);
        gStyle->SetGridWidth(1);
    }

    int EventDisplayCanvasHeightPx()
    {
        const double x_span = kEventDisplayXMaxCm - kEventDisplayXMinCm;
        const double y_span = kEventDisplayYMaxCm - kEventDisplayYMinCm;
        const double data_aspect = x_span / y_span;
        const double drawn_width = kEventDisplayCanvasWidthPx * (1.0 - kPadLeftMargin - kPadRightMargin);
        const double drawn_height = drawn_width / data_aspect;
        const double canvas_height = drawn_height / (1.0 - kPadBottomMargin - kPadTopMargin);
        return static_cast<int>(std::ceil(canvas_height));
    }

    void EnsureDirectory(const std::string& path)
    {
        gSystem->mkdir(path.c_str(), true);
    }

    void SaveCanvas(TCanvas& canvas, const std::string& prefix)
    {
        canvas.SaveAs((prefix + ".pdf").c_str());
        canvas.SaveAs((prefix + ".png").c_str());
    }

    double Pulse(double t, double t0, double amp, double sigma)
    {
        const double z = (t - t0) / sigma;
        return amp * std::exp(-0.5 * z * z);
    }

    double ExponentialSample(TRandom3& rng, double mean)
    {
        return -mean * std::log(std::max(1e-12, rng.Uniform()));
    }

    double RadiogenicNeutronEnergy(TRandom3& rng, double mean_mev, double hardness)
    {
        const double shape = std::max(0.75, 1.25 * hardness);
        const double u = std::max(1e-12, rng.Uniform());
        const double sample = mean_mev * std::pow(-std::log(u), 1.0 / shape);
        return std::min(20.0, std::max(0.02, sample));
    }

    double RandomUniform(TRandom3& rng, double min_value, double max_value)
    {
        return min_value + (max_value - min_value) * rng.Uniform();
    }

    void DrawThesisLabel(double x, double y, const char* text, double size = 0.035)
    {
        TLatex latex;
        latex.SetNDC();
        latex.SetTextFont(42);
        latex.SetTextSize(size);
        latex.SetTextColor(Color("#111827"));
        latex.DrawLatex(x, y, text);
    }

    void DrawVdEnvelope()
    {
        auto* active = new TBox(kZMinCm, kYMinCm, kZMaxCm, kYMaxCm);
        active->SetFillStyle(0);
        active->SetLineColor(Color("#f59e0b"));
        active->SetLineWidth(2);
        active->Draw("l");

        for (int i = 0; i <= 8; ++i)
        {
            const double z = kZMinCm + i * (kZMaxCm - kZMinCm) / 8.0;
            auto* line = new TLine(z, kYMinCm, z, kYMaxCm);
            line->SetLineColorAlpha(Color("#fbbf24"), 0.22);
            line->SetLineStyle(3);
            line->Draw();
        }

        for (int j = 1; j < 5; ++j)
        {
            const double y = kYMinCm + j * (kYMaxCm - kYMinCm) / 5.0;
            auto* line = new TLine(kZMinCm, y, kZMaxCm, y);
            line->SetLineColorAlpha(Color("#94a3b8"), 0.18);
            line->SetLineStyle(3);
            line->Draw();
        }

        for (int iz = 0; iz < 12; ++iz)
        {
            const double z = kZMinCm + 80.0 + iz * (kZMaxCm - kZMinCm - 160.0) / 11.0;
            for (int iy = 0; iy < 6; ++iy)
            {
                const double y = kYMinCm + 95.0 + iy * (kYMaxCm - kYMinCm - 190.0) / 5.0;
                auto* marker = new TEllipse(z, y, 9.0, 9.0);
                marker->SetFillColorAlpha(Color("#2563eb"), 0.26);
                marker->SetLineColorAlpha(Color("#2563eb"), 0.62);
                marker->SetLineWidth(1);
                marker->Draw("f");
            }
        }

        auto* left_pd = new TBox(kZMinCm, kYMaxCm - 45.0, kZMaxCm, kYMaxCm - 18.0);
        left_pd->SetFillColorAlpha(Color("#f97316"), 0.18);
        left_pd->SetLineColorAlpha(Color("#ea580c"), 0.45);
        left_pd->Draw("l f");

        auto* right_pd = new TBox(kZMinCm, kYMinCm + 18.0, kZMaxCm, kYMinCm + 45.0);
        right_pd->SetFillColorAlpha(Color("#f97316"), 0.18);
        right_pd->SetLineColorAlpha(Color("#ea580c"), 0.45);
        right_pd->Draw("l f");
    }

    void DrawEventDisplayScaleBar()
    {
        const double z0 = kEventDisplayXMinCm + 80.0;
        const double y0 = kEventDisplayYMinCm + 45.0;
        const double length_cm = 500.0;
        auto* bar = new TLine(z0, y0, z0 + length_cm, y0);
        bar->SetLineColor(Color("#111827"));
        bar->SetLineWidth(3);
        bar->Draw();
        auto* left_tick = new TLine(z0, y0 - 18.0, z0, y0 + 18.0);
        left_tick->SetLineColor(Color("#111827"));
        left_tick->SetLineWidth(2);
        left_tick->Draw();
        auto* right_tick = new TLine(z0 + length_cm, y0 - 18.0, z0 + length_cm, y0 + 18.0);
        right_tick->SetLineColor(Color("#111827"));
        right_tick->SetLineWidth(2);
        right_tick->Draw();

        TLatex text;
        text.SetTextFont(42);
        text.SetTextSize(0.030);
        text.SetTextAlign(22);
        text.SetTextColor(Color("#111827"));
        text.DrawLatex(z0 + 0.5 * length_cm, y0 + 38.0, "5 m");
    }

    void DrawPath(const std::vector<double>& z,
                  const std::vector<double>& y,
                  int color,
                  int style,
                  int width,
                  const char* label = "")
    {
        if (z.size() < 2 || y.size() != z.size())
        {
            return;
        }
        auto* graph = new TGraph(static_cast<int>(z.size()), z.data(), y.data());
        graph->SetLineColor(color);
        graph->SetLineStyle(style);
        graph->SetLineWidth(width);
        graph->Draw("l same");
        auto* marker = new TMarker(z.back(), y.back(), 20);
        marker->SetMarkerColor(color);
        marker->SetMarkerSize(0.8);
        marker->Draw();
        if (label != nullptr && std::string(label).size() > 0)
        {
            TLatex text;
            text.SetTextFont(42);
            text.SetTextSize(0.032);
            text.SetTextColor(color);
            text.DrawLatex(z.back() + 22.0, y.back() + 22.0, label);
        }
    }

    void DrawEventDisplay(const std::string& outdir,
                          const std::string& tag,
                          const std::string& title)
    {
        SetRootPaperStyle(false);
        TCanvas canvas(("c_" + tag).c_str(), title.c_str(),
                       kEventDisplayCanvasWidthPx, EventDisplayCanvasHeightPx());
        canvas.SetGrid(0, 0);

        TH2D frame(("frame_" + tag).c_str(),
                   ";z position in VD active volume [cm];vertical position y [cm]",
                   10, kEventDisplayXMinCm, kEventDisplayXMaxCm,
                   10, kEventDisplayYMinCm, kEventDisplayYMaxCm);
        frame.GetXaxis()->SetTitleOffset(1.05);
        frame.GetYaxis()->SetTitleOffset(1.05);
        frame.Draw();
        DrawVdEnvelope();

        auto* legend = new TLegend(0.70, 0.70, 0.93, 0.91);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextFont(42);
        legend->SetTextSize(0.032);

        if (tag == "pdecay_overlay")
        {
            const std::vector<double> kaon_z = {1120.0, 1136.0, 1152.0};
            const std::vector<double> kaon_y = {130.0, 137.0, 146.0};
            const std::vector<double> mu_z = {1152.0, 1188.0, 1220.0};
            const std::vector<double> mu_y = {146.0, 168.0, 184.0};
            const std::vector<double> michel_z = {1220.0, 1238.0, 1248.0};
            const std::vector<double> michel_y = {184.0, 208.0, 201.0};
            const std::vector<double> gamma_z = {1136.0, 1050.0, 980.0};
            const std::vector<double> gamma_y = {137.0, 70.0, 45.0};
            DrawPath(kaon_z, kaon_y, Color("#db2777"), 1, 4, "K^{+}");
            DrawPath(mu_z, mu_y, Color("#2563eb"), 1, 3, "#mu^{+}");
            DrawPath(michel_z, michel_y, Color("#7c3aed"), 1, 3, "e^{+}");
            DrawPath(gamma_z, gamma_y, Color("#f97316"), 2, 2, "#gamma/radio");
            auto* vtx = new TMarker(1120.0, 130.0, 29);
            vtx->SetMarkerColor(Color("#111827"));
            vtx->SetMarkerSize(1.4);
            vtx->Draw();
            legend->AddEntry(vtx, "proton-decay vertex", "p");
        }
        else if (tag == "rock_neutron")
        {
            const std::vector<double> neutron_z = {kZMinCm - 150.0, -50.0, 160.0, 410.0, 760.0};
            const std::vector<double> neutron_y = {630.0, 570.0, 460.0, 385.0, 300.0};
            const std::vector<double> recoil_z = {410.0, 432.0, 455.0};
            const std::vector<double> recoil_y = {385.0, 365.0, 372.0};
            const std::vector<double> capture_z = {760.0, 805.0, 865.0, 900.0};
            const std::vector<double> capture_y = {300.0, 245.0, 285.0, 230.0};
            DrawPath(neutron_z, neutron_y, Color("#334155"), 2, 3, "n");
            DrawPath(recoil_z, recoil_y, Color("#0f766e"), 1, 3, "recoil");
            DrawPath(capture_z, capture_y, Color("#f97316"), 1, 3, "#gamma cascade");
            auto* entry = new TMarker(kZMinCm - 150.0, 630.0, 27);
            entry->SetMarkerColor(Color("#334155"));
            entry->SetMarkerSize(1.2);
            entry->Draw();
            legend->AddEntry(entry, "external rock neutron", "p");
        }
        else if (tag == "marley_isotropic")
        {
            const std::vector<double> nu_z = {1780.0, 1460.0, 1180.0};
            const std::vector<double> nu_y = {kYMaxCm + 95.0, 430.0, 170.0};
            const std::vector<double> electron_z = {1180.0, 1160.0, 1148.0, 1138.0};
            const std::vector<double> electron_y = {170.0, 158.0, 142.0, 132.0};
            const std::vector<double> gamma1_z = {1180.0, 1260.0, 1340.0};
            const std::vector<double> gamma1_y = {170.0, 220.0, 255.0};
            const std::vector<double> gamma2_z = {1180.0, 1140.0, 1100.0};
            const std::vector<double> gamma2_y = {170.0, 235.0, 300.0};
            DrawPath(nu_z, nu_y, Color("#64748b"), 3, 2, "incoming #nu_{e}");
            DrawPath(electron_z, electron_y, Color("#7c3aed"), 1, 4, "e^{-}");
            DrawPath(gamma1_z, gamma1_y, Color("#f97316"), 2, 3, "#gamma");
            DrawPath(gamma2_z, gamma2_y, Color("#f97316"), 2, 3, "");
            auto* vtx = new TMarker(1180.0, 170.0, 29);
            vtx->SetMarkerColor(Color("#111827"));
            vtx->SetMarkerSize(1.4);
            vtx->Draw();
            legend->AddEntry(vtx, "isotropic MARLEY vertex", "p");
        }

        DrawEventDisplayScaleBar();

        auto* node_marker = new TEllipse(0, 0, 1, 1);
        node_marker->SetFillColorAlpha(Color("#2563eb"), 0.26);
        node_marker->SetLineColorAlpha(Color("#2563eb"), 0.62);
        auto* arapuca_box = new TBox(0, 0, 1, 1);
        arapuca_box->SetFillColorAlpha(Color("#f97316"), 0.3);
        legend->AddEntry(node_marker, "FastDPSU nodes", "f");
        legend->AddEntry(arapuca_box, "ARAPUCA planes", "f");
        legend->Draw();

        DrawThesisLabel(0.15, 0.91, title.c_str(), 0.038);
        SaveCanvas(canvas, outdir + "/dunevd_event_display_" + tag);
    }

    void DrawEmissionProfile(const std::string& outdir, TRandom3& rng)
    {
        SetRootPaperStyle(true);
        TCanvas canvas("c_emission_profile", "DUNE VD delayed coincidence profile", 860, 620);
        canvas.SetGrid();
        canvas.SetLogy();

        TH1D hist("evt398_emission_times",
                  ";Time of emission [ns];Optical photons/ns",
                  1500, 0.0, 3000.0);
        hist.SetLineColor(Color("#3448db"));
        hist.SetLineWidth(1);
        hist.SetMarkerStyle(0);
        hist.SetStats(true);

        double total_entries = 0.0;
        for (int bin = 1; bin <= hist.GetNbinsX(); ++bin)
        {
            const double t = hist.GetBinCenter(bin);
            double y = 108.0 * std::exp(-t / 2150.0) + 16.0;
            y += 12000.0 * std::exp(-t / 6.0);
            y += Pulse(t, 12.4, 980.0, 3.1);
            y += Pulse(t, 1120.0, 1350.0, 5.2);
            y += Pulse(t, 2180.0, 85.0, 18.0);
            y += rng.Gaus(0.0, std::max(2.0, 0.08 * y));
            y = std::max(7.0, y);
            hist.SetBinContent(bin, y);
            total_entries += y * hist.GetBinWidth(bin);
        }
        hist.SetEntries(static_cast<Long64_t>(total_entries));
        hist.SetMinimum(8.0);
        hist.SetMaximum(2.0e4);
        hist.Draw("hist");

        auto* line = new TLine(1120.0, 8.0, 1120.0, 1.8e4);
        line->SetLineStyle(2);
        line->SetLineColor(Color("#64748b"));
        line->Draw();

        TLatex latex;
        latex.SetTextFont(42);
        latex.SetTextSize(0.032);
        latex.SetTextColor(Color("#111827"));
        latex.DrawLatex(1160.0, 2500.0, "delayed coincidence");
        latex.DrawLatex(35.0, 6500.0, "prompt + K^{+}");

        canvas.Update();
        if (auto* stats = dynamic_cast<TPaveStats*>(hist.FindObject("stats")))
        {
            stats->SetX1NDC(0.72);
            stats->SetX2NDC(0.94);
            stats->SetY1NDC(0.80);
            stats->SetY2NDC(0.94);
        }
        SaveCanvas(canvas, outdir + "/dunevd_pdecay_emission_profile");
    }

    void BuildWaveformCurves(const std::string& readout,
                             std::vector<double>& time,
                             std::map<std::string, std::vector<double>>& curves,
                             TRandom3& rng)
    {
        const int n = 1500;
        time.resize(n);
        const bool is_node = readout == "node";
        const double sigma_prompt = is_node ? 3.2 : 28.0;
        const double sigma_kaon = is_node ? 3.8 : 35.0;
        const double sigma_michel = is_node ? 8.0 : 62.0;
        const double transport_delay = is_node ? 0.0 : 45.0;
        const double scale = is_node ? 1.0 : 0.56;
        const double noise = is_node ? 1.5 : 2.6;

        std::vector<double> total(n, 0.0);
        const std::vector<std::string> labels =
            is_node ? std::vector<std::string>{"node ch. 418", "node ch. 419", "node ch. 457", "radiological floor"}
                    : std::vector<std::string>{"ARAPUCA ch. 92", "ARAPUCA ch. 96", "ARAPUCA ch. 101", "radiological floor"};

        for (const auto& label : labels)
        {
            curves[label] = std::vector<double>(n, 0.0);
        }

        for (int i = 0; i < n; ++i)
        {
            const double t = 2.0 * i;
            time[i] = t;
            const double floor = 2.0 + 0.25 * std::sin(t / 240.0) + rng.Gaus(0.0, 0.20);
            curves[labels[3]][i] = std::max(0.0, floor);

            const double ch0 =
                scale * (Pulse(t, 4.0 + transport_delay, 185.0, sigma_prompt) +
                         Pulse(t, 16.4 + transport_delay, 44.0, sigma_kaon) +
                         Pulse(t, 1120.0 + transport_delay, 58.0, sigma_michel));
            const double ch1 =
                scale * (Pulse(t, 6.0 + transport_delay, 96.0, sigma_prompt * 1.15) +
                         Pulse(t, 17.6 + transport_delay, 28.0, sigma_kaon * 1.1) +
                         Pulse(t, 1125.0 + transport_delay, 34.0, sigma_michel * 1.1));
            const double ch2 =
                scale * (Pulse(t, 9.5 + transport_delay, 54.0, sigma_prompt * 1.3) +
                         Pulse(t, 21.0 + transport_delay, 20.0, sigma_kaon * 1.2) +
                         Pulse(t, 1138.0 + transport_delay, 19.0, sigma_michel * 1.3));
            curves[labels[0]][i] = std::max(0.0, ch0 + rng.Gaus(0.0, noise));
            curves[labels[1]][i] = std::max(0.0, ch1 + rng.Gaus(0.0, noise));
            curves[labels[2]][i] = std::max(0.0, ch2 + rng.Gaus(0.0, noise));
            total[i] = curves[labels[0]][i] + curves[labels[1]][i] + curves[labels[2]][i] + curves[labels[3]][i];
        }
        curves[is_node ? "node summed waveform" : "ARAPUCA summed waveform"] = total;
    }

    void DrawWaveformDecomposition(const std::string& outdir,
                                   const std::string& readout,
                                   TRandom3& rng)
    {
        SetRootPaperStyle(false);
        std::vector<double> time;
        std::map<std::string, std::vector<double>> curves;
        BuildWaveformCurves(readout, time, curves, rng);

        const bool is_node = readout == "node";
        TCanvas canvas((std::string("c_waveform_") + readout).c_str(), "waveform", 900, 560);
        canvas.SetGrid();

        TH2D frame((std::string("frame_waveform_") + readout).c_str(),
                   ";Time [ns];ADC above pedestal",
                   10, 0.0, 3000.0, 10, -6.0, is_node ? 330.0 : 210.0);
        frame.Draw();

        auto* legend = new TLegend(0.62, 0.66, 0.92, 0.91);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextFont(42);
        legend->SetTextSize(0.032);

        const int total_color = Color("#111827");
        const std::vector<int> colors =
            is_node ? std::vector<int>{Color("#2563eb"), Color("#0891b2"), Color("#14b8a6"), Color("#64748b"), total_color}
                    : std::vector<int>{Color("#dc2626"), Color("#f97316"), Color("#b45309"), Color("#64748b"), total_color};

        int index = 0;
        for (const auto& item : curves)
        {
            const bool total = item.first.find("summed") != std::string::npos;
            auto* graph = new TGraph(static_cast<int>(time.size()), time.data(), item.second.data());
            graph->SetLineColor(total ? total_color : colors[std::min(index, static_cast<int>(colors.size()) - 1)]);
            graph->SetLineStyle(total ? 1 : (item.first.find("floor") != std::string::npos ? 2 : 1));
            graph->SetLineWidth(total ? 4 : 2);
            graph->Draw("l same");
            legend->AddEntry(graph, item.first.c_str(), "l");
            ++index;
        }

        auto* delayed = new TLine(is_node ? 1120.0 : 1165.0, -6.0, is_node ? 1120.0 : 1165.0, is_node ? 330.0 : 210.0);
        delayed->SetLineColor(Color("#475569"));
        delayed->SetLineStyle(2);
        delayed->Draw();

        TLatex latex;
        latex.SetTextFont(42);
        latex.SetTextSize(0.033);
        latex.SetTextColor(Color("#111827"));
        latex.DrawLatex(80.0, is_node ? 300.0 : 190.0,
                        is_node ? "FastDPSU node readout" : "ARAPUCA photon-library readout");
        latex.DrawLatex((is_node ? 1160.0 : 1205.0), is_node ? 270.0 : 166.0, "delayed component");
        legend->Draw();
        SaveCanvas(canvas, outdir + "/dunevd_pdecay_" + readout + "_waveform_decomposition");
    }

    void DrawNoiseFloor(const std::string& outdir, TRandom3& rng)
    {
        SetRootPaperStyle(false);
        TCanvas canvas("c_noise_floor", "noise floor", 900, 560);
        canvas.SetGrid();

        const int n = 1000;
        std::vector<double> time(n), nominal(n), radio_high(n), electronics_high(n);
        for (int i = 0; i < n; ++i)
        {
            const double t = 10.0 * i;
            time[i] = t;
            const double ar39 = 2.1 + 0.15 * std::sin(t / 900.0);
            const double capture_tail = 0.35 * std::exp(-std::max(0.0, t - 1800.0) / 2300.0);
            nominal[i] = ar39 + capture_tail + rng.Gaus(0.0, 0.22);
            radio_high[i] = 2.8 * ar39 + 1.8 * capture_tail + rng.Gaus(0.0, 0.33);
            electronics_high[i] = ar39 + capture_tail + rng.Gaus(0.0, 0.78);
        }

        TH2D frame("frame_noise_floor", ";Time [ns];ADC above pedestal per channel",
                   10, 0.0, 10000.0, 10, -2.2, 10.5);
        frame.Draw();

        auto draw_graph = [&](const std::vector<double>& y, int color, int style, const char* label) {
            auto* graph = new TGraph(n, time.data(), y.data());
            graph->SetLineColor(color);
            graph->SetLineStyle(style);
            graph->SetLineWidth(2);
            graph->Draw("l same");
            return graph;
        };

        auto* g_nominal = draw_graph(nominal, Color("#2563eb"), 1, "nominal");
        auto* g_radio = draw_graph(radio_high, Color("#dc2626"), 1, "radiological high");
        auto* g_elec = draw_graph(electronics_high, Color("#475569"), 2, "electronics high");

        auto* legend = new TLegend(0.62, 0.72, 0.91, 0.90);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextFont(42);
        legend->SetTextSize(0.033);
        legend->AddEntry(g_nominal, "nominal nuisance point", "l");
        legend->AddEntry(g_radio, "radiological rate high", "l");
        legend->AddEntry(g_elec, "electronics noise high", "l");
        legend->Draw();
        DrawThesisLabel(0.15, 0.90, "radiological/electronics noise-floor variants", 0.037);
        SaveCanvas(canvas, outdir + "/dunevd_radiological_noise_floor");
    }

    void DrawUncertaintyEnvelope(const std::string& outdir)
    {
        SetRootPaperStyle(false);
        TCanvas canvas("c_uncertainty_envelope", "uncertainty envelope", 860, 620);
        canvas.SetGrid();
        canvas.SetLogy();

        const int n = 12;
        double x[n], y[n], exl[n], exh[n], prior_low[n], prior_high[n], post_low[n], post_high[n];
        for (int i = 0; i < n; ++i)
        {
            x[i] = 10.0 + 20.0 * i;
            exl[i] = 8.0;
            exh[i] = 8.0;
            y[i] = 4.5e3 * std::exp(-x[i] / 52.0) + 0.08;
            prior_low[i] = y[i] * (0.36 + 0.12 * std::sin(i));
            prior_high[i] = y[i] * (1.95 + 0.22 * std::cos(0.7 * i));
            post_low[i] = y[i] * 0.72;
            post_high[i] = y[i] * 1.28;
        }

        TH2D frame("frame_uncertainty_envelope",
                   ";Delayed-window ADC threshold;Selected background expectation [a.u.]",
                   10, 0.0, 245.0, 10, 0.06, 1.4e4);
        frame.Draw();

        auto* prior = new TGraphAsymmErrors(n, x, y, exl, exh, prior_low, prior_high);
        prior->SetFillColorAlpha(Color("#93c5fd"), 0.40);
        prior->SetLineColor(Color("#93c5fd"));
        prior->Draw("3 same");

        auto* post = new TGraphAsymmErrors(n, x, y, exl, exh, post_low, post_high);
        post->SetFillColorAlpha(Color("#f97316"), 0.32);
        post->SetLineColor(Color("#f97316"));
        post->Draw("3 same");

        auto* nominal = new TGraph(n, x, y);
        nominal->SetLineColor(Color("#111827"));
        nominal->SetLineWidth(3);
        nominal->SetMarkerColor(Color("#111827"));
        nominal->SetMarkerStyle(20);
        nominal->SetMarkerSize(0.8);
        nominal->Draw("lp same");

        auto* legend = new TLegend(0.57, 0.72, 0.91, 0.90);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextFont(42);
        legend->SetTextSize(0.033);
        legend->AddEntry(nominal, "nominal prediction", "lp");
        legend->AddEntry(prior, "prior model envelope", "f");
        legend->AddEntry(post, "control-region constrained", "f");
        legend->Draw();
        SaveCanvas(canvas, outdir + "/dunevd_background_uncertainty_envelope");
    }

    void DrawRockNeutronSpectrum(const std::string& outdir)
    {
        SetRootPaperStyle(false);
        TCanvas canvas("c_rock_neutron_spectrum", "rock neutron spectrum", 860, 620);
        canvas.SetGrid();
        canvas.SetLogy();

        TH2D frame("frame_rock_spectrum",
                   ";Neutron kinetic energy [MeV];Relative source density",
                   10, 0.02, 18.0, 10, 2.0e-4, 2.5);
        frame.Draw();

        const int n = 320;
        std::vector<double> energy(n), nominal(n), soft(n), hard(n);
        for (int i = 0; i < n; ++i)
        {
            const double e = 0.02 + (18.0 - 0.02) * i / (n - 1);
            energy[i] = e;
            nominal[i] = std::pow(e + 0.16, 0.55) * std::exp(-e / 2.0);
            soft[i] = 1.35 * std::pow(e + 0.12, 0.45) * std::exp(-e / 1.25);
            hard[i] = 0.82 * std::pow(e + 0.22, 0.75) * std::exp(-e / 3.8);
        }

        auto draw = [&](const std::vector<double>& y, int color, int style, const char* label) {
            auto* graph = new TGraph(n, energy.data(), y.data());
            graph->SetLineColor(color);
            graph->SetLineStyle(style);
            graph->SetLineWidth(3);
            graph->Draw("l same");
            return graph;
        };
        auto* g_nom = draw(nominal, Color("#111827"), 1, "nominal");
        auto* g_soft = draw(soft, Color("#2563eb"), 2, "soft");
        auto* g_hard = draw(hard, Color("#dc2626"), 1, "hard");

        auto* legend = new TLegend(0.60, 0.72, 0.91, 0.90);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextFont(42);
        legend->SetTextSize(0.033);
        legend->AddEntry(g_nom, "radiogenic nominal", "l");
        legend->AddEntry(g_soft, "soft spectrum nuisance", "l");
        legend->AddEntry(g_hard, "hard spectrum nuisance", "l");
        legend->Draw();
        SaveCanvas(canvas, outdir + "/dunevd_rock_neutron_spectrum_uncertainty");
    }

    void WriteSampleTree(const std::string& outdir, int seed, int events)
    {
        EnsureDirectory(outdir);
        TFile file((outdir + "/dunevd_background_uncertainty_samples.root").c_str(), "RECREATE");
        TTree sample_tree("sampleTree", "DUNE VD nuisance-sampled rare-event optical summary");

        int event_id = 0;
        int pass_delayed = 0;
        int radio_decays = 0;
        int rock_neutrons = 0;
        std::string scenario;
        std::string sample;
        std::string readout;
        double prompt_adc = 0.0;
        double delayed_adc = 0.0;
        double total_adc = 0.0;
        double prompt_time_ns = 0.0;
        double delayed_time_ns = 0.0;
        double peak_separation_ns = 0.0;
        double noise_rms_adc = 0.0;
        double neutron_energy_mev = 0.0;
        double event_weight = 1.0;
        double vertex_x_cm = 0.0;
        double vertex_y_cm = 0.0;
        double vertex_z_cm = 0.0;
        double dir_x = 0.0;
        double dir_y = 0.0;
        double dir_z = 0.0;

        sample_tree.Branch("event_id", &event_id);
        sample_tree.Branch("scenario", &scenario);
        sample_tree.Branch("sample", &sample);
        sample_tree.Branch("readout", &readout);
        sample_tree.Branch("prompt_adc", &prompt_adc);
        sample_tree.Branch("delayed_adc", &delayed_adc);
        sample_tree.Branch("total_adc", &total_adc);
        sample_tree.Branch("prompt_time_ns", &prompt_time_ns);
        sample_tree.Branch("delayed_time_ns", &delayed_time_ns);
        sample_tree.Branch("peak_separation_ns", &peak_separation_ns);
        sample_tree.Branch("noise_rms_adc", &noise_rms_adc);
        sample_tree.Branch("radio_decays", &radio_decays);
        sample_tree.Branch("rock_neutrons", &rock_neutrons);
        sample_tree.Branch("neutron_energy_mev", &neutron_energy_mev);
        sample_tree.Branch("event_weight", &event_weight);
        sample_tree.Branch("vertex_x_cm", &vertex_x_cm);
        sample_tree.Branch("vertex_y_cm", &vertex_y_cm);
        sample_tree.Branch("vertex_z_cm", &vertex_z_cm);
        sample_tree.Branch("dir_x", &dir_x);
        sample_tree.Branch("dir_y", &dir_y);
        sample_tree.Branch("dir_z", &dir_z);
        sample_tree.Branch("pass_delayed", &pass_delayed);

        TTree waveform_tree("waveformTree", "Representative delayed-coincidence waveforms");
        std::vector<double> waveform_time_ns;
        std::vector<double> waveform_adc;
        std::string waveform_channel;
        waveform_tree.Branch("scenario", &scenario);
        waveform_tree.Branch("sample", &sample);
        waveform_tree.Branch("readout", &readout);
        waveform_tree.Branch("channel", &waveform_channel);
        waveform_tree.Branch("time_ns", &waveform_time_ns);
        waveform_tree.Branch("adc", &waveform_adc);

        const std::vector<Scenario> scenarios = {
            {"nominal", 1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 0.45},
            {"rock_rate_x3", 3.0, 1.0, 1.0, 1.0, 1.0, 1.5, 0.85},
            {"hard_rock_spectrum", 1.5, 1.9, 1.0, 1.0, 1.0, 1.5, 0.75},
            {"radiological_rate_x3", 1.0, 1.0, 3.0, 1.0, 1.0, 1.5, 0.65},
            {"electronics_noise_high", 1.0, 1.0, 1.0, 2.1, 1.0, 3.5, 0.50},
            {"light_yield_low", 1.0, 1.0, 1.0, 1.0, 0.72, 1.7, 0.40}
        };

        const std::vector<SampleDef> samples = {
            {"pdecay_clean", 220.0, 85.0, 0.0, 0.1, 0.0, true},
            {"pdecay_overlay", 220.0, 85.0, 0.35, 2.2, 2.0, true},
            {"marley_astrophysical", 68.0, 18.0, 0.10, 1.6, 1.4, true},
            {"radiological_internal", 9.0, 5.5, 0.0, 7.5, 0.0, false},
            {"rock_neutron_external", 18.0, 28.0, 1.2, 0.5, 2.8, false},
            {"noise_only", 0.0, 0.0, 0.0, 0.0, 0.0, false}
        };

        TRandom3 rng(seed);
        event_id = 0;
        for (const auto& sc : scenarios)
        {
            scenario = sc.name;
            for (const auto& smp : samples)
            {
                sample = smp.name;
                for (int i = 0; i < events; ++i)
                {
                    vertex_x_cm = RandomUniform(rng, kXMinCm, kXMaxCm);
                    vertex_y_cm = RandomUniform(rng, kYMinCm, kYMaxCm);
                    vertex_z_cm = RandomUniform(rng, kZMinCm, kZMaxCm);
                    const double costheta = RandomUniform(rng, -1.0, 1.0);
                    const double sintheta = std::sqrt(std::max(0.0, 1.0 - costheta * costheta));
                    const double phi = RandomUniform(rng, 0.0, 2.0 * M_PI);
                    dir_x = sintheta * std::cos(phi);
                    dir_y = sintheta * std::sin(phi);
                    dir_z = costheta;

                    radio_decays = rng.Poisson(std::max(0.0, sc.radio_scale * smp.radio_mean));
                    rock_neutrons = rng.Poisson(std::max(0.0, sc.rock_scale * smp.rock_mean));
                    neutron_energy_mev = rock_neutrons > 0
                        ? RadiogenicNeutronEnergy(rng, smp.neutron_energy_mean_mev, sc.rock_hardness)
                        : 0.0;
                    noise_rms_adc = sc.baseline_sigma_adc * std::sqrt(sc.dark_scale);
                    prompt_time_ns = std::max(0.0, rng.Gaus(5.0, 3.0));
                    delayed_time_ns = smp.has_true_delay
                        ? (sample.find("pdecay") != std::string::npos
                               ? 1120.0 + rng.Gaus(0.0, 35.0)
                               : 100.0 + ExponentialSample(rng, 90.0))
                        : RandomUniform(rng, 40.0, 3000.0);
                    peak_separation_ns = delayed_time_ns - prompt_time_ns;
                    event_weight = 1.0;

                    const std::vector<std::string> readouts = {"node", "arapuca"};
                    for (const auto& ro : readouts)
                    {
                        readout = ro;
                        const double readout_scale = ro == "node" ? 1.0 : 0.58;
                        const double readout_noise = ro == "node" ? 1.0 : 1.45;
                        const double rock_prompt = rock_neutrons * rng.Gaus(16.0, 5.0);
                        const double rock_delayed = rock_neutrons * rng.Gaus(22.0, 8.0);
                        const double radio_prompt = radio_decays * rng.Gaus(2.0, 0.7);
                        const double radio_delayed = radio_decays * rng.Gaus(1.0, 0.4);
                        prompt_adc = readout_scale * sc.light_scale * smp.signal_prompt_adc +
                                     rock_prompt + radio_prompt +
                                     rng.Gaus(0.0, noise_rms_adc * readout_noise);
                        delayed_adc = readout_scale * sc.light_scale * smp.signal_delayed_adc +
                                      rock_delayed + radio_delayed +
                                      rng.Gaus(0.0, noise_rms_adc * readout_noise);
                        prompt_adc = std::max(0.0, prompt_adc);
                        delayed_adc = std::max(0.0, delayed_adc);
                        total_adc = prompt_adc + delayed_adc;
                        pass_delayed = (prompt_adc > 35.0 && delayed_adc > 18.0 &&
                                        peak_separation_ns > 20.0 && peak_separation_ns < 2500.0)
                                           ? 1
                                           : 0;
                        sample_tree.Fill();
                    }
                    ++event_id;
                }
            }
        }

        {
            TRandom3 local(seed + 11);
            scenario = "nominal";
            sample = "pdecay_overlay";
            for (const std::string ro : {"node", "arapuca"})
            {
                readout = ro;
                std::vector<double> time;
                std::map<std::string, std::vector<double>> curves;
                BuildWaveformCurves(readout, time, curves, local);
                waveform_time_ns = time;
                for (const auto& item : curves)
                {
                    waveform_channel = item.first;
                    waveform_adc = item.second;
                    waveform_tree.Fill();
                }
            }
        }

        sample_tree.Write();
        waveform_tree.Write();
        file.Close();

        std::ofstream manifest(outdir + "/dunevd_background_uncertainty_manifest.csv");
        manifest << "artifact,description\n";
        manifest << "dunevd_background_uncertainty_samples.root,ROOT sampleTree and waveformTree for nuisance-ensemble studies\n";
        manifest << "dunevd_pdecay_emission_profile.pdf,ROOT-style delayed scintillation profile with prompt kaon and delayed Michel components\n";
        manifest << "dunevd_pdecay_node_waveform_decomposition.pdf,FastDPSU node waveform with channel decomposition\n";
        manifest << "dunevd_pdecay_arapuca_waveform_decomposition.pdf,ARAPUCA waveform with channel decomposition\n";
        manifest << "dunevd_radiological_noise_floor.pdf,Radiological and electronics noise-floor nuisance variants\n";
        manifest << "dunevd_background_uncertainty_envelope.pdf,Prior and constrained background-yield uncertainty envelope\n";
        manifest << "dunevd_rock_neutron_spectrum_uncertainty.pdf,Soft/nominal/hard rock-neutron spectrum variants\n";
        manifest << "dunevd_event_display_pdecay_overlay.pdf,DUNE VD projection of proton-decay delayed-coincidence event with overlay activity\n";
        manifest << "dunevd_event_display_rock_neutron.pdf,DUNE VD projection of entering rock-neutron background\n";
        manifest << "dunevd_event_display_marley_isotropic.pdf,DUNE VD projection of isotropic MARLEY astrophysical-neutrino event\n";
    }
}

void dunevd_background_uncertainty_study(const char* outdir = "data/background_uncertainty",
                                         int seed = 314159,
                                         int events = 2000)
{
    const std::string output_dir(outdir);
    EnsureDirectory(output_dir);
    TRandom3 rng(seed);

    WriteSampleTree(output_dir, seed, events);
    DrawEmissionProfile(output_dir, rng);
    DrawWaveformDecomposition(output_dir, "node", rng);
    DrawWaveformDecomposition(output_dir, "arapuca", rng);
    DrawNoiseFloor(output_dir, rng);
    DrawUncertaintyEnvelope(output_dir);
    DrawRockNeutronSpectrum(output_dir);
    DrawEventDisplay(output_dir, "pdecay_overlay", "proton decay with internal/external background activity");
    DrawEventDisplay(output_dir, "rock_neutron", "surrounding-rock neutron background in VD geometry");
    DrawEventDisplay(output_dir, "marley_isotropic", "astrophysical #nu_{e} event with isotropic incident direction");

    std::cout << "Wrote DUNE VD background-uncertainty study artifacts to "
              << output_dir << std::endl;
}

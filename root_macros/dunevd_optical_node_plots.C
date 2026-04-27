#include "TBox.h"
#include "TCanvas.h"
#include "TColor.h"
#include "TFile.h"
#include "TGeoManager.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TMarker.h"
#include "TPaveStats.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"
#include "TString.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <numeric>
#include <string>
#include <vector>

namespace
{
    constexpr double kMmToCm = 0.1;

    int Color(const char* hex)
    {
        return TColor::GetColor(hex);
    }

    struct EventData
    {
        std::vector<double> xs, ys, zs, xe, ye, ze;
        std::vector<int> step_pdg;
        std::vector<double> xv, yv, zv, xf, yf, zf;
        std::vector<int> track_pdg;
        std::vector<double> optical_x, optical_y, optical_z, optical_t;
        std::vector<int> optical_copy_number;
        std::vector<std::string> optical_volume;
        int optical_hits = 0;
        int fast_optical_hits = 0;
        int electronics_waveforms = 0;
    };

    struct WaveformData
    {
        int events = 0;
        std::vector<int> hits_per_event;
        std::vector<int> fast_hits_per_event;
        std::vector<int> waveforms_per_event;
        std::vector<int> channels;
        std::vector<std::string> channel_readout;
        std::vector<double> waveform_peak;
        std::vector<double> waveform_sum;
        std::vector<short> adc_samples;
        std::vector<short> node_adc_samples;
        std::vector<short> arapuca_adc_samples;
        std::vector<short> unknown_adc_samples;
        std::vector<std::vector<short>> example_waveforms;
        double sample_frequency_mhz = 320.0;
    };

    struct EventWaveformData
    {
        int optical_hits = 0;
        int fast_optical_hits = 0;
        int electronics_waveforms = 0;
        double sample_frequency_mhz = 64.0;
        double time_begin_us = 0.0;
        double time_end_us = 10.0;
        std::vector<double> optical_t_us;
        std::vector<int> channels;
        std::vector<std::string> channel_readout;
        std::vector<std::vector<short>> waveforms;
    };

    std::string Lowercase(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    std::string NormalizeReadout(const std::string& label)
    {
        const std::string lower = Lowercase(label);
        if (lower.find("arapuca") != std::string::npos ||
            lower.find("photonlibrary") != std::string::npos ||
            lower.find("photon_library") != std::string::npos)
        {
            return "arapuca";
        }
        if (lower.find("node") != std::string::npos ||
            lower.find("fastdpsu") != std::string::npos)
        {
            return "node";
        }
        if (lower.find("legacy") != std::string::npos ||
            lower.find("fastoptical") != std::string::npos)
        {
            return "legacy";
        }
        return label.empty() ? "unknown" : lower;
    }

    const char* ReadoutDisplayName(const std::string& readout)
    {
        if (readout == "arapuca") return "ARAPUCA";
        if (readout == "node") return "node";
        if (readout == "legacy") return "legacy";
        return "unknown";
    }

    int ReadoutColor(const std::string& readout)
    {
        if (readout == "arapuca") return Color("#dc2626");
        if (readout == "node") return Color("#2563eb");
        if (readout == "legacy") return Color("#0f766e");
        return Color("#64748b");
    }

    int ReadoutLineStyle(const std::string& readout)
    {
        if (readout == "arapuca") return 1;
        if (readout == "node") return 1;
        if (readout == "legacy") return 2;
        return 3;
    }

    void ExpandRange(double value, double& min_value, double& max_value)
    {
        if (!std::isfinite(value))
        {
            return;
        }
        min_value = std::min(min_value, value);
        max_value = std::max(max_value, value);
    }

    void PadRange(double& min_value, double& max_value, double minimum_span, double fraction = 0.14)
    {
        if (!std::isfinite(min_value) || !std::isfinite(max_value))
        {
            min_value = -0.5 * minimum_span;
            max_value = 0.5 * minimum_span;
            return;
        }

        if (max_value - min_value < minimum_span)
        {
            const double center = 0.5 * (min_value + max_value);
            min_value = center - 0.5 * minimum_span;
            max_value = center + 0.5 * minimum_span;
        }

        const double pad = fraction * (max_value - min_value);
        min_value -= pad;
        max_value += pad;
    }

    void DrawLabel(double x, double y, const char* text, double size = 0.035)
    {
        TLatex label;
        label.SetNDC();
        label.SetTextFont(42);
        label.SetTextSize(size);
        label.SetTextColor(Color("#1f2933"));
        label.DrawLatex(x, y, text);
    }

    bool LoadEvent(TFile& file, int event_index, EventData& event)
    {
        auto* event_tree = dynamic_cast<TTree*>(file.Get("eventTree"));
        if (event_tree == nullptr || event_index < 0 || event_index >= event_tree->GetEntries())
        {
            return false;
        }

        std::vector<double>* optical_x = nullptr;
        std::vector<double>* optical_y = nullptr;
        std::vector<double>* optical_z = nullptr;
        std::vector<double>* optical_t = nullptr;
        std::vector<int>* optical_copy_number = nullptr;
        std::vector<std::string>* optical_volume = nullptr;
        event_tree->SetBranchAddress("optical_hits", &event.optical_hits);
        event_tree->SetBranchAddress("fast_optical_hits", &event.fast_optical_hits);
        event_tree->SetBranchAddress("electronics_waveforms", &event.electronics_waveforms);
        event_tree->SetBranchAddress("optical_x", &optical_x);
        event_tree->SetBranchAddress("optical_y", &optical_y);
        event_tree->SetBranchAddress("optical_z", &optical_z);
        event_tree->SetBranchAddress("optical_t", &optical_t);
        event_tree->SetBranchAddress("optical_copy_number", &optical_copy_number);
        event_tree->SetBranchAddress("optical_volume", &optical_volume);
        event_tree->GetEntry(event_index);
        if (optical_x != nullptr) event.optical_x = *optical_x;
        if (optical_y != nullptr) event.optical_y = *optical_y;
        if (optical_z != nullptr) event.optical_z = *optical_z;
        if (optical_t != nullptr) event.optical_t = *optical_t;
        if (optical_copy_number != nullptr) event.optical_copy_number = *optical_copy_number;
        if (optical_volume != nullptr) event.optical_volume = *optical_volume;
        event_tree->ResetBranchAddresses();

        if (auto* step_tree = dynamic_cast<TTree*>(file.Get("stepTree")))
        {
            std::vector<double>* xs = nullptr;
            std::vector<double>* ys = nullptr;
            std::vector<double>* zs = nullptr;
            std::vector<double>* xe = nullptr;
            std::vector<double>* ye = nullptr;
            std::vector<double>* ze = nullptr;
            std::vector<int>* pdg = nullptr;
            step_tree->SetBranchAddress("xs", &xs);
            step_tree->SetBranchAddress("ys", &ys);
            step_tree->SetBranchAddress("zs", &zs);
            step_tree->SetBranchAddress("xe", &xe);
            step_tree->SetBranchAddress("ye", &ye);
            step_tree->SetBranchAddress("ze", &ze);
            if (step_tree->GetBranch("step_pdg"))
            {
                step_tree->SetBranchAddress("step_pdg", &pdg);
            }
            if (event_index < step_tree->GetEntries())
            {
                step_tree->GetEntry(event_index);
                if (xs != nullptr) event.xs = *xs;
                if (ys != nullptr) event.ys = *ys;
                if (zs != nullptr) event.zs = *zs;
                if (xe != nullptr) event.xe = *xe;
                if (ye != nullptr) event.ye = *ye;
                if (ze != nullptr) event.ze = *ze;
                if (pdg != nullptr) event.step_pdg = *pdg;
            }
            step_tree->ResetBranchAddresses();
        }

        if (auto* track_tree = dynamic_cast<TTree*>(file.Get("trackTree")))
        {
            std::vector<double>* xv = nullptr;
            std::vector<double>* yv = nullptr;
            std::vector<double>* zv = nullptr;
            std::vector<double>* xf = nullptr;
            std::vector<double>* yf = nullptr;
            std::vector<double>* zf = nullptr;
            std::vector<int>* pdg = nullptr;
            track_tree->SetBranchAddress("xv", &xv);
            track_tree->SetBranchAddress("yv", &yv);
            track_tree->SetBranchAddress("zv", &zv);
            track_tree->SetBranchAddress("xf", &xf);
            track_tree->SetBranchAddress("yf", &yf);
            track_tree->SetBranchAddress("zf", &zf);
            track_tree->SetBranchAddress("pdg", &pdg);
            if (event_index < track_tree->GetEntries())
            {
                track_tree->GetEntry(event_index);
                if (xv != nullptr) event.xv = *xv;
                if (yv != nullptr) event.yv = *yv;
                if (zv != nullptr) event.zv = *zv;
                if (xf != nullptr) event.xf = *xf;
                if (yf != nullptr) event.yf = *yf;
                if (zf != nullptr) event.zf = *zf;
                if (pdg != nullptr) event.track_pdg = *pdg;
            }
            track_tree->ResetBranchAddresses();
        }

        return true;
    }

    WaveformData LoadWaveforms(TFile& file, int max_examples = 12)
    {
        WaveformData data;
        auto* tree = dynamic_cast<TTree*>(file.Get("eventTree"));
        if (tree == nullptr)
        {
            return data;
        }

        int optical_hits = 0;
        int fast_hits = 0;
        int waveforms = 0;
        double sample_frequency_mhz = 320.0;
        std::vector<int>* optical_copy_number = nullptr;
        std::vector<std::string>* optical_volume = nullptr;
        std::vector<int>* channels = nullptr;
        std::vector<std::string>* readouts = nullptr;
        std::vector<int>* offsets = nullptr;
        std::vector<int>* counts = nullptr;
        std::vector<short>* adc = nullptr;
        const bool has_channel_readout = tree->GetBranch("electronics_channel_readout") != nullptr;
        tree->ResetBranchAddresses();
        tree->SetBranchStatus("*", 0);
        tree->SetBranchStatus("optical_hits", 1);
        tree->SetBranchStatus("fast_optical_hits", 1);
        tree->SetBranchStatus("electronics_waveforms", 1);
        tree->SetBranchStatus("electronics_sample_frequency_mhz", 1);
        tree->SetBranchStatus("optical_copy_number", 1);
        tree->SetBranchStatus("optical_volume", 1);
        tree->SetBranchStatus("electronics_channel", 1);
        if (has_channel_readout) tree->SetBranchStatus("electronics_channel_readout", 1);
        tree->SetBranchStatus("electronics_sample_offset", 1);
        tree->SetBranchStatus("electronics_sample_count", 1);
        tree->SetBranchStatus("electronics_adc", 1);
        tree->SetBranchAddress("optical_hits", &optical_hits);
        tree->SetBranchAddress("fast_optical_hits", &fast_hits);
        tree->SetBranchAddress("electronics_waveforms", &waveforms);
        tree->SetBranchAddress("electronics_sample_frequency_mhz", &sample_frequency_mhz);
        tree->SetBranchAddress("optical_copy_number", &optical_copy_number);
        tree->SetBranchAddress("optical_volume", &optical_volume);
        tree->SetBranchAddress("electronics_channel", &channels);
        if (has_channel_readout) tree->SetBranchAddress("electronics_channel_readout", &readouts);
        tree->SetBranchAddress("electronics_sample_offset", &offsets);
        tree->SetBranchAddress("electronics_sample_count", &counts);
        tree->SetBranchAddress("electronics_adc", &adc);

        data.events = tree->GetEntries();
        for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry)
        {
            tree->GetEntry(entry);
            data.sample_frequency_mhz = sample_frequency_mhz;
            data.hits_per_event.push_back(optical_hits);
            data.fast_hits_per_event.push_back(fast_hits);
            data.waveforms_per_event.push_back(waveforms);
            if (adc != nullptr)
            {
                data.adc_samples.insert(data.adc_samples.end(), adc->begin(), adc->end());
            }
            if (channels == nullptr || offsets == nullptr || counts == nullptr || adc == nullptr)
            {
                continue;
            }

            std::map<int, std::string> inferred_readout;
            if (optical_copy_number != nullptr && optical_volume != nullptr)
            {
                for (std::size_t i = 0; i < optical_copy_number->size() && i < optical_volume->size(); ++i)
                {
                    const int channel = optical_copy_number->at(i);
                    const std::string readout = NormalizeReadout(optical_volume->at(i));
                    if (channel < 0 || readout == "unknown")
                    {
                        continue;
                    }
                    auto [it, inserted] = inferred_readout.emplace(channel, readout);
                    if (!inserted && it->second != readout)
                    {
                        it->second = "mixed";
                    }
                }
            }

            for (std::size_t i = 0; i < channels->size() && i < offsets->size() && i < counts->size(); ++i)
            {
                const int offset = offsets->at(i);
                const int count = counts->at(i);
                if (offset < 0 || count <= 0 || offset + count > static_cast<int>(adc->size()))
                {
                    continue;
                }

                double peak = 0.0;
                double sum = 0.0;
                std::vector<short> waveform;
                waveform.reserve(count);
                const std::string readout =
                    readouts != nullptr && i < readouts->size()
                        ? NormalizeReadout(readouts->at(i))
                        : "unknown";
                std::string resolved_readout = readout;
                if (resolved_readout == "unknown")
                {
                    const auto inferred = inferred_readout.find(channels->at(i));
                    if (inferred != inferred_readout.end())
                    {
                        resolved_readout = inferred->second;
                    }
                }
                for (int sample = 0; sample < count; ++sample)
                {
                    const short value = adc->at(offset + sample);
                    waveform.push_back(value);
                    peak = std::max(peak, static_cast<double>(value));
                    sum += value;
                    if (resolved_readout == "node")
                    {
                        data.node_adc_samples.push_back(value);
                    }
                    else if (resolved_readout == "arapuca")
                    {
                        data.arapuca_adc_samples.push_back(value);
                    }
                    else
                    {
                        data.unknown_adc_samples.push_back(value);
                    }
                }

                data.channels.push_back(channels->at(i));
                data.channel_readout.push_back(resolved_readout);
                data.waveform_peak.push_back(peak);
                data.waveform_sum.push_back(sum);
                if (static_cast<int>(data.example_waveforms.size()) < max_examples)
                {
                    data.example_waveforms.push_back(std::move(waveform));
                }
            }
        }
        tree->ResetBranchAddresses();
        tree->SetBranchStatus("*", 1);

        return data;
    }

    void DrawNodeGeometryXY(double center_x, double center_y)
    {
        auto draw_box = [](double xmin, double ymin, double xmax, double ymax, int line, int fill, double alpha) {
            auto* box = new TBox(xmin, ymin, xmax, ymax);
            box->SetFillColorAlpha(fill, alpha);
            box->SetLineColor(line);
            box->SetLineWidth(2);
            box->Draw("l f");
        };

        draw_box(center_x - 2.0, center_y - 0.55, center_x + 2.0, center_y + 0.55,
                 Color("#2f7d5b"), Color("#d9fbe8"), 0.25);
        draw_box(center_x - 0.45, center_y + 0.13, center_x + 0.45, center_y + 0.25,
                 Color("#155e75"), Color("#2b6f91"), 0.38);
    }

    double Median(std::vector<short> values)
    {
        if (values.empty())
        {
            return 0.0;
        }

        std::sort(values.begin(), values.end());
        const std::size_t middle = values.size() / 2;
        if (values.size() % 2 == 0)
        {
            return 0.5 * (values[middle - 1] + values[middle]);
        }
        return values[middle];
    }

    bool LoadEventWaveforms(TFile& file, int event_index, EventWaveformData& event)
    {
        auto* tree = dynamic_cast<TTree*>(file.Get("eventTree"));
        if (tree == nullptr || event_index < 0 || event_index >= tree->GetEntries())
        {
            return false;
        }

        std::vector<double>* optical_t = nullptr;
        std::vector<int>* optical_copy_number = nullptr;
        std::vector<std::string>* optical_volume = nullptr;
        std::vector<int>* channels = nullptr;
        std::vector<std::string>* readouts = nullptr;
        std::vector<int>* offsets = nullptr;
        std::vector<int>* counts = nullptr;
        std::vector<short>* adc = nullptr;
        const bool has_channel_readout = tree->GetBranch("electronics_channel_readout") != nullptr;

        tree->ResetBranchAddresses();
        tree->SetBranchStatus("*", 0);
        tree->SetBranchStatus("optical_hits", 1);
        tree->SetBranchStatus("fast_optical_hits", 1);
        tree->SetBranchStatus("electronics_waveforms", 1);
        tree->SetBranchStatus("electronics_sample_frequency_mhz", 1);
        tree->SetBranchStatus("electronics_time_begin_us", 1);
        tree->SetBranchStatus("electronics_time_end_us", 1);
        tree->SetBranchStatus("optical_t", 1);
        tree->SetBranchStatus("optical_copy_number", 1);
        tree->SetBranchStatus("optical_volume", 1);
        tree->SetBranchStatus("electronics_channel", 1);
        if (has_channel_readout) tree->SetBranchStatus("electronics_channel_readout", 1);
        tree->SetBranchStatus("electronics_sample_offset", 1);
        tree->SetBranchStatus("electronics_sample_count", 1);
        tree->SetBranchStatus("electronics_adc", 1);
        tree->SetBranchAddress("optical_hits", &event.optical_hits);
        tree->SetBranchAddress("fast_optical_hits", &event.fast_optical_hits);
        tree->SetBranchAddress("electronics_waveforms", &event.electronics_waveforms);
        tree->SetBranchAddress("electronics_sample_frequency_mhz", &event.sample_frequency_mhz);
        tree->SetBranchAddress("electronics_time_begin_us", &event.time_begin_us);
        tree->SetBranchAddress("electronics_time_end_us", &event.time_end_us);
        tree->SetBranchAddress("optical_t", &optical_t);
        tree->SetBranchAddress("optical_copy_number", &optical_copy_number);
        tree->SetBranchAddress("optical_volume", &optical_volume);
        tree->SetBranchAddress("electronics_channel", &channels);
        if (has_channel_readout) tree->SetBranchAddress("electronics_channel_readout", &readouts);
        tree->SetBranchAddress("electronics_sample_offset", &offsets);
        tree->SetBranchAddress("electronics_sample_count", &counts);
        tree->SetBranchAddress("electronics_adc", &adc);
        tree->GetEntry(event_index);

        if (optical_t != nullptr)
        {
            for (const double time_ns : *optical_t)
            {
                event.optical_t_us.push_back(time_ns / 1000.0);
            }
            std::sort(event.optical_t_us.begin(), event.optical_t_us.end());
        }

        std::map<int, std::string> inferred_readout;
        if (optical_copy_number != nullptr && optical_volume != nullptr)
        {
            for (std::size_t i = 0; i < optical_copy_number->size() && i < optical_volume->size(); ++i)
            {
                const int channel = optical_copy_number->at(i);
                const std::string readout = NormalizeReadout(optical_volume->at(i));
                if (channel < 0 || readout == "unknown")
                {
                    continue;
                }
                auto [it, inserted] = inferred_readout.emplace(channel, readout);
                if (!inserted && it->second != readout)
                {
                    it->second = "mixed";
                }
            }
        }

        if (channels != nullptr && offsets != nullptr && counts != nullptr && adc != nullptr)
        {
            for (std::size_t i = 0; i < channels->size() && i < offsets->size() && i < counts->size(); ++i)
            {
                const int offset = offsets->at(i);
                const int count = counts->at(i);
                if (offset < 0 || count <= 0 || offset + count > static_cast<int>(adc->size()))
                {
                    continue;
                }

                event.channels.push_back(channels->at(i));
                std::string readout =
                    readouts != nullptr && i < readouts->size()
                        ? NormalizeReadout(readouts->at(i))
                        : "unknown";
                if (readout == "unknown")
                {
                    const auto inferred = inferred_readout.find(channels->at(i));
                    if (inferred != inferred_readout.end())
                    {
                        readout = inferred->second;
                    }
                }
                event.channel_readout.push_back(readout);
                event.waveforms.emplace_back(adc->begin() + offset, adc->begin() + offset + count);
            }
        }

        tree->ResetBranchAddresses();
        tree->SetBranchStatus("*", 1);
        return true;
    }

    int FindDelayedCoincidenceEvent(TFile& file)
    {
        auto* tree = dynamic_cast<TTree*>(file.Get("eventTree"));
        if (tree == nullptr)
        {
            return -1;
        }

        int waveforms = 0;
        std::vector<double>* optical_t = nullptr;
        tree->ResetBranchAddresses();
        tree->SetBranchStatus("*", 0);
        tree->SetBranchStatus("electronics_waveforms", 1);
        tree->SetBranchStatus("optical_t", 1);
        tree->SetBranchAddress("electronics_waveforms", &waveforms);
        tree->SetBranchAddress("optical_t", &optical_t);

        int best_event = -1;
        double best_spread = 0.0;
        for (Long64_t entry = 0; entry < tree->GetEntries(); ++entry)
        {
            tree->GetEntry(entry);
            if (waveforms < 2 || optical_t == nullptr || optical_t->size() < 2)
            {
                continue;
            }

            const auto [min_it, max_it] = std::minmax_element(optical_t->begin(), optical_t->end());
            const double spread_us = (*max_it - *min_it) / 1000.0;
            if (spread_us > best_spread)
            {
                best_spread = spread_us;
                best_event = static_cast<int>(entry);
            }
        }

        tree->ResetBranchAddresses();
        tree->SetBranchStatus("*", 1);
        return best_event;
    }

    void DrawEventDisplayIndividual(const EventData& event,
                                    const char* output_prefix,
                                    const char* title,
                                    int event_index)
    {
        double seed_x = std::numeric_limits<double>::quiet_NaN();
        double seed_y = std::numeric_limits<double>::quiet_NaN();
        if (!event.xv.empty())
        {
            seed_x = event.xv.front() * kMmToCm;
            seed_y = event.yv.front() * kMmToCm;
        }
        else if (!event.xs.empty())
        {
            seed_x = event.xs.front() * kMmToCm;
            seed_y = event.ys.front() * kMmToCm;
        }
        else if (!event.optical_x.empty())
        {
            seed_x = event.optical_x.front() * kMmToCm;
            seed_y = event.optical_y.front() * kMmToCm - 0.19;
        }
        if (!std::isfinite(seed_x))
        {
            seed_x = 0.0;
            seed_y = 0.0;
        }

        double local_xmin = seed_x - 6.0;
        double local_xmax = seed_x + 12.0;
        double local_ymin = seed_y - 4.0;
        double local_ymax = seed_y + 4.0;
        double xz_xmin = local_xmin;
        double xz_xmax = local_xmax;
        double xz_zmin = std::numeric_limits<double>::infinity();
        double xz_zmax = -std::numeric_limits<double>::infinity();

        for (std::size_t i = 0; i < event.xs.size() && i < event.xe.size(); ++i)
        {
            ExpandRange(event.xs[i] * kMmToCm, local_xmin, local_xmax);
            ExpandRange(event.xe[i] * kMmToCm, local_xmin, local_xmax);
            ExpandRange(event.ys[i] * kMmToCm, local_ymin, local_ymax);
            ExpandRange(event.ye[i] * kMmToCm, local_ymin, local_ymax);
            ExpandRange(event.xs[i] * kMmToCm, xz_xmin, xz_xmax);
            ExpandRange(event.xe[i] * kMmToCm, xz_xmin, xz_xmax);
            ExpandRange(event.zs[i] * kMmToCm, xz_zmin, xz_zmax);
            ExpandRange(event.ze[i] * kMmToCm, xz_zmin, xz_zmax);
        }
        for (std::size_t i = 0; i < event.xv.size() && i < event.xf.size(); ++i)
        {
            ExpandRange(event.xv[i] * kMmToCm, local_xmin, local_xmax);
            ExpandRange(event.xf[i] * kMmToCm, local_xmin, local_xmax);
            ExpandRange(event.yv[i] * kMmToCm, local_ymin, local_ymax);
            ExpandRange(event.yf[i] * kMmToCm, local_ymin, local_ymax);
            ExpandRange(event.xv[i] * kMmToCm, xz_xmin, xz_xmax);
            ExpandRange(event.xf[i] * kMmToCm, xz_xmin, xz_xmax);
            ExpandRange(event.zv[i] * kMmToCm, xz_zmin, xz_zmax);
            ExpandRange(event.zf[i] * kMmToCm, xz_zmin, xz_zmax);
        }
        for (std::size_t i = 0; i < event.optical_x.size() && i < event.optical_z.size(); ++i)
        {
            ExpandRange(event.optical_x[i] * kMmToCm, xz_xmin, xz_xmax);
            ExpandRange(event.optical_z[i] * kMmToCm, xz_zmin, xz_zmax);
        }

        PadRange(local_xmin, local_xmax, 10.0);
        PadRange(local_ymin, local_ymax, 6.0);
        PadRange(xz_xmin, xz_xmax, 120.0);
        PadRange(xz_zmin, xz_zmax, 500.0);

        auto draw_line = [](double x1, double y1, double x2, double y2, int color, int width, double alpha) {
            auto* line = new TLine(x1, y1, x2, y2);
            line->SetLineColorAlpha(color, alpha);
            line->SetLineWidth(width);
            line->Draw();
        };
        const int charged = Color("#8b5cf6");
        const int photon = Color("#9aa6b2");
        const int optical = Color("#0891b2");

        {
            auto* canvas = new TCanvas("dunevd_event_local_xy", "DUNE VD local optical-node display", 1100, 850);
            canvas->SetFillColor(kWhite);
            gPad->SetGrid(1, 1);
            auto* hxy = new TH2D("local_xy_individual", "", 10, local_xmin, local_xmax, 10, local_ymin, local_ymax);
            hxy->SetStats(false);
            hxy->SetTitle(TString::Format("%s event %d local node view;X [cm];Y [cm]", title, event_index));
            hxy->Draw();
            DrawNodeGeometryXY(seed_x, seed_y);

            for (std::size_t i = 0; i < event.xs.size() && i < event.xe.size(); ++i)
            {
                int color = charged;
                if (i < event.step_pdg.size() && event.step_pdg[i] == 22)
                {
                    color = photon;
                }
                draw_line(event.xs[i] * kMmToCm,
                          event.ys[i] * kMmToCm,
                          event.xe[i] * kMmToCm,
                          event.ye[i] * kMmToCm,
                          color,
                          color == charged ? 2 : 1,
                          color == charged ? 0.58 : 0.28);
            }
            if (event.xs.empty())
            {
                for (std::size_t i = 0; i < event.xv.size() && i < event.xf.size(); ++i)
                {
                    draw_line(event.xv[i] * kMmToCm,
                              event.yv[i] * kMmToCm,
                              event.xf[i] * kMmToCm,
                              event.yf[i] * kMmToCm,
                              optical,
                              3,
                              0.86);
                }
            }
            for (std::size_t i = 0; i < event.optical_x.size() && i < event.optical_y.size(); ++i)
            {
                auto* marker = new TMarker(event.optical_x[i] * kMmToCm,
                                           event.optical_y[i] * kMmToCm,
                                           29);
                marker->SetMarkerColor(Color("#f97316"));
                marker->SetMarkerSize(1.6);
                marker->Draw();
            }

            auto* legend = new TLegend(0.13, 0.76, 0.56, 0.90);
            legend->SetBorderSize(0);
            legend->SetFillStyle(0);
            auto* sensor_sample = new TBox(0, 0, 1, 1);
            sensor_sample->SetFillColorAlpha(Color("#2b6f91"), 0.38);
            sensor_sample->SetLineColor(Color("#155e75"));
            auto* hit_sample = new TMarker(0, 0, 29);
            hit_sample->SetMarkerColor(Color("#f97316"));
            auto* track_sample = new TLine(0, 0, 1, 1);
            track_sample->SetLineColor(event.xs.empty() ? optical : charged);
            track_sample->SetLineWidth(2);
            legend->AddEntry(sensor_sample, "FastDPSU sensitive tile", "f");
            legend->AddEntry(track_sample, event.xs.empty() ? "optical photon track" : "charged/secondary steps", "l");
            legend->AddEntry(hit_sample, "recorded optical hit", "p");
            legend->Draw();

            const TString output = TString::Format("%s_local_xy.png", output_prefix);
            canvas->SaveAs(output.Data());
            std::cout << "Saved " << output << std::endl;
            delete canvas;
        }

        {
            auto* canvas = new TCanvas("dunevd_event_xz", "DUNE VD X-Z optical-node display", 1100, 850);
            canvas->SetFillColor(kWhite);
            gPad->SetGrid(1, 1);
            auto* hxz = new TH2D("xz_projection_individual", "", 10, xz_xmin, xz_xmax, 10, xz_zmin, xz_zmax);
            hxz->SetStats(false);
            hxz->SetTitle(TString::Format("%s event %d X-Z projection;X [cm];Z [cm]", title, event_index));
            hxz->Draw();
            for (std::size_t i = 0; i < event.xs.size() && i < event.xe.size(); ++i)
            {
                int color = charged;
                if (i < event.step_pdg.size() && event.step_pdg[i] == 22)
                {
                    color = photon;
                }
                draw_line(event.xs[i] * kMmToCm,
                          event.zs[i] * kMmToCm,
                          event.xe[i] * kMmToCm,
                          event.ze[i] * kMmToCm,
                          color,
                          color == charged ? 2 : 1,
                          color == charged ? 0.46 : 0.20);
            }
            if (event.xs.empty())
            {
                for (std::size_t i = 0; i < event.xv.size() && i < event.xf.size(); ++i)
                {
                    draw_line(event.xv[i] * kMmToCm,
                              event.zv[i] * kMmToCm,
                              event.xf[i] * kMmToCm,
                              event.zf[i] * kMmToCm,
                              optical,
                              3,
                              0.86);
                }
            }
            for (std::size_t i = 0; i < event.optical_x.size() && i < event.optical_z.size(); ++i)
            {
                auto* marker = new TMarker(event.optical_x[i] * kMmToCm,
                                           event.optical_z[i] * kMmToCm,
                                           29);
                marker->SetMarkerColor(Color("#f97316"));
                marker->SetMarkerSize(1.5);
                marker->Draw();
            }
            DrawLabel(0.14, 0.86, TString::Format("hits: %d  fast: %d  waveforms: %d",
                                                  event.optical_hits,
                                                  event.fast_optical_hits,
                                                  event.electronics_waveforms)
                                      .Data(),
                      0.032);

            const TString output = TString::Format("%s_xz.png", output_prefix);
            canvas->SaveAs(output.Data());
            std::cout << "Saved " << output << std::endl;
            delete canvas;
        }
    }

    void DrawEventDisplay(const EventData& event,
                          const char* output,
                          const char* title,
                          int event_index)
    {
        double seed_x = std::numeric_limits<double>::quiet_NaN();
        double seed_y = std::numeric_limits<double>::quiet_NaN();
        if (!event.xv.empty())
        {
            seed_x = event.xv.front() * kMmToCm;
            seed_y = event.yv.front() * kMmToCm;
        }
        else if (!event.xs.empty())
        {
            seed_x = event.xs.front() * kMmToCm;
            seed_y = event.ys.front() * kMmToCm;
        }
        else if (!event.optical_x.empty())
        {
            seed_x = event.optical_x.front() * kMmToCm;
            seed_y = event.optical_y.front() * kMmToCm - 0.19;
        }
        if (!std::isfinite(seed_x))
        {
            seed_x = 0.0;
            seed_y = 0.0;
        }

        double local_xmin = seed_x - 6.0;
        double local_xmax = seed_x + 12.0;
        double local_ymin = seed_y - 4.0;
        double local_ymax = seed_y + 4.0;
        for (std::size_t i = 0; i < event.xs.size() && i < event.xe.size(); ++i)
        {
            ExpandRange(event.xs[i] * kMmToCm, local_xmin, local_xmax);
            ExpandRange(event.xe[i] * kMmToCm, local_xmin, local_xmax);
            ExpandRange(event.ys[i] * kMmToCm, local_ymin, local_ymax);
            ExpandRange(event.ye[i] * kMmToCm, local_ymin, local_ymax);
        }
        for (std::size_t i = 0; i < event.xv.size() && i < event.xf.size(); ++i)
        {
            ExpandRange(event.xv[i] * kMmToCm, local_xmin, local_xmax);
            ExpandRange(event.xf[i] * kMmToCm, local_xmin, local_xmax);
            ExpandRange(event.yv[i] * kMmToCm, local_ymin, local_ymax);
            ExpandRange(event.yf[i] * kMmToCm, local_ymin, local_ymax);
        }
        PadRange(local_xmin, local_xmax, 10.0);
        PadRange(local_ymin, local_ymax, 6.0);

        double xz_xmin = local_xmin;
        double xz_xmax = local_xmax;
        double xz_zmin = std::numeric_limits<double>::infinity();
        double xz_zmax = -std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < event.xs.size() && i < event.xe.size(); ++i)
        {
            ExpandRange(event.xs[i] * kMmToCm, xz_xmin, xz_xmax);
            ExpandRange(event.xe[i] * kMmToCm, xz_xmin, xz_xmax);
            ExpandRange(event.zs[i] * kMmToCm, xz_zmin, xz_zmax);
            ExpandRange(event.ze[i] * kMmToCm, xz_zmin, xz_zmax);
        }
        for (std::size_t i = 0; i < event.xv.size() && i < event.xf.size(); ++i)
        {
            ExpandRange(event.xv[i] * kMmToCm, xz_xmin, xz_xmax);
            ExpandRange(event.xf[i] * kMmToCm, xz_xmin, xz_xmax);
            ExpandRange(event.zv[i] * kMmToCm, xz_zmin, xz_zmax);
            ExpandRange(event.zf[i] * kMmToCm, xz_zmin, xz_zmax);
        }
        for (std::size_t i = 0; i < event.optical_x.size() && i < event.optical_z.size(); ++i)
        {
            ExpandRange(event.optical_x[i] * kMmToCm, xz_xmin, xz_xmax);
            ExpandRange(event.optical_z[i] * kMmToCm, xz_zmin, xz_zmax);
        }
        PadRange(xz_xmin, xz_xmax, 120.0);
        PadRange(xz_zmin, xz_zmax, 500.0);

        auto* canvas = new TCanvas("dunevd_event_display", "DUNE VD optical event display", 1600, 800);
        canvas->SetFillColor(kWhite);
        canvas->Divide(2, 1);

        canvas->cd(1);
        gPad->SetGrid(1, 1);
        auto* hxy = new TH2D("local_xy", "", 10, local_xmin, local_xmax, 10, local_ymin, local_ymax);
        hxy->SetStats(false);
        hxy->SetTitle(TString::Format("%s event %d;X [cm];Y [cm]", title, event_index));
        hxy->Draw();
        DrawNodeGeometryXY(seed_x, seed_y);

        auto draw_line = [](double x1, double y1, double x2, double y2, int color, int width, double alpha) {
            auto* line = new TLine(x1, y1, x2, y2);
            line->SetLineColorAlpha(color, alpha);
            line->SetLineWidth(width);
            line->Draw();
        };

        const int charged = Color("#8b5cf6");
        const int photon = Color("#9aa6b2");
        const int optical = Color("#0891b2");
        for (std::size_t i = 0; i < event.xs.size() && i < event.xe.size(); ++i)
        {
            int color = charged;
            if (i < event.step_pdg.size() && event.step_pdg[i] == 22)
            {
                color = photon;
            }
            draw_line(event.xs[i] * kMmToCm,
                      event.ys[i] * kMmToCm,
                      event.xe[i] * kMmToCm,
                      event.ye[i] * kMmToCm,
                      color,
                      color == charged ? 2 : 1,
                      color == charged ? 0.58 : 0.28);
        }
        if (event.xs.empty())
        {
            for (std::size_t i = 0; i < event.xv.size() && i < event.xf.size(); ++i)
            {
                draw_line(event.xv[i] * kMmToCm,
                          event.yv[i] * kMmToCm,
                          event.xf[i] * kMmToCm,
                          event.yf[i] * kMmToCm,
                          optical,
                          3,
                          0.86);
            }
        }

        for (std::size_t i = 0; i < event.optical_x.size() && i < event.optical_y.size(); ++i)
        {
            auto* marker = new TMarker(event.optical_x[i] * kMmToCm,
                                       event.optical_y[i] * kMmToCm,
                                       29);
            marker->SetMarkerColor(Color("#f97316"));
            marker->SetMarkerSize(1.4);
            marker->Draw();
        }

        auto* legend = new TLegend(0.13, 0.75, 0.56, 0.90);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        auto* sensor_sample = new TBox(0, 0, 1, 1);
        sensor_sample->SetFillColorAlpha(Color("#2b6f91"), 0.38);
        sensor_sample->SetLineColor(Color("#155e75"));
        auto* hit_sample = new TMarker(0, 0, 29);
        hit_sample->SetMarkerColor(Color("#f97316"));
        auto* track_sample = new TLine(0, 0, 1, 1);
        track_sample->SetLineColor(charged);
        track_sample->SetLineWidth(2);
        legend->AddEntry(sensor_sample, "FastDPSU sensitive tile", "f");
        legend->AddEntry(track_sample, event.xs.empty() ? "optical photon track" : "charged/secondary steps", "l");
        legend->AddEntry(hit_sample, "recorded optical hit", "p");
        legend->Draw();

        canvas->cd(2);
        gPad->SetGrid(1, 1);
        auto* hxz = new TH2D("xz_projection", "", 10, xz_xmin, xz_xmax, 10, xz_zmin, xz_zmax);
        hxz->SetStats(false);
        hxz->SetTitle("X-Z projection with optical-node hits;X [cm];Z [cm]");
        hxz->Draw();
        for (std::size_t i = 0; i < event.xs.size() && i < event.xe.size(); ++i)
        {
            int color = charged;
            if (i < event.step_pdg.size() && event.step_pdg[i] == 22)
            {
                color = photon;
            }
            draw_line(event.xs[i] * kMmToCm,
                      event.zs[i] * kMmToCm,
                      event.xe[i] * kMmToCm,
                      event.ze[i] * kMmToCm,
                      color,
                      color == charged ? 2 : 1,
                      color == charged ? 0.46 : 0.20);
        }
        if (event.xs.empty())
        {
            for (std::size_t i = 0; i < event.xv.size() && i < event.xf.size(); ++i)
            {
                draw_line(event.xv[i] * kMmToCm,
                          event.zv[i] * kMmToCm,
                          event.xf[i] * kMmToCm,
                          event.zf[i] * kMmToCm,
                          optical,
                          3,
                          0.86);
            }
        }
        for (std::size_t i = 0; i < event.optical_x.size() && i < event.optical_z.size(); ++i)
        {
            auto* marker = new TMarker(event.optical_x[i] * kMmToCm,
                                       event.optical_z[i] * kMmToCm,
                                       29);
            marker->SetMarkerColor(Color("#f97316"));
            marker->SetMarkerSize(1.3);
            marker->Draw();
        }

        DrawLabel(0.14, 0.86, TString::Format("hits: %d  fast: %d  waveforms: %d",
                                              event.optical_hits,
                                              event.fast_optical_hits,
                                              event.electronics_waveforms)
                                  .Data(),
                  0.032);

        canvas->SaveAs(output);
        delete canvas;
    }

    void DrawWaveformDistributions(const WaveformData& data, const char* output, const char* title)
    {
        auto* canvas = new TCanvas("dunevd_waveforms", "DUNE VD waveform distributions", 1600, 1100);
        canvas->SetFillColor(kWhite);
        canvas->Divide(2, 2);

        int max_hits = 1;
        for (int value : data.hits_per_event) max_hits = std::max(max_hits, value);
        canvas->cd(1);
        gPad->SetGrid(1, 1);
        auto* h_hits = new TH1D("hits_per_event",
                                TString::Format("%s;optical hits per event;events", title),
                                std::max(1, max_hits + 1),
                                -0.5,
                                max_hits + 0.5);
        for (int value : data.hits_per_event) h_hits->Fill(value);
        h_hits->SetFillColorAlpha(Color("#38bdf8"), 0.45);
        h_hits->SetLineColor(Color("#0369a1"));
        h_hits->Draw("hist");

        canvas->cd(2);
        gPad->SetGrid(1, 1);
        double adc_min = 0.0;
        double adc_max = 1.0;
        if (!data.adc_samples.empty())
        {
            auto [min_it, max_it] = std::minmax_element(data.adc_samples.begin(), data.adc_samples.end());
            adc_min = *min_it;
            adc_max = *max_it;
            PadRange(adc_min, adc_max, 10.0, 0.05);
        }
        auto* h_adc = new TH1D("adc_samples",
                               "ADC sample distribution;ADC;sample count",
                               80,
                               adc_min,
                               adc_max);
        for (short value : data.adc_samples) h_adc->Fill(value);
        h_adc->SetFillColorAlpha(Color("#a78bfa"), 0.45);
        h_adc->SetLineColor(Color("#6d28d9"));
        h_adc->Draw("hist");

        canvas->cd(3);
        gPad->SetGrid(1, 1);
        double peak_max = 1.0;
        for (double peak : data.waveform_peak) peak_max = std::max(peak_max, peak);
        auto* h_peak = new TH1D("waveform_peak",
                                "Waveform peak distribution;peak ADC;waveforms",
                                50,
                                0.0,
                                1.15 * peak_max);
        for (double peak : data.waveform_peak) h_peak->Fill(peak);
        h_peak->SetFillColorAlpha(Color("#fb923c"), 0.45);
        h_peak->SetLineColor(Color("#c2410c"));
        h_peak->Draw("hist");

        canvas->cd(4);
        gPad->SetGrid(1, 1);
        auto* frame = new TH2D("waveform_examples",
                               "Example waveforms;time [#mus];ADC",
                               10,
                               0.0,
                               data.example_waveforms.empty()
                                   ? 2.0
                                   : data.example_waveforms.front().size() / data.sample_frequency_mhz,
                               10,
                               adc_min,
                               adc_max);
        frame->SetStats(false);
        frame->Draw();
        const int colors[] = {Color("#0f766e"), Color("#2563eb"), Color("#c026d3"), Color("#ea580c"),
                              Color("#16a34a"), Color("#7c3aed"), Color("#dc2626"), Color("#0891b2")};
        for (std::size_t i = 0; i < data.example_waveforms.size(); ++i)
        {
            const auto& waveform = data.example_waveforms[i];
            auto* graph = new TGraph(waveform.size());
            for (std::size_t sample = 0; sample < waveform.size(); ++sample)
            {
                graph->SetPoint(sample, sample / data.sample_frequency_mhz, waveform[sample]);
            }
            graph->SetLineColorAlpha(colors[i % 8], 0.70);
            graph->SetLineWidth(2);
            graph->Draw("L same");
        }
        DrawLabel(0.13, 0.86, TString::Format("events: %d  waveforms: %zu  samples: %zu",
                                              data.events,
                                              data.waveform_peak.size(),
                                              data.adc_samples.size())
                                  .Data(),
                  0.030);

        canvas->SaveAs(output);
        delete canvas;
    }

    void DrawWaveformDistributionsIndividual(const WaveformData& data,
                                             const char* output_prefix,
                                             const char* title)
    {
        int max_hits = 1;
        for (int value : data.hits_per_event) max_hits = std::max(max_hits, value);

        {
            auto* canvas = new TCanvas("hits_per_event_individual", "Hits per event", 1000, 760);
            canvas->SetFillColor(kWhite);
            gPad->SetGrid(1, 1);
            auto* hist = new TH1D("hits_per_event_single",
                                  TString::Format("%s;optical hits per event;events", title),
                                  std::max(1, max_hits + 1),
                                  -0.5,
                                  max_hits + 0.5);
            for (int value : data.hits_per_event) hist->Fill(value);
            hist->SetFillColorAlpha(Color("#38bdf8"), 0.45);
            hist->SetLineColor(Color("#0369a1"));
            hist->SetLineWidth(2);
            hist->Draw("hist");
            const TString output = TString::Format("%s_hits_per_event.png", output_prefix);
            canvas->SaveAs(output.Data());
            std::cout << "Saved " << output << std::endl;
            delete canvas;
        }

        double adc_min = 0.0;
        double adc_max = 1.0;
        if (!data.adc_samples.empty())
        {
            const auto minmax = std::minmax_element(data.adc_samples.begin(), data.adc_samples.end());
            adc_min = *minmax.first;
            adc_max = *minmax.second;
            PadRange(adc_min, adc_max, 10.0, 0.05);
        }

        {
            auto* canvas = new TCanvas("adc_samples_individual", "ADC sample distribution", 1000, 760);
            canvas->SetFillColor(kWhite);
            gPad->SetGrid(1, 1);
            auto* hist = new TH1D("adc_samples_single",
                                  "ADC sample distribution;ADC;sample count",
                                  80,
                                  adc_min,
                                  adc_max);
            for (short value : data.adc_samples) hist->Fill(value);
            hist->SetFillColorAlpha(Color("#a78bfa"), 0.45);
            hist->SetLineColor(Color("#6d28d9"));
            hist->SetLineWidth(2);
            hist->Draw("hist");
            const TString output = TString::Format("%s_adc_samples.png", output_prefix);
            canvas->SaveAs(output.Data());
            std::cout << "Saved " << output << std::endl;
            delete canvas;
        }

        if (!data.node_adc_samples.empty() || !data.arapuca_adc_samples.empty())
        {
            auto* canvas = new TCanvas("adc_samples_by_readout", "ADC samples by readout", 1000, 760);
            canvas->SetFillColor(kWhite);
            gPad->SetGrid(1, 1);
            auto* frame = new TH1D("adc_samples_by_readout_frame",
                                   "ADC sample distribution by optical readout;ADC;sample count",
                                   80,
                                   adc_min,
                                   adc_max);
            frame->SetStats(false);
            frame->SetMinimum(0.0);

            auto make_hist = [&](const char* name,
                                 const std::vector<short>& samples,
                                 const std::string& readout) {
                auto* hist = new TH1D(name, "", 80, adc_min, adc_max);
                for (short value : samples) hist->Fill(value);
                hist->SetLineColor(ReadoutColor(readout));
                hist->SetFillColorAlpha(ReadoutColor(readout), 0.22);
                hist->SetLineWidth(3);
                hist->SetLineStyle(ReadoutLineStyle(readout));
                return hist;
            };

            auto* node_hist = make_hist("adc_samples_node", data.node_adc_samples, "node");
            auto* arapuca_hist = make_hist("adc_samples_arapuca", data.arapuca_adc_samples, "arapuca");
            frame->SetMaximum(1.18 * std::max(node_hist->GetMaximum(), arapuca_hist->GetMaximum()));
            frame->Draw("hist");
            if (!data.node_adc_samples.empty()) node_hist->Draw("hist same");
            if (!data.arapuca_adc_samples.empty()) arapuca_hist->Draw("hist same");

            auto* legend = new TLegend(0.62, 0.74, 0.90, 0.88);
            legend->SetBorderSize(0);
            legend->SetFillStyle(0);
            if (!data.node_adc_samples.empty()) legend->AddEntry(node_hist, "FastDPSU node ADC", "lf");
            if (!data.arapuca_adc_samples.empty()) legend->AddEntry(arapuca_hist, "ARAPUCA ADC", "lf");
            legend->Draw();

            const TString output = TString::Format("%s_adc_samples_by_readout.png", output_prefix);
            canvas->SaveAs(output.Data());
            std::cout << "Saved " << output << std::endl;
            delete canvas;
        }

        {
            double peak_min = std::numeric_limits<double>::infinity();
            double peak_max = -std::numeric_limits<double>::infinity();
            for (double peak : data.waveform_peak)
            {
                ExpandRange(peak, peak_min, peak_max);
            }
            PadRange(peak_min, peak_max, 10.0, 0.08);
            auto* canvas = new TCanvas("waveform_peak_individual", "Waveform peak distribution", 1000, 760);
            canvas->SetFillColor(kWhite);
            gPad->SetGrid(1, 1);
            auto* hist = new TH1D("waveform_peak_single",
                                  "Waveform peak distribution;peak ADC;waveforms",
                                  50,
                                  peak_min,
                                  peak_max);
            for (double peak : data.waveform_peak) hist->Fill(peak);
            hist->SetFillColorAlpha(Color("#fb923c"), 0.45);
            hist->SetLineColor(Color("#c2410c"));
            hist->SetLineWidth(2);
            hist->Draw("hist");
            const TString output = TString::Format("%s_waveform_peaks.png", output_prefix);
            canvas->SaveAs(output.Data());
            std::cout << "Saved " << output << std::endl;
            delete canvas;
        }
    }

    void DrawDelayedCoincidenceWaveform(const EventWaveformData& event,
                                        const char* output,
                                        const char* title,
                                        int event_index)
    {
        if (event.waveforms.empty())
        {
            return;
        }

        std::size_t sample_count = 0;
        for (const auto& waveform : event.waveforms)
        {
            sample_count = std::max(sample_count, waveform.size());
        }
        if (sample_count == 0 || event.sample_frequency_mhz <= 0.0)
        {
            return;
        }

        std::vector<double> summed(sample_count, 0.0);
        std::vector<std::vector<double>> centered_waveforms;
        centered_waveforms.reserve(event.waveforms.size());
        double ymin = std::numeric_limits<double>::infinity();
        double ymax = -std::numeric_limits<double>::infinity();

        for (const auto& waveform : event.waveforms)
        {
            const double baseline = Median(waveform);
            std::vector<double> centered(sample_count, 0.0);
            for (std::size_t sample = 0; sample < waveform.size(); ++sample)
            {
                centered[sample] = waveform[sample] - baseline;
                summed[sample] += centered[sample];
                ExpandRange(centered[sample], ymin, ymax);
            }
            centered_waveforms.push_back(std::move(centered));
        }
        for (double value : summed)
        {
            ExpandRange(value, ymin, ymax);
        }
        PadRange(ymin, ymax, 12.0, 0.18);

        const double tmin = event.time_begin_us;
        const double waveform_end_us = tmin + sample_count / event.sample_frequency_mhz;
        double tmax = event.time_end_us > tmin ? event.time_end_us : waveform_end_us;
        if (!event.optical_t_us.empty())
        {
            tmax = std::min(tmax, std::max(tmin + 1.0, event.optical_t_us.back() + 0.75));
        }

        auto* canvas = new TCanvas("delayed_coincidence_waveform",
                                   "Delayed coincidence waveform",
                                   1400,
                                   820);
        canvas->SetFillColor(kWhite);
        canvas->SetLeftMargin(0.10);
        canvas->SetRightMargin(0.24);
        canvas->SetBottomMargin(0.11);
        gPad->SetGrid(1, 1);
        auto* frame = new TH2D("delayed_coincidence_frame",
                               TString::Format("%s event %d delayed coincidence channel decomposition;time [#mus];ADC - median baseline",
                                               title,
                                               event_index),
                               10,
                               tmin,
                               tmax,
                               10,
                               ymin,
                               ymax);
        frame->SetStats(false);
        frame->Draw();

        const int colors[] = {Color("#2563eb"), Color("#0f766e"), Color("#c026d3"), Color("#ea580c"),
                              Color("#16a34a"), Color("#7c3aed"), Color("#dc2626"), Color("#0891b2")};
        auto* legend = new TLegend(0.78, 0.14, 0.98, 0.90);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextSize(event.waveforms.size() > 12 ? 0.016 : 0.020);

        for (std::size_t i = 0; i < centered_waveforms.size(); ++i)
        {
            auto* graph = new TGraph(sample_count);
            for (std::size_t sample = 0; sample < sample_count; ++sample)
            {
                graph->SetPoint(sample,
                                tmin + sample / event.sample_frequency_mhz,
                                centered_waveforms[i][sample]);
            }
            graph->SetLineColorAlpha(colors[i % 8], 0.50);
            graph->SetLineWidth(2);
            graph->Draw("L same");
            const int channel = i < event.channels.size() ? event.channels[i] : static_cast<int>(i);
            legend->AddEntry(graph, TString::Format("channel %d", channel), "l");
        }

        auto* sum_graph = new TGraph(sample_count);
        for (std::size_t sample = 0; sample < sample_count; ++sample)
        {
            sum_graph->SetPoint(sample,
                                tmin + sample / event.sample_frequency_mhz,
                                summed[sample]);
        }
        sum_graph->SetLineColor(Color("#111827"));
        sum_graph->SetLineWidth(4);
        sum_graph->Draw("L same");
        legend->AddEntry(sum_graph, "channel sum", "l");

        const double yrange = ymax - ymin;
        for (std::size_t i = 0; i < event.optical_t_us.size(); ++i)
        {
            const double time_us = event.optical_t_us[i];
            if (time_us < tmin || time_us > tmax)
            {
                continue;
            }
            auto* line = new TLine(time_us, ymin, time_us, ymax);
            line->SetLineColor(i + 1 == event.optical_t_us.size() ? Color("#dc2626") : Color("#f97316"));
            line->SetLineStyle(i + 1 == event.optical_t_us.size() ? 2 : 3);
            line->SetLineWidth(3);
            line->Draw();

            TLatex label;
            label.SetTextFont(42);
            label.SetTextSize(0.027);
            label.SetTextColor(line->GetLineColor());
            label.DrawLatex(time_us + 0.04,
                            ymax - (0.12 + 0.07 * (i % 3)) * yrange,
                            TString::Format("hit %.3f #mus", time_us));
        }

        if (event.optical_t_us.size() >= 2)
        {
            const double delay = event.optical_t_us.back() - event.optical_t_us.front();
            DrawLabel(0.14,
                      0.86,
                      TString::Format("event %d: %d hits, %d waveforms, max #Deltat = %.3f #mus",
                                      event_index,
                                      event.optical_hits,
                                      event.electronics_waveforms,
                                      delay)
                          .Data(),
                      0.032);
        }
        legend->Draw();

        canvas->SaveAs(output);
        std::cout << "Saved " << output << std::endl;
        delete canvas;
    }

    void DrawDelayedCoincidenceRootStyleChannels(const EventWaveformData& event,
                                                 const char* output,
                                                 int event_index)
    {
        if (event.waveforms.empty() || event.sample_frequency_mhz <= 0.0)
        {
            return;
        }

        std::size_t sample_count = 0;
        for (const auto& waveform : event.waveforms)
        {
            sample_count = std::max(sample_count, waveform.size());
        }
        if (sample_count == 0)
        {
            return;
        }

        std::vector<double> summed(sample_count, 0.0);
        std::vector<double> node_sum(sample_count, 0.0);
        std::vector<double> arapuca_sum(sample_count, 0.0);
        std::vector<double> unknown_sum(sample_count, 0.0);
        std::vector<std::vector<double>> centered_waveforms;
        std::vector<double> channel_max_abs;
        centered_waveforms.reserve(event.waveforms.size());
        channel_max_abs.reserve(event.waveforms.size());
        double ymin = std::numeric_limits<double>::infinity();
        double ymax = -std::numeric_limits<double>::infinity();
        bool has_node = false;
        bool has_arapuca = false;
        bool has_unknown = false;

        for (std::size_t waveform_index = 0; waveform_index < event.waveforms.size(); ++waveform_index)
        {
            const auto& waveform = event.waveforms[waveform_index];
            const std::string readout =
                waveform_index < event.channel_readout.size()
                    ? NormalizeReadout(event.channel_readout[waveform_index])
                    : "unknown";
            const double baseline = Median(waveform);
            std::vector<double> centered(sample_count, 0.0);
            double max_abs = 0.0;
            for (std::size_t sample = 0; sample < waveform.size(); ++sample)
            {
                centered[sample] = waveform[sample] - baseline;
                summed[sample] += centered[sample];
                if (readout == "node")
                {
                    node_sum[sample] += centered[sample];
                    has_node = true;
                }
                else if (readout == "arapuca")
                {
                    arapuca_sum[sample] += centered[sample];
                    has_arapuca = true;
                }
                else
                {
                    unknown_sum[sample] += centered[sample];
                    has_unknown = true;
                }
                max_abs = std::max(max_abs, std::abs(centered[sample]));
                ExpandRange(centered[sample], ymin, ymax);
            }
            channel_max_abs.push_back(max_abs);
            centered_waveforms.push_back(std::move(centered));
        }
        for (double value : summed)
        {
            ExpandRange(value, ymin, ymax);
        }
        for (double value : node_sum) ExpandRange(value, ymin, ymax);
        for (double value : arapuca_sum) ExpandRange(value, ymin, ymax);
        for (double value : unknown_sum) ExpandRange(value, ymin, ymax);
        PadRange(ymin, ymax, 12.0, 0.18);

        double tmax_us = event.time_end_us;
        if (!event.optical_t_us.empty())
        {
            tmax_us = std::min(tmax_us, std::max(event.time_begin_us + 1.0,
                                                 event.optical_t_us.back() + 0.75));
        }
        if (tmax_us <= event.time_begin_us)
        {
            tmax_us = event.time_begin_us + sample_count / event.sample_frequency_mhz;
        }

        const double tmin_ns = event.time_begin_us * 1000.0;
        const double tmax_ns = tmax_us * 1000.0;
        const double sample_width_ns = 1000.0 / event.sample_frequency_mhz;

        gStyle->SetOptStat(0);
        gStyle->SetGridColor(Color("#d9d9d9"));
        gStyle->SetGridStyle(1);
        gStyle->SetGridWidth(1);
        gStyle->SetTitleFont(42, "xyz");
        gStyle->SetLabelFont(42, "xyz");
        gStyle->SetTitleSize(0.043, "xyz");
        gStyle->SetLabelSize(0.037, "xyz");

        auto* canvas = new TCanvas("delayed_coincidence_root_style_channels",
                                   "Delayed coincidence channel decomposition",
                                   1000,
                                   680);
        canvas->SetFillColor(kWhite);
        canvas->SetLeftMargin(0.12);
        canvas->SetRightMargin(0.24);
        canvas->SetTopMargin(0.04);
        canvas->SetBottomMargin(0.12);
        canvas->SetGrid(1, 1);

        auto* frame = new TH2D(TString::Format("evt%d_delayed_channels_frame", event_index),
                               "",
                               10,
                               tmin_ns,
                               tmax_ns,
                               10,
                               ymin,
                               ymax);
        frame->SetStats(false);
        frame->GetXaxis()->SetTitle("Time of signal [ns]");
        frame->GetYaxis()->SetTitle("ADC - median baseline");
        frame->GetXaxis()->SetTitleOffset(1.03);
        frame->GetYaxis()->SetTitleOffset(1.18);
        frame->Draw();

        std::vector<std::size_t> order(centered_waveforms.size());
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](std::size_t lhs, std::size_t rhs) {
            return channel_max_abs[lhs] < channel_max_abs[rhs];
        });

        auto* legend = new TLegend(0.78, 0.13, 0.98, 0.90);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextSize(centered_waveforms.size() > 12 ? 0.015 : 0.019);

        const std::size_t legend_limit = std::min<std::size_t>(centered_waveforms.size(), 14);
        std::size_t legend_count = 0;
        for (std::size_t draw_index = 0; draw_index < order.size(); ++draw_index)
        {
            const std::size_t i = order[draw_index];
            auto* graph = new TGraph(sample_count);
            for (std::size_t sample = 0; sample < sample_count; ++sample)
            {
                const double time_ns = event.time_begin_us * 1000.0 + sample * sample_width_ns;
                graph->SetPoint(sample, time_ns, centered_waveforms[i][sample]);
            }
            const std::string readout =
                i < event.channel_readout.size()
                    ? NormalizeReadout(event.channel_readout[i])
                    : "unknown";
            graph->SetLineColorAlpha(ReadoutColor(readout), readout == "arapuca" ? 0.46 : 0.34);
            graph->SetLineStyle(ReadoutLineStyle(readout));
            graph->SetLineWidth(1);
            graph->Draw("L same");

            const bool strong_enough_for_legend =
                draw_index + legend_limit >= order.size() && legend_count < legend_limit;
            if (strong_enough_for_legend)
            {
                const int channel = i < event.channels.size() ? event.channels[i] : static_cast<int>(i);
                legend->AddEntry(graph,
                                 TString::Format("%s ch %d",
                                                 ReadoutDisplayName(readout),
                                                 channel),
                                 "l");
                ++legend_count;
            }
        }

        auto draw_sum = [&](const std::vector<double>& values,
                            const std::string& readout,
                            const char* label,
                            int width) {
            auto* graph = new TGraph(sample_count);
            for (std::size_t sample = 0; sample < sample_count; ++sample)
            {
                const double time_ns = event.time_begin_us * 1000.0 + sample * sample_width_ns;
                graph->SetPoint(sample, time_ns, values[sample]);
            }
            graph->SetLineColor(ReadoutColor(readout));
            graph->SetLineStyle(ReadoutLineStyle(readout));
            graph->SetLineWidth(width);
            graph->Draw("L same");
            legend->AddEntry(graph, label, "l");
            return graph;
        };

        if (has_node)
        {
            draw_sum(node_sum, "node", "node sum", 4);
        }
        if (has_arapuca)
        {
            draw_sum(arapuca_sum, "arapuca", "ARAPUCA sum", 4);
        }
        if (has_unknown && !has_node && !has_arapuca)
        {
            draw_sum(unknown_sum, "unknown", "unclassified sum", 3);
        }

        auto* sum_graph = new TGraph(sample_count);
        for (std::size_t sample = 0; sample < sample_count; ++sample)
        {
            const double time_ns = event.time_begin_us * 1000.0 + sample * sample_width_ns;
            sum_graph->SetPoint(sample, time_ns, summed[sample]);
        }
        sum_graph->SetLineColor(Color("#111827"));
        sum_graph->SetLineStyle(2);
        sum_graph->SetLineWidth(2);
        sum_graph->Draw("L same");
        legend->AddEntry(sum_graph, "total sum", "l");

        const double yrange = ymax - ymin;
        for (std::size_t i = 0; i < event.optical_t_us.size(); ++i)
        {
            const double time_ns = event.optical_t_us[i] * 1000.0;
            if (time_ns < tmin_ns || time_ns > tmax_ns)
            {
                continue;
            }
            auto* line = new TLine(time_ns, ymin, time_ns, ymax);
            line->SetLineColor(i + 1 == event.optical_t_us.size() ? Color("#dc2626") : Color("#f97316"));
            line->SetLineStyle(i + 1 == event.optical_t_us.size() ? 2 : 3);
            line->SetLineWidth(2);
            line->Draw();

            TLatex label;
            label.SetTextFont(42);
            label.SetTextSize(0.025);
            label.SetTextColor(line->GetLineColor());
            label.DrawLatex(time_ns + 35.0,
                            ymax - (0.12 + 0.07 * (i % 3)) * yrange,
                            TString::Format("hit %.0f ns", time_ns));
        }

        legend->Draw();
        canvas->SaveAs(output);
        std::cout << "Saved " << output << std::endl;
        delete canvas;
    }

    void DrawDelayedCoincidenceRootStyle(const EventWaveformData& event,
                                         const char* output,
                                         int event_index)
    {
        if (event.waveforms.empty() || event.sample_frequency_mhz <= 0.0)
        {
            return;
        }

        std::size_t sample_count = 0;
        for (const auto& waveform : event.waveforms)
        {
            sample_count = std::max(sample_count, waveform.size());
        }
        if (sample_count == 0)
        {
            return;
        }

        std::vector<double> summed(sample_count, 0.0);
        for (const auto& waveform : event.waveforms)
        {
            const double baseline = Median(waveform);
            for (std::size_t sample = 0; sample < waveform.size(); ++sample)
            {
                summed[sample] += waveform[sample] - baseline;
            }
        }

        double tmax_us = event.time_end_us;
        if (!event.optical_t_us.empty())
        {
            tmax_us = std::min(tmax_us, std::max(event.time_begin_us + 1.0,
                                                 event.optical_t_us.back() + 0.75));
        }
        if (tmax_us <= event.time_begin_us)
        {
            tmax_us = event.time_begin_us + sample_count / event.sample_frequency_mhz;
        }

        const double sample_width_ns = 1000.0 / event.sample_frequency_mhz;
        const double tmin_ns = event.time_begin_us * 1000.0;
        const double tmax_ns = tmax_us * 1000.0;
        const int bins = std::max(1, static_cast<int>(std::ceil((tmax_ns - tmin_ns) / sample_width_ns)));

        gStyle->SetOptStat(1111);
        gStyle->SetStatX(0.90);
        gStyle->SetStatY(0.90);
        gStyle->SetStatW(0.18);
        gStyle->SetStatH(0.13);
        gStyle->SetGridColor(Color("#d9d9d9"));
        gStyle->SetGridStyle(1);
        gStyle->SetGridWidth(1);
        gStyle->SetTitleFont(42, "xyz");
        gStyle->SetLabelFont(42, "xyz");
        gStyle->SetTitleSize(0.043, "xyz");
        gStyle->SetLabelSize(0.037, "xyz");

        auto* canvas = new TCanvas("delayed_coincidence_root_style",
                                   "Delayed coincidence waveform",
                                   900,
                                   650);
        canvas->SetFillColor(kWhite);
        canvas->SetLeftMargin(0.12);
        canvas->SetRightMargin(0.05);
        canvas->SetTopMargin(0.04);
        canvas->SetBottomMargin(0.12);
        canvas->SetGrid(1, 1);
        canvas->SetLogy(1);

        auto* hist = new TH1D(TString::Format("evt%d_delayed_coincidence", event_index),
                              "",
                              bins,
                              tmin_ns,
                              tmax_ns);
        hist->SetLineColor(Color("#4f63d8"));
        hist->SetLineWidth(1);
        hist->SetFillStyle(0);
        hist->GetXaxis()->SetTitle("Time of signal [ns]");
        hist->GetYaxis()->SetTitle("Optical-node signal [ADC/ns]");
        hist->GetXaxis()->SetTitleOffset(1.03);
        hist->GetYaxis()->SetTitleOffset(1.18);

        double ymax = 0.0;
        for (std::size_t sample = 0; sample < summed.size(); ++sample)
        {
            const double time_ns = event.time_begin_us * 1000.0 + sample * sample_width_ns;
            if (time_ns < tmin_ns || time_ns >= tmax_ns)
            {
                continue;
            }

            const double adc_per_ns = std::max(0.04, summed[sample] / sample_width_ns);
            hist->Fill(time_ns, adc_per_ns);
            ymax = std::max(ymax, adc_per_ns);
        }

        hist->SetMinimum(0.03);
        hist->SetMaximum(std::max(1.0, ymax * 8.0));
        hist->Draw("hist l");

        canvas->Update();
        if (auto* stats = dynamic_cast<TPaveStats*>(hist->FindObject("stats")))
        {
            stats->SetName(TString::Format("evt%d_delayed_stats", event_index));
            stats->SetBorderSize(1);
            stats->SetFillColor(kWhite);
        }
        canvas->Modified();
        canvas->SaveAs(output);
        std::cout << "Saved " << output << std::endl;
        delete canvas;
    }
}

void dunevd_optical_node_plots(const char* input_file,
                               int event_index,
                               const char* gdml_file,
                               const char* output_prefix,
                               const char* title = "DUNE VD FastDPSU optical",
                               int delayed_event_index = -1)
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(1110);
    gSystem->Load("libGeom");
    gSystem->Load("libGdml");

    TGeoManager::Import(gdml_file);
    if (gGeoManager == nullptr)
    {
        std::cerr << "Warning: failed to import GDML file " << gdml_file << std::endl;
    }

    TFile file(input_file, "READ");
    if (file.IsZombie())
    {
        std::cerr << "Failed to open " << input_file << std::endl;
        return;
    }

    EventData event;
    if (!LoadEvent(file, event_index, event))
    {
        std::cerr << "Failed to load event " << event_index << " from " << input_file << std::endl;
        return;
    }

    const TString event_output_prefix = TString::Format("%s_event%d", output_prefix, event_index);
    DrawEventDisplayIndividual(event, event_output_prefix.Data(), title, event_index);

    const WaveformData waveforms = LoadWaveforms(file);
    DrawWaveformDistributionsIndividual(waveforms, output_prefix, title);

    const int delayed_event = delayed_event_index >= 0 ? delayed_event_index : FindDelayedCoincidenceEvent(file);
    if (delayed_event >= 0)
    {
        EventWaveformData delayed_waveforms;
        if (LoadEventWaveforms(file, delayed_event, delayed_waveforms))
        {
            const TString delayed_output = TString::Format("%s_delayed_coincidence_event%d.png",
                                                          output_prefix,
                                                          delayed_event);
            DrawDelayedCoincidenceWaveform(delayed_waveforms,
                                           delayed_output.Data(),
                                           title,
                                           delayed_event);

            const TString delayed_channels_root_style_output =
                TString::Format("%s_delayed_coincidence_rootstyle_channels_event%d.png",
                                output_prefix,
                                delayed_event);
            DrawDelayedCoincidenceRootStyleChannels(delayed_waveforms,
                                                    delayed_channels_root_style_output.Data(),
                                                    delayed_event);

            const TString delayed_root_style_output = TString::Format("%s_delayed_coincidence_rootstyle_event%d.png",
                                                                     output_prefix,
                                                                     delayed_event);
            DrawDelayedCoincidenceRootStyle(delayed_waveforms,
                                            delayed_root_style_output.Data(),
                                            delayed_event);
        }
    }
    else
    {
        std::cout << "No multi-hit delayed coincidence waveform found in " << input_file << std::endl;
    }
}

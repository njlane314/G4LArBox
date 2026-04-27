#include "TArrow.h"
#include "TBox.h"
#include "TCanvas.h"
#include "TColor.h"
#include "TFile.h"
#include "TGeoBBox.h"
#include "TGeoManager.h"
#include "TGeoVolume.h"
#include "TGLLightSet.h"
#include "TGLRnrCtx.h"
#include "TGLUtil.h"
#include "TGLViewer.h"
#include "TLine.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TMarker.h"
#include "TPolyLine.h"
#include "TPolyLine3D.h"
#include "TPolyMarker3D.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TTree.h"
#include "TString.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    struct StepSegment
    {
        int pdg = 0;
        int parent = -1;
        int track = -1;
        double x1 = 0.0;
        double y1 = 0.0;
        double z1 = 0.0;
        double x2 = 0.0;
        double y2 = 0.0;
        double z2 = 0.0;
    };

    struct TrackPath
    {
        int pdg = 0;
        int parent = -1;
        int track = -1;
        std::size_t order = 0;
        double length = 0.0;
        std::vector<std::array<double, 3>> points;
    };

    struct TruthInfo
    {
        bool has_truth = false;
        bool has_neutrino = false;
        bool has_incident_direction = false;
        std::string source = "unknown";
        int genie_neu = 0;
        double vertex_x = 0.0;
        double vertex_y = 0.0;
        double vertex_z = 0.0;
        double incident_dir_x = 0.0;
        double incident_dir_y = 0.0;
        double incident_dir_z = 0.0;
    };

    struct BoxGeometry
    {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        double dx = 128.175;
        double dy = 116.5;
        double dz = 518.4;
    };

    struct DisplayOptions
    {
        std::set<int> pdg_filter;
        bool primary_only = false;
        double min_step_cm = 0.005;
        int max_tracks = 2000;
    };

    struct RenderMode
    {
        std::string name;
        bool debug = false;
    };

    struct DrawPath2D
    {
        std::vector<std::array<double, 3>> world_points;
        std::vector<std::array<double, 2>> points;
        double depth = 0.0;
        int width = 1;
        Color_t color = kBlack;
        int pdg = 0;
        int style = 1;
        double alpha = 1.0;
    };

    struct LambdaDecayInfo
    {
        bool has_lambda = false;
        int pdg = 0;
        int track_id = -1;
        bool decay_inside = false;
        bool charged_p_pi_mode = false;
        bool charged_daughter_end_inside = true;
        double start_x = 0.0;
        double start_y = 0.0;
        double start_z = 0.0;
        double decay_x = 0.0;
        double decay_y = 0.0;
        double decay_z = 0.0;
        std::vector<int> daughter_pdgs;
    };

    struct ZoomView
    {
        double left = 1030.0;
        double bottom = 100.0;
        double width = 470.0;
        double height = 285.0;
        double center_x = 0.0;
        double center_y = 0.0;
        double center_z = 0.0;
        double scale = 2.2;
    };

    std::string Lower(std::string value);

    bool IsInvisiblePdg(int pdg)
    {
        const int apdg = std::abs(pdg);
        return pdg == 0 || apdg == 12 || apdg == 14 || apdg == 16;
    }

    bool IsNeutralPdg(int pdg)
    {
        const int apdg = std::abs(pdg);
        return pdg == 22 || pdg == 111 || pdg == 130 || pdg == 310 ||
               apdg == 2112 || apdg == 3122;
    }

    double SegmentLength(const StepSegment& segment)
    {
        const double dx = segment.x2 - segment.x1;
        const double dy = segment.y2 - segment.y1;
        const double dz = segment.z2 - segment.z1;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    Color_t TrackColor(int pdg)
    {
        switch (pdg)
        {
            case 2212:
                return TColor::GetColor(255, 72, 0);
            case 211:
                return TColor::GetColor(0, 235, 95);
            case -211:
                return TColor::GetColor(255, 0, 125);
            case 321:
                return TColor::GetColor(0, 205, 255);
            case -321:
                return TColor::GetColor(255, 205, 0);
            case 130:
                return TColor::GetColor(255, 172, 35);
            case 13:
                return TColor::GetColor(0, 135, 255);
            case -13:
                return TColor::GetColor(125, 85, 255);
            case 11:
                return TColor::GetColor(205, 55, 255);
            case -11:
                return TColor::GetColor(255, 120, 0);
            case 111:
                return TColor::GetColor(160, 210, 255);
            case 22:
                return TColor::GetColor(190, 198, 205);
            case 2112:
                return TColor::GetColor(185, 196, 205);
            default:
                break;
        }

        if (std::abs(pdg) >= 1000000000)
        {
            return TColor::GetColor(0, 225, 185);
        }
        return IsNeutralPdg(pdg) ? TColor::GetColor(190, 198, 205) : TColor::GetColor(255, 45, 45);
    }

    Color_t NeutrinoLineColor()
    {
        return TColor::GetColor(205, 210, 214);
    }

    Color_t ActiveBoxColor()
    {
        return TColor::GetColor(190, 90, 65);
    }

    Color_t CryostatColor()
    {
        return TColor::GetColor(150, 155, 160);
    }

    Color_t ContainedColor()
    {
        return TColor::GetColor(0, 170, 95);
    }

    Color_t EscapingColor()
    {
        return TColor::GetColor(235, 35, 35);
    }

    constexpr double kNeutrinoDashCm = 2.4;
    constexpr double kNeutrinoGapCm = 1.8;
    constexpr double kNeutralDashCm = 3.0;
    constexpr double kNeutralGapCm = 2.0;
    constexpr double kPi = 3.14159265358979323846;

    int TrackLineStyle(int pdg)
    {
        return IsNeutralPdg(pdg) ? 2 : 1;
    }

    double TrackAlpha(int pdg)
    {
        return IsNeutralPdg(pdg) ? 0.34 : 1.0;
    }

    int TrackWidth(const TrackPath& track)
    {
        return track.parent == 0 ? 4 : 2;
    }

    std::string TrackLabel(int pdg)
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
            case 3122:
                return "#Lambda";
            case -3122:
                return "#bar{#Lambda}";
            default:
                break;
        }

        if (std::abs(pdg) >= 1000000000)
        {
            return "ion";
        }

        std::ostringstream label;
        label << "PDG " << pdg;
        return label.str();
    }

    std::vector<std::string> SplitTokens(const char* value_list)
    {
        std::vector<std::string> values;
        std::stringstream input(value_list == nullptr ? "" : value_list);
        std::string item;

        while (std::getline(input, item, ','))
        {
            item.erase(std::remove_if(item.begin(),
                                      item.end(),
                                      [](unsigned char c) { return std::isspace(c); }),
                       item.end());
            if (!item.empty())
            {
                values.push_back(item);
            }
        }

        return values;
    }

    std::vector<std::string> SplitFormats(const char* format_list)
    {
        std::vector<std::string> formats = SplitTokens(format_list);

        if (formats.empty())
        {
            formats.push_back("png");
        }
        return formats;
    }

    std::set<int> ParsePdgFilter(const char* pdg_list)
    {
        std::set<int> filter;
        for (const auto& token : SplitTokens(pdg_list))
        {
            const std::string lower = Lower(token);
            if (lower == "all" || lower == "auto" || lower == "*")
            {
                continue;
            }

            std::stringstream parser(token);
            int pdg = 0;
            if (parser >> pdg)
            {
                filter.insert(pdg);
            }
            else
            {
                std::cerr << "Ignoring unrecognized PDG filter token: " << token << std::endl;
            }
        }
        return filter;
    }

    void AddRenderMode(std::vector<RenderMode>& modes, const RenderMode& mode)
    {
        for (const auto& existing : modes)
        {
            if (existing.name == mode.name)
            {
                return;
            }
        }
        modes.push_back(mode);
    }

    std::vector<RenderMode> ParseRenderModes(const char* render_mode_list)
    {
        std::vector<RenderMode> modes;
        for (const auto& token : SplitTokens(render_mode_list))
        {
            const std::string lower = Lower(token);
            if (lower == "both" || lower == "all")
            {
                AddRenderMode(modes, {"clean", false});
                AddRenderMode(modes, {"debug", true});
            }
            else if (lower == "debug")
            {
                AddRenderMode(modes, {"debug", true});
            }
            else if (lower == "clean")
            {
                AddRenderMode(modes, {"clean", false});
            }
            else
            {
                std::cerr << "Ignoring unrecognized render mode: " << token << std::endl;
            }
        }

        if (modes.empty())
        {
            AddRenderMode(modes, {"clean", false});
        }
        return modes;
    }

    std::string Lower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    bool Has(const std::string& value, const std::string& token)
    {
        return value.find(token) != std::string::npos;
    }

    int DisplayVolumeRank(const std::string& lower_name)
    {
        if (const char* requested = gSystem->Getenv("G4LARBOX_DISPLAY_VOLUME"))
        {
            if (lower_name == Lower(requested))
            {
                return 1000;
            }
        }

        if (lower_name == "volenclosuretpc")
        {
            return 900;
        }
        if (Has(lower_name, "enclosure") && Has(lower_name, "tpc"))
        {
            return 850;
        }
        if (lower_name == "voltpcactive")
        {
            return 800;
        }
        if (Has(lower_name, "tpcactive"))
        {
            return 750;
        }
        if (Has(lower_name, "active"))
        {
            return 600;
        }
        if (lower_name == "voltpc" || Has(lower_name, "tpc"))
        {
            return 450;
        }
        if (Has(lower_name, "lar"))
        {
            return 250;
        }
        return 0;
    }

    void StyleGeometry()
    {
        if (!gGeoManager)
        {
            return;
        }

        gGeoManager->DefaultColors();
        gGeoManager->SetTopVisible(kFALSE);
        gGeoManager->SetNsegments(96);
        gGeoManager->SetMaxVisNodes(100000);

        auto* volumes = gGeoManager->GetListOfVolumes();
        const int count = volumes == nullptr ? 0 : volumes->GetEntriesFast();
        for (int i = 0; i < count; ++i)
        {
            auto* volume = dynamic_cast<TGeoVolume*>(volumes->At(i));
            if (volume == nullptr)
            {
                continue;
            }

            const std::string name = Lower(volume->GetName());
            if (Has(name, "world"))
            {
                volume->SetVisibility(kFALSE);
                volume->SetTransparency(100);
                continue;
            }

            if (DisplayVolumeRank(name) >= 450)
            {
                volume->SetVisibility(kTRUE);
                volume->SetLineColor(ActiveBoxColor());
                volume->SetFillColor(kWhite);
                volume->SetTransparency(100);
                volume->SetLineWidth(1);
                continue;
            }

            volume->SetVisibility(kTRUE);
            volume->SetLineColor(kGray + 1);
            volume->SetFillColor(kGray + 1);
            volume->SetTransparency(90);
        }
    }

    TGeoVolume* FindActiveVolume()
    {
        if (gGeoManager == nullptr)
        {
            return nullptr;
        }

        auto* volumes = gGeoManager->GetListOfVolumes();
        const int count = volumes == nullptr ? 0 : volumes->GetEntriesFast();
        TGeoVolume* best_volume = nullptr;
        int best_rank = 0;
        for (int i = 0; i < count; ++i)
        {
            auto* volume = dynamic_cast<TGeoVolume*>(volumes->At(i));
            if (volume == nullptr)
            {
                continue;
            }

            const std::string name = Lower(volume->GetName());
            const int rank = DisplayVolumeRank(name);
            if (rank > best_rank)
            {
                best_rank = rank;
                best_volume = volume;
            }
        }
        return best_volume;
    }

    BoxGeometry LoadBoxGeometry()
    {
        BoxGeometry box;
        auto* active = FindActiveVolume();
        if (active == nullptr)
        {
            return box;
        }

        if (auto* bbox = dynamic_cast<TGeoBBox*>(active->GetShape()))
        {
            bbox->ComputeBBox();
            box.dx = bbox->GetDX();
            box.dy = bbox->GetDY();
            box.dz = bbox->GetDZ();
        }

        if (gGeoManager != nullptr && gGeoManager->GetTopVolume() != nullptr)
        {
            TGeoIterator next(gGeoManager->GetTopVolume());
            TGeoNode* node = nullptr;
            while ((node = next()))
            {
                if (node->GetVolume() != active)
                {
                    continue;
                }

                const Double_t* translation = next.GetCurrentMatrix()->GetTranslation();
                box.x = translation[0];
                box.y = translation[1];
                box.z = translation[2];
                break;
            }
        }

        return box;
    }

    double ToCm(double value_mm)
    {
        return 0.1 * value_mm;
    }

    template <typename T>
    bool HasIndex(const std::vector<T>* values, std::size_t index)
    {
        return values != nullptr && index < values->size();
    }

    double ValueOrNaN(const std::vector<double>* values, std::size_t index)
    {
        if (!HasIndex(values, index))
        {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return (*values)[index];
    }

    bool IsFinite(double value)
    {
        return std::isfinite(value);
    }

    bool InsideActive(const BoxGeometry& box, double x, double y, double z)
    {
        constexpr double tolerance = 1.0e-5;
        return std::abs(x - box.x) <= box.dx + tolerance &&
               std::abs(y - box.y) <= box.dy + tolerance &&
               std::abs(z - box.z) <= box.dz + tolerance;
    }

    bool IncomingNeutrinoSegment(const BoxGeometry& box,
                                 const TruthInfo& truth,
                                 std::array<double, 3>& start,
                                 std::array<double, 3>& end)
    {
        if (!truth.has_neutrino || !truth.has_incident_direction)
        {
            return false;
        }

        const double norm = std::sqrt(truth.incident_dir_x * truth.incident_dir_x +
                                      truth.incident_dir_y * truth.incident_dir_y +
                                      truth.incident_dir_z * truth.incident_dir_z);
        if (norm <= 0.0)
        {
            return false;
        }

        const std::array<double, 3> vertex = {truth.vertex_x, truth.vertex_y, truth.vertex_z};
        const std::array<double, 3> direction = {
            truth.incident_dir_x / norm,
            truth.incident_dir_y / norm,
            truth.incident_dir_z / norm
        };
        const std::array<double, 3> center = {box.x, box.y, box.z};
        const std::array<double, 3> half = {box.dx, box.dy, box.dz};

        double distance = std::numeric_limits<double>::infinity();
        constexpr double epsilon = 1.0e-9;
        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::abs(direction[axis]) <= epsilon)
            {
                continue;
            }

            const double boundary = direction[axis] > 0.0
                                        ? center[axis] - half[axis]
                                        : center[axis] + half[axis];
            const double candidate = (vertex[axis] - boundary) / direction[axis];
            if (candidate > 0.0)
            {
                distance = std::min(distance, candidate);
            }
        }

        if (!std::isfinite(distance))
        {
            distance = std::max({box.dx, box.dy, box.dz});
        }

        end = vertex;
        start = {
            vertex[0] - direction[0] * distance,
            vertex[1] - direction[1] * distance,
            vertex[2] - direction[2] * distance
        };
        return true;
    }

    std::array<double, 3> Interpolate(const std::array<double, 3>& start,
                                      const std::array<double, 3>& end,
                                      double t)
    {
        return {start[0] + t * (end[0] - start[0]),
                start[1] + t * (end[1] - start[1]),
                start[2] + t * (end[2] - start[2])};
    }

    std::array<double, 3> ActiveExitPoint(const BoxGeometry& box,
                                          const std::array<double, 3>& start,
                                          const std::array<double, 3>& end)
    {
        double exit_t = 1.0;
        const std::array<double, 3> center = {box.x, box.y, box.z};
        const std::array<double, 3> half = {box.dx, box.dy, box.dz};

        for (int axis = 0; axis < 3; ++axis)
        {
            const double delta = end[axis] - start[axis];
            if (std::abs(delta) < 1.0e-9)
            {
                continue;
            }

            const double boundary = center[axis] + (delta > 0.0 ? half[axis] : -half[axis]);
            const double t = (boundary - start[axis]) / delta;
            if (t < 0.0 || t > exit_t)
            {
                continue;
            }

            const auto candidate = Interpolate(start, end, t);
            if (InsideActive(box, candidate[0], candidate[1], candidate[2]))
            {
                exit_t = t;
            }
        }

        return Interpolate(start, end, exit_t);
    }

    bool LoadTrackPaths(TTree& step_tree,
                        int event_index,
                        std::vector<TrackPath>& tracks,
                        int max_segments,
                        const DisplayOptions& options)
    {
        if (event_index < 0 || event_index >= step_tree.GetEntries())
        {
            std::cerr << "Requested event " << event_index
                      << " but stepTree contains " << step_tree.GetEntries()
                      << " events." << std::endl;
            return false;
        }

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

        const std::size_t count = std::min({xs->size(),
                                            ys->size(),
                                            zs->size(),
                                            xe->size(),
                                            ye->size(),
                                            ze->size(),
                                            parid->size(),
                                            trkid->size(),
                                            step_pdg->size()});
        const std::size_t stride = max_segments > 0 && count > static_cast<std::size_t>(max_segments)
                                       ? 1 + count / static_cast<std::size_t>(max_segments)
                                       : 1;
        std::map<int, TrackPath> track_map;
        std::size_t next_order = 0;

        for (std::size_t i = 0; i < count; i += stride)
        {
            StepSegment segment;
            segment.pdg = (*step_pdg)[i];
            segment.parent = (*parid)[i];
            segment.track = (*trkid)[i];
            segment.x1 = 0.1 * (*xs)[i];
            segment.y1 = 0.1 * (*ys)[i];
            segment.z1 = 0.1 * (*zs)[i];
            segment.x2 = 0.1 * (*xe)[i];
            segment.y2 = 0.1 * (*ye)[i];
            segment.z2 = 0.1 * (*ze)[i];

            const double length = SegmentLength(segment);
            if (IsInvisiblePdg(segment.pdg) || length < options.min_step_cm)
            {
                continue;
            }

            if (options.primary_only && segment.parent != 0)
            {
                continue;
            }

            if (!options.pdg_filter.empty() && options.pdg_filter.count(segment.pdg) == 0)
            {
                continue;
            }

            TrackPath& path = track_map[segment.track];
            if (path.points.empty())
            {
                path.pdg = segment.pdg;
                path.parent = segment.parent;
                path.track = segment.track;
                path.order = next_order++;
                path.points.push_back({segment.x1, segment.y1, segment.z1});
            }

            path.length += length;
            path.points.push_back({segment.x2, segment.y2, segment.z2});
        }

        tracks.clear();
        tracks.reserve(track_map.size());
        for (auto& entry : track_map)
        {
            if (entry.second.points.size() > 1)
            {
                tracks.push_back(entry.second);
            }
        }

        std::sort(tracks.begin(), tracks.end(), [](const TrackPath& lhs, const TrackPath& rhs) {
            const bool lhs_primary = lhs.parent == 0;
            const bool rhs_primary = rhs.parent == 0;
            if (lhs_primary != rhs_primary)
            {
                return lhs_primary;
            }
            if (lhs.length != rhs.length)
            {
                return lhs.length > rhs.length;
            }
            return lhs.order < rhs.order;
        });

        if (options.max_tracks > 0 && tracks.size() > static_cast<std::size_t>(options.max_tracks))
        {
            tracks.resize(static_cast<std::size_t>(options.max_tracks));
        }

        std::sort(tracks.begin(), tracks.end(), [](const TrackPath& lhs, const TrackPath& rhs) {
            return lhs.order < rhs.order;
        });

        return true;
    }

    std::vector<LambdaDecayInfo> LoadLambdaDecays(TTree* track_tree,
                                                  int event_index,
                                                  const BoxGeometry& box)
    {
        std::vector<LambdaDecayInfo> lambdas;
        if (track_tree == nullptr || event_index < 0 || event_index >= track_tree->GetEntries())
        {
            return lambdas;
        }

        std::vector<int>* pdg = nullptr;
        std::vector<int>* curid = nullptr;
        std::vector<int>* preid = nullptr;
        std::vector<double>* xv = nullptr;
        std::vector<double>* yv = nullptr;
        std::vector<double>* zv = nullptr;
        std::vector<double>* xf = nullptr;
        std::vector<double>* yf = nullptr;
        std::vector<double>* zf = nullptr;
        std::vector<double>* xi = nullptr;
        std::vector<double>* yi = nullptr;
        std::vector<double>* zi = nullptr;

        if (track_tree->GetBranch("pdg") == nullptr ||
            track_tree->GetBranch("curid") == nullptr ||
            track_tree->GetBranch("preid") == nullptr ||
            track_tree->GetBranch("xi") == nullptr ||
            track_tree->GetBranch("yi") == nullptr ||
            track_tree->GetBranch("zi") == nullptr)
        {
            return lambdas;
        }

        track_tree->SetBranchAddress("pdg", &pdg);
        track_tree->SetBranchAddress("curid", &curid);
        track_tree->SetBranchAddress("preid", &preid);
        if (track_tree->GetBranch("xv") != nullptr)
        {
            track_tree->SetBranchAddress("xv", &xv);
            track_tree->SetBranchAddress("yv", &yv);
            track_tree->SetBranchAddress("zv", &zv);
        }
        if (track_tree->GetBranch("xf") != nullptr)
        {
            track_tree->SetBranchAddress("xf", &xf);
            track_tree->SetBranchAddress("yf", &yf);
            track_tree->SetBranchAddress("zf", &zf);
        }
        track_tree->SetBranchAddress("xi", &xi);
        track_tree->SetBranchAddress("yi", &yi);
        track_tree->SetBranchAddress("zi", &zi);

        track_tree->GetEntry(event_index);
        if (pdg == nullptr || curid == nullptr || preid == nullptr)
        {
            return lambdas;
        }

        const std::size_t count = std::min({pdg->size(), curid->size(), preid->size()});
        for (std::size_t i = 0; i < count; ++i)
        {
            if ((*pdg)[i] != 3122 && (*pdg)[i] != -3122)
            {
                continue;
            }

            LambdaDecayInfo info;
            info.has_lambda = true;
            info.pdg = (*pdg)[i];
            info.track_id = (*curid)[i];
            info.start_x = ToCm(ValueOrNaN(xv, i));
            info.start_y = ToCm(ValueOrNaN(yv, i));
            info.start_z = ToCm(ValueOrNaN(zv, i));
            info.decay_x = ToCm(ValueOrNaN(xf != nullptr ? xf : xi, i));
            info.decay_y = ToCm(ValueOrNaN(yf != nullptr ? yf : yi, i));
            info.decay_z = ToCm(ValueOrNaN(zf != nullptr ? zf : zi, i));

            if (!IsFinite(info.start_x) || !IsFinite(info.start_y) || !IsFinite(info.start_z))
            {
                info.start_x = info.decay_x;
                info.start_y = info.decay_y;
                info.start_z = info.decay_z;
            }

            info.decay_inside = InsideActive(box, info.decay_x, info.decay_y, info.decay_z);

            const bool anti_lambda = info.pdg == -3122;
            const int proton_pdg = anti_lambda ? -2212 : 2212;
            const int pion_pdg = anti_lambda ? 211 : -211;
            bool has_proton = false;
            bool has_pion = false;

            for (std::size_t j = 0; j < count; ++j)
            {
                if ((*preid)[j] != info.track_id)
                {
                    continue;
                }

                info.daughter_pdgs.push_back((*pdg)[j]);
                if ((*pdg)[j] == proton_pdg)
                {
                    has_proton = true;
                }
                if ((*pdg)[j] == pion_pdg)
                {
                    has_pion = true;
                }

                if ((*pdg)[j] == proton_pdg || (*pdg)[j] == pion_pdg)
                {
                    const double end_x = ToCm(ValueOrNaN(xf != nullptr ? xf : xi, j));
                    const double end_y = ToCm(ValueOrNaN(yf != nullptr ? yf : yi, j));
                    const double end_z = ToCm(ValueOrNaN(zf != nullptr ? zf : zi, j));
                    info.charged_daughter_end_inside =
                        info.charged_daughter_end_inside && InsideActive(box, end_x, end_y, end_z);
                }
            }

            info.charged_p_pi_mode = has_proton && has_pion;
            lambdas.push_back(info);
        }

        return lambdas;
    }

    TruthInfo LoadTruth(TTree* truth_tree, int event_index)
    {
        TruthInfo truth;
        if (truth_tree == nullptr || event_index < 0 || event_index >= truth_tree->GetEntries())
        {
            return truth;
        }

        std::string* source = nullptr;
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
        if (truth_tree->GetBranch("has_incident_direction"))
        {
            truth_tree->SetBranchAddress("has_incident_direction", &truth.has_incident_direction);
        }
        if (truth_tree->GetBranch("incident_dir_x"))
        {
            truth_tree->SetBranchAddress("incident_dir_x", &truth.incident_dir_x);
        }
        if (truth_tree->GetBranch("incident_dir_y"))
        {
            truth_tree->SetBranchAddress("incident_dir_y", &truth.incident_dir_y);
        }
        if (truth_tree->GetBranch("incident_dir_z"))
        {
            truth_tree->SetBranchAddress("incident_dir_z", &truth.incident_dir_z);
        }

        truth_tree->GetEntry(event_index);
        truth.has_truth = true;
        if (source != nullptr)
        {
            truth.source = *source;
        }
        truth.vertex_x *= 0.1;
        truth.vertex_y *= 0.1;
        truth.vertex_z *= 0.1;

        const int abs_neu = std::abs(truth.genie_neu);
        const std::string source_lower = Lower(truth.source);
        truth.has_neutrino = abs_neu == 12 || abs_neu == 14 || abs_neu == 16 ||
                             source_lower.find("marley") != std::string::npos;
        if (!truth.has_incident_direction &&
            source_lower.find("marley") == std::string::npos &&
            (abs_neu == 12 || abs_neu == 14 || abs_neu == 16))
        {
            truth.has_incident_direction = true;
            truth.incident_dir_x = 0.0;
            truth.incident_dir_y = 0.0;
            truth.incident_dir_z = 1.0;
        }
        return truth;
    }

    void DrawNeutrinoFlight(const BoxGeometry& box, const TruthInfo& truth)
    {
        std::array<double, 3> start;
        std::array<double, 3> end;
        if (!IncomingNeutrinoSegment(box, truth, start, end))
        {
            return;
        }

        const double dx = end[0] - start[0];
        const double dy = end[1] - start[1];
        const double dz = end[2] - start[2];
        const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (length <= 0.0)
        {
            return;
        }

        for (double offset = 0.0; offset < length; offset += kNeutrinoDashCm + kNeutrinoGapCm)
        {
            const double t1 = offset / length;
            const double t2 = std::min(offset + kNeutrinoDashCm, length) / length;

            auto* line = new TPolyLine3D(2);
            line->SetPoint(0,
                           start[0] + dx * t1,
                           start[1] + dy * t1,
                           start[2] + dz * t1);
            line->SetPoint(1,
                           start[0] + dx * t2,
                           start[1] + dy * t2,
                           start[2] + dz * t2);
            line->SetLineColorAlpha(NeutrinoLineColor(), 0.72);
            line->SetLineWidth(2);
            line->Draw("same");
        }
    }

    void DrawLine3D(const std::array<double, 3>& start,
                    const std::array<double, 3>& end,
                    Color_t color,
                    int width,
                    double alpha = 1.0)
    {
        auto* line = new TPolyLine3D(2);
        line->SetPoint(0, start[0], start[1], start[2]);
        line->SetPoint(1, end[0], end[1], end[2]);
        line->SetLineColorAlpha(color, alpha);
        line->SetLineWidth(width);
        line->Draw("same");
    }

    void DrawDashedLine3D(const std::array<double, 3>& start,
                          const std::array<double, 3>& end,
                          Color_t color,
                          int width,
                          double alpha = 1.0,
                          double dash_cm = 9.0,
                          double gap_cm = 6.0)
    {
        const double dx = end[0] - start[0];
        const double dy = end[1] - start[1];
        const double dz = end[2] - start[2];
        const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (length <= 0.0)
        {
            return;
        }

        for (double offset = 0.0; offset < length; offset += dash_cm + gap_cm)
        {
            const double t1 = offset / length;
            const double t2 = std::min(offset + dash_cm, length) / length;
            DrawLine3D(Interpolate(start, end, t1), Interpolate(start, end, t2), color, width, alpha);
        }
    }

    void DrawCryostatContext3D(const BoxGeometry& box)
    {
        constexpr int circle_points = 96;
        constexpr int longitudinal_lines = 16;
        const double z_min = box.z - box.dz - 70.0;
        const double z_max = box.z + box.dz + 70.0;
        const double rx = box.dx * 1.45;
        const double ry = box.dy * 1.65;

        for (double z : {z_min, z_max})
        {
            auto* cap = new TPolyLine3D(circle_points + 1);
            for (int i = 0; i <= circle_points; ++i)
            {
                const double theta = 2.0 * kPi * static_cast<double>(i) /
                                     static_cast<double>(circle_points);
                cap->SetPoint(i,
                              box.x + rx * std::cos(theta),
                              box.y + ry * std::sin(theta),
                              z);
            }
            cap->SetLineColor(CryostatColor());
            cap->SetLineWidth(1);
            cap->Draw("same");
        }

        for (int i = 0; i < longitudinal_lines; ++i)
        {
            const double theta = 2.0 * kPi * static_cast<double>(i) /
                                 static_cast<double>(longitudinal_lines);
            DrawLine3D({box.x + rx * std::cos(theta),
                        box.y + ry * std::sin(theta),
                        z_min},
                       {box.x + rx * std::cos(theta),
                        box.y + ry * std::sin(theta),
                        z_max},
                       CryostatColor(),
                       1);
        }
    }

    void DrawLambdaOverlays3D(const BoxGeometry& box,
                              const std::vector<LambdaDecayInfo>& lambdas)
    {
        for (const auto& lambda : lambdas)
        {
            if (!lambda.has_lambda)
            {
                continue;
            }

            const std::array<double, 3> start = {lambda.start_x, lambda.start_y, lambda.start_z};
            const std::array<double, 3> decay = {lambda.decay_x, lambda.decay_y, lambda.decay_z};
            if (lambda.decay_inside)
            {
                DrawDashedLine3D(start,
                                 decay,
                                 TrackColor(lambda.pdg),
                                 3,
                                 TrackAlpha(lambda.pdg),
                                 kNeutralDashCm,
                                 kNeutralGapCm);
            }
            else
            {
                DrawDashedLine3D(start,
                                 decay,
                                 TrackColor(lambda.pdg),
                                 3,
                                 TrackAlpha(lambda.pdg),
                                 kNeutralDashCm,
                                 kNeutralGapCm);
            }
        }
    }

    void DrawTracks(const std::vector<TrackPath>& tracks, const TruthInfo& truth, bool debug)
    {
        for (const auto& track : tracks)
        {
            if (IsNeutralPdg(track.pdg))
            {
                for (std::size_t i = 1; i < track.points.size(); ++i)
                {
                    DrawDashedLine3D(track.points[i - 1],
                                     track.points[i],
                                     TrackColor(track.pdg),
                                     TrackWidth(track),
                                     TrackAlpha(track.pdg),
                                     kNeutralDashCm,
                                     kNeutralGapCm);
                }
                continue;
            }

            auto* line = new TPolyLine3D(static_cast<int>(track.points.size()));
            for (std::size_t i = 0; i < track.points.size(); ++i)
            {
                line->SetPoint(static_cast<int>(i),
                               track.points[i][0],
                               track.points[i][1],
                               track.points[i][2]);
            }
            line->SetLineColorAlpha(TrackColor(track.pdg), TrackAlpha(track.pdg));
            line->SetLineWidth(TrackWidth(track));
            line->SetLineStyle(TrackLineStyle(track.pdg));
            line->Draw("same");
        }

        if (debug && truth.has_truth)
        {
            auto* vertex = new TPolyMarker3D(1);
            vertex->SetPoint(0, truth.vertex_x, truth.vertex_y, truth.vertex_z);
            vertex->SetMarkerStyle(20);
            vertex->SetMarkerSize(1.4);
            vertex->SetMarkerColor(kBlack);
            vertex->Draw("same");
        }
    }

    TGLViewer* ConfigureViewer(double fov,
                               double dolly,
                               double hrot,
                               double vrot)
    {
        auto* viewer = dynamic_cast<TGLViewer*>(gPad->GetViewer3D());
        if (viewer == nullptr)
        {
            return nullptr;
        }

        viewer->UseLightColorSet();
        viewer->SetStyle(TGLRnrCtx::kFill);
        viewer->SetSmoothLines(kTRUE);
        viewer->SetSmoothPoints(kTRUE);
        viewer->SetLineScale(1.25f);

        Double_t ref[3] = {0.0, 0.0, 0.0};
        viewer->SetGuideState(TGLUtil::kAxesNone, kFALSE, kFALSE, ref);

        auto* lights = viewer->GetLightSet();
        lights->SetLight(TGLLightSet::kLightRight, kFALSE);
        lights->SetLight(TGLLightSet::kLightBottom, kFALSE);

        const auto camera = TGLViewer::kCameraPerspXOZ;
        viewer->SetCurrentCamera(camera);
        viewer->CurrentCamera().SetExternalCenter(kTRUE);

        Double_t center[3] = {0.0, 0.0, 0.0};
        viewer->SetPerspectiveCamera(camera, fov, dolly, center, hrot, vrot);

        gPad->Modified();
        gPad->Update();
        gSystem->ProcessEvents();
        return viewer;
    }

    void EnsureOutputDirectory(const std::string& output)
    {
        const char* dir = gSystem->DirName(output.c_str());
        if (dir != nullptr && std::string(dir) != ".")
        {
            gSystem->mkdir(dir, kTRUE);
        }
    }

    void SaveViewer(TGLViewer& viewer,
                    const std::string& output_prefix,
                    const std::vector<std::string>& formats,
                    int width,
                    int height)
    {
        for (const auto& format : formats)
        {
            const std::string output = output_prefix + "." + format;
            EnsureOutputDirectory(output);
            gSystem->Unlink(output.c_str());

            bool wrote = false;
            if (format == "png")
            {
                wrote = viewer.SavePictureUsingFBO(output.c_str(), width, height, 0.0f);
                if (!wrote)
                {
                    viewer.SavePictureUsingBB(output.c_str());
                    wrote = !gSystem->AccessPathName(output.c_str());
                }
            }
            else
            {
                viewer.SavePicture(output.c_str());
                wrote = !gSystem->AccessPathName(output.c_str());
            }

            if (wrote)
            {
                std::cout << "Saved " << output << std::endl;
            }
            else
            {
                std::cerr << "ROOT did not create " << output
                          << "; this ROOT build may not support that graphics backend." << std::endl;
            }
        }
    }

    void ProjectFallback(const BoxGeometry& box,
                         double x,
                         double y,
                         double z,
                         double& px,
                         double& py,
                         double& depth)
    {
        constexpr double depth_skew = 0.22;
        constexpr double canvas_width = 1600.0;
        constexpr double canvas_height = 900.0;

        auto axis_index = [](char axis) {
            switch (axis)
            {
                case 'x': return 0;
                case 'y': return 1;
                case 'z': return 2;
                default: return -1;
            }
        };

        int horizontal_axis = 2;
        int vertical_axis = 1;
        if (const char* view = gSystem->Getenv("G4LARBOX_DISPLAY_VIEW"))
        {
            const std::string lower_view = Lower(view);
            if (lower_view.size() >= 2)
            {
                const int requested_horizontal = axis_index(lower_view[0]);
                const int requested_vertical = axis_index(lower_view[1]);
                if (requested_horizontal >= 0 &&
                    requested_vertical >= 0 &&
                    requested_horizontal != requested_vertical)
                {
                    horizontal_axis = requested_horizontal;
                    vertical_axis = requested_vertical;
                }
            }
        }

        int depth_axis = 0;
        for (int axis = 0; axis < 3; ++axis)
        {
            if (axis != horizontal_axis && axis != vertical_axis)
            {
                depth_axis = axis;
                break;
            }
        }

        const std::array<double, 3> value = {x, y, z};
        const std::array<double, 3> center = {box.x, box.y, box.z};
        const std::array<double, 3> half = {box.dx, box.dy, box.dz};

        const double horizontal_min = center[horizontal_axis] - half[horizontal_axis];
        const double projected_width_cm = std::max(1.0,
                                                   2.0 * half[horizontal_axis] +
                                                   2.0 * half[depth_axis] * depth_skew);
        const double projected_height_cm = std::max(1.0,
                                                    2.0 * half[vertical_axis] +
                                                    2.0 * half[depth_axis] * depth_skew);
        const double cm_to_px = std::min({1.16,
                                          0.76 * canvas_width / projected_width_cm,
                                          0.62 * canvas_height / projected_height_cm});
        const double x_origin = 0.5 * (canvas_width - projected_width_cm * cm_to_px) +
                                half[depth_axis] * depth_skew * cm_to_px;
        const double y_origin = 0.54 * canvas_height;
        depth = (value[depth_axis] - center[depth_axis]) +
                0.05 * (value[horizontal_axis] - center[horizontal_axis]);
        px = x_origin +
             (value[horizontal_axis] - horizontal_min) * cm_to_px +
             (value[depth_axis] - center[depth_axis]) * cm_to_px * depth_skew;
        py = y_origin -
             (value[vertical_axis] - center[vertical_axis]) * cm_to_px -
             (value[depth_axis] - center[depth_axis]) * cm_to_px * depth_skew;
    }

    void DrawProjectedLine(const BoxGeometry& box,
                           double x1,
                           double y1,
                           double z1,
                           double x2,
                           double y2,
                           double z2,
                           Color_t color,
                           double alpha,
                           int width,
                           int style = 1)
    {
        double px1 = 0.0;
        double py1 = 0.0;
        double d1 = 0.0;
        double px2 = 0.0;
        double py2 = 0.0;
        double d2 = 0.0;
        ProjectFallback(box, x1, y1, z1, px1, py1, d1);
        ProjectFallback(box, x2, y2, z2, px2, py2, d2);

        auto* line = new TLine(px1, py1, px2, py2);
        line->SetLineColorAlpha(color, alpha);
        line->SetLineWidth(width);
        line->SetLineStyle(style);
        line->Draw();
    }

    void DrawProjectedPolyline(const BoxGeometry& box,
                               const std::vector<std::array<double, 3>>& points,
                               Color_t color,
                               double alpha,
                               int width)
    {
        auto* line = new TPolyLine(static_cast<int>(points.size()));
        for (std::size_t i = 0; i < points.size(); ++i)
        {
            double px = 0.0;
            double py = 0.0;
            double depth = 0.0;
            ProjectFallback(box, points[i][0], points[i][1], points[i][2], px, py, depth);
            line->SetPoint(static_cast<int>(i), px, py);
        }
        line->SetLineColorAlpha(color, alpha);
        line->SetLineWidth(width);
        line->Draw();
    }

    void DrawDashedProjectedLine(const BoxGeometry& box,
                                 const std::array<double, 3>& start,
                                 const std::array<double, 3>& end,
                                 Color_t color,
                                 double alpha,
                                 int width,
                                 double dash_cm = 9.0,
                                 double gap_cm = 6.0)
    {
        const double dx = end[0] - start[0];
        const double dy = end[1] - start[1];
        const double dz = end[2] - start[2];
        const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (length <= 0.0)
        {
            return;
        }

        for (double offset = 0.0; offset < length; offset += dash_cm + gap_cm)
        {
            const double t1 = offset / length;
            const double t2 = std::min(offset + dash_cm, length) / length;
            const auto p1 = Interpolate(start, end, t1);
            const auto p2 = Interpolate(start, end, t2);
            DrawProjectedLine(box,
                              p1[0],
                              p1[1],
                              p1[2],
                              p2[0],
                              p2[1],
                              p2[2],
                              color,
                              alpha,
                              width);
        }
    }

    void DrawFallbackCryostatContext(const BoxGeometry& box)
    {
        constexpr int circle_points = 96;
        constexpr int longitudinal_lines = 16;
        const double z_min = box.z - box.dz - 70.0;
        const double z_max = box.z + box.dz + 70.0;
        const double rx = box.dx * 1.45;
        const double ry = box.dy * 1.65;

        for (double z : {z_min, z_max})
        {
            std::vector<std::array<double, 3>> cap;
            cap.reserve(circle_points + 1);
            for (int i = 0; i <= circle_points; ++i)
            {
                const double theta = 2.0 * kPi * static_cast<double>(i) /
                                     static_cast<double>(circle_points);
                cap.push_back({box.x + rx * std::cos(theta),
                               box.y + ry * std::sin(theta),
                               z});
            }
            DrawProjectedPolyline(box, cap, CryostatColor(), 0.32, 1);
        }

        for (int i = 0; i < longitudinal_lines; ++i)
        {
            const double theta = 2.0 * kPi * static_cast<double>(i) /
                                 static_cast<double>(longitudinal_lines);
            DrawProjectedLine(box,
                              box.x + rx * std::cos(theta),
                              box.y + ry * std::sin(theta),
                              z_min,
                              box.x + rx * std::cos(theta),
                              box.y + ry * std::sin(theta),
                              z_max,
                              CryostatColor(),
                              0.22,
                              1);
        }
    }

    void DrawFallbackGdmlBox(const BoxGeometry& box)
    {
        const std::array<std::array<double, 3>, 8> corners = {{
            {box.x - box.dx, box.y - box.dy, box.z - box.dz},
            {box.x + box.dx, box.y - box.dy, box.z - box.dz},
            {box.x + box.dx, box.y + box.dy, box.z - box.dz},
            {box.x - box.dx, box.y + box.dy, box.z - box.dz},
            {box.x - box.dx, box.y - box.dy, box.z + box.dz},
            {box.x + box.dx, box.y - box.dy, box.z + box.dz},
            {box.x + box.dx, box.y + box.dy, box.z + box.dz},
            {box.x - box.dx, box.y + box.dy, box.z + box.dz}
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
            ProjectFallback(box,
                            corners[edge[0]][0],
                            corners[edge[0]][1],
                            corners[edge[0]][2],
                            x1,
                            y1,
                            d1);
            ProjectFallback(box,
                            corners[edge[1]][0],
                            corners[edge[1]][1],
                            corners[edge[1]][2],
                            x2,
                            y2,
                            d2);

            auto* line = new TLine(x1, y1, x2, y2);
            line->SetLineColorAlpha(ActiveBoxColor(), 0.68);
            line->SetLineWidth(1);
            line->Draw();
        }
    }

    void DrawFallbackTracks(const BoxGeometry& box, const std::vector<TrackPath>& tracks)
    {
        std::vector<DrawPath2D> paths;
        paths.reserve(tracks.size());
        for (const auto& track : tracks)
        {
            DrawPath2D path;
            path.points.reserve(track.points.size());
            path.world_points = track.points;
            path.width = TrackWidth(track);
            path.color = TrackColor(track.pdg);
            path.pdg = track.pdg;
            path.style = TrackLineStyle(track.pdg);
            path.alpha = TrackAlpha(track.pdg);

            double depth_sum = 0.0;
            for (const auto& point : track.points)
            {
                double px = 0.0;
                double py = 0.0;
                double depth = 0.0;
                ProjectFallback(box, point[0], point[1], point[2], px, py, depth);
                path.points.push_back({px, py});
                depth_sum += depth;
            }

            path.depth = path.points.empty() ? 0.0 : depth_sum / static_cast<double>(path.points.size());
            paths.push_back(path);
        }

        std::sort(paths.begin(), paths.end(), [](const DrawPath2D& lhs, const DrawPath2D& rhs) {
            return lhs.depth < rhs.depth;
        });

        for (const auto& path : paths)
        {
            if (path.points.size() < 2)
            {
                continue;
            }

            if (IsNeutralPdg(path.pdg))
            {
                for (std::size_t i = 1; i < path.world_points.size(); ++i)
                {
                    DrawDashedProjectedLine(box,
                                            path.world_points[i - 1],
                                            path.world_points[i],
                                            path.color,
                                            path.alpha,
                                            path.width,
                                            kNeutralDashCm,
                                            kNeutralGapCm);
                }
                continue;
            }

            auto* drawable = new TPolyLine(static_cast<int>(path.points.size()));
            for (std::size_t i = 0; i < path.points.size(); ++i)
            {
                drawable->SetPoint(static_cast<int>(i), path.points[i][0], path.points[i][1]);
            }
            drawable->SetLineColorAlpha(path.color, path.alpha);
            drawable->SetLineWidth(path.width);
            drawable->SetLineStyle(path.style);
            drawable->Draw();
        }
    }

    void DrawFallbackNeutrinoFlight(const BoxGeometry& box, const TruthInfo& truth)
    {
        std::array<double, 3> start;
        std::array<double, 3> end;
        if (!IncomingNeutrinoSegment(box, truth, start, end))
        {
            return;
        }

        const double dx = end[0] - start[0];
        const double dy = end[1] - start[1];
        const double dz = end[2] - start[2];
        const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (length <= 0.0)
        {
            return;
        }

        for (double offset = 0.0; offset < length; offset += kNeutrinoDashCm + kNeutrinoGapCm)
        {
            const double t1 = offset / length;
            const double t2 = std::min(offset + kNeutrinoDashCm, length) / length;
            DrawProjectedLine(box,
                              start[0] + dx * t1,
                              start[1] + dy * t1,
                              start[2] + dz * t1,
                              start[0] + dx * t2,
                              start[1] + dy * t2,
                              start[2] + dz * t2,
                              NeutrinoLineColor(),
                              0.72,
                              2);
        }
    }

    const LambdaDecayInfo* FirstLambda(const std::vector<LambdaDecayInfo>& lambdas)
    {
        for (const auto& lambda : lambdas)
        {
            if (lambda.has_lambda)
            {
                return &lambda;
            }
        }
        return nullptr;
    }

    void DrawFallbackLambdaOverlays(const BoxGeometry& box,
                                    const std::vector<LambdaDecayInfo>& lambdas)
    {
        for (const auto& lambda : lambdas)
        {
            if (!lambda.has_lambda)
            {
                continue;
            }

            const std::array<double, 3> start = {lambda.start_x, lambda.start_y, lambda.start_z};
            const std::array<double, 3> decay = {lambda.decay_x, lambda.decay_y, lambda.decay_z};
            if (lambda.decay_inside)
            {
                DrawDashedProjectedLine(box,
                                        start,
                                        decay,
                                        TrackColor(lambda.pdg),
                                        TrackAlpha(lambda.pdg),
                                        3,
                                        kNeutralDashCm,
                                        kNeutralGapCm);
            }
            else
            {
                DrawDashedProjectedLine(box,
                                        start,
                                        decay,
                                        TrackColor(lambda.pdg),
                                        TrackAlpha(lambda.pdg),
                                        3,
                                        kNeutralDashCm,
                                        kNeutralGapCm);
            }
        }
    }

    void DrawFallbackEventMarkers(const BoxGeometry& box,
                                  const TruthInfo& truth,
                                  const std::vector<LambdaDecayInfo>& lambdas)
    {
        (void)truth;
        DrawFallbackLambdaOverlays(box, lambdas);
    }

    ZoomView MakeZoomView(const BoxGeometry& box,
                          const TruthInfo& truth,
                          const std::vector<LambdaDecayInfo>& lambdas)
    {
        ZoomView view;
        const auto* lambda = FirstLambda(lambdas);
        if (lambda != nullptr)
        {
            view.center_x = 0.5 * (lambda->start_x + lambda->decay_x);
            view.center_y = 0.5 * (lambda->start_y + lambda->decay_y);
            view.center_z = 0.5 * (lambda->start_z + lambda->decay_z);
            view.scale = lambda->decay_inside ? 3.0 : 2.7;
        }
        else if (truth.has_truth)
        {
            view.center_x = truth.vertex_x;
            view.center_y = truth.vertex_y;
            view.center_z = truth.vertex_z;
        }
        else
        {
            view.center_x = box.x;
            view.center_y = box.y;
            view.center_z = box.z;
        }

        return view;
    }

    void ProjectZoom(const ZoomView& view,
                     double x,
                     double y,
                     double z,
                     double& px,
                     double& py)
    {
        px = view.left + 0.5 * view.width +
             (z - view.center_z) * view.scale +
             (x - view.center_x) * view.scale * 0.18;
        py = view.bottom + 0.5 * view.height -
             (y - view.center_y) * view.scale -
             (x - view.center_x) * view.scale * 0.22 -
             (z - view.center_z) * view.scale * 0.012;
    }

    bool InZoomPanel(const ZoomView& view, double px, double py, double pad = 12.0)
    {
        return px >= view.left - pad &&
               px <= view.left + view.width + pad &&
               py >= view.bottom - pad &&
               py <= view.bottom + view.height + pad;
    }

    void DrawZoomLine(const ZoomView& view,
                      const std::array<double, 3>& start,
                      const std::array<double, 3>& end,
                      Color_t color,
                      double alpha,
                      int width)
    {
        double x1 = 0.0;
        double y1 = 0.0;
        double x2 = 0.0;
        double y2 = 0.0;
        ProjectZoom(view, start[0], start[1], start[2], x1, y1);
        ProjectZoom(view, end[0], end[1], end[2], x2, y2);
        if (!InZoomPanel(view, x1, y1, 25.0) || !InZoomPanel(view, x2, y2, 25.0))
        {
            return;
        }

        auto* line = new TLine(x1, y1, x2, y2);
        line->SetLineColorAlpha(color, alpha);
        line->SetLineWidth(width);
        line->Draw();
    }

    void DrawZoomDashedLine(const ZoomView& view,
                            const std::array<double, 3>& start,
                            const std::array<double, 3>& end,
                            Color_t color,
                            double alpha,
                            int width,
                            double dash_cm = kNeutralDashCm,
                            double gap_cm = kNeutralGapCm)
    {
        const double dx = end[0] - start[0];
        const double dy = end[1] - start[1];
        const double dz = end[2] - start[2];
        const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (length <= 0.0)
        {
            return;
        }

        for (double offset = 0.0; offset < length; offset += dash_cm + gap_cm)
        {
            const double t1 = offset / length;
            const double t2 = std::min(offset + dash_cm, length) / length;
            DrawZoomLine(view, Interpolate(start, end, t1), Interpolate(start, end, t2), color, alpha, width);
        }
    }

    void DrawZoomActiveBoundaries(const ZoomView& view, const BoxGeometry& box)
    {
        const double z_min = box.z - box.dz;
        const double z_max = box.z + box.dz;
        const double x_min = box.x - box.dx;
        const double x_max = box.x + box.dx;
        const double y_min = box.y - box.dy;
        const double y_max = box.y + box.dy;

        for (double z_boundary : {z_min, z_max})
        {
            if (std::abs(view.center_z - z_boundary) < 140.0)
            {
                DrawZoomLine(view,
                             {view.center_x, view.center_y - 45.0, z_boundary},
                             {view.center_x, view.center_y + 45.0, z_boundary},
                             ActiveBoxColor(),
                             0.8,
                             2);
            }
        }

        for (double x_boundary : {x_min, x_max})
        {
            if (std::abs(view.center_x - x_boundary) < 100.0)
            {
                DrawZoomLine(view,
                             {x_boundary, view.center_y - 45.0, view.center_z - 55.0},
                             {x_boundary, view.center_y + 45.0, view.center_z + 55.0},
                             ActiveBoxColor(),
                             0.75,
                             2);
            }
        }

        for (double y_boundary : {y_min, y_max})
        {
            if (std::abs(view.center_y - y_boundary) < 100.0)
            {
                DrawZoomLine(view,
                             {view.center_x, y_boundary, view.center_z - 55.0},
                             {view.center_x, y_boundary, view.center_z + 55.0},
                             ActiveBoxColor(),
                             0.75,
                             2);
            }
        }
    }

    void DrawZoomTracks(const ZoomView& view, const std::vector<TrackPath>& tracks)
    {
        for (const auto& track : tracks)
        {
            if (track.points.size() < 2)
            {
                continue;
            }

            for (std::size_t i = 1; i < track.points.size(); ++i)
            {
                if (IsNeutralPdg(track.pdg))
                {
                    DrawZoomDashedLine(view,
                                       track.points[i - 1],
                                       track.points[i],
                                       TrackColor(track.pdg),
                                       TrackAlpha(track.pdg),
                                       std::max(2, TrackWidth(track) - 1),
                                       kNeutralDashCm,
                                       kNeutralGapCm);
                    continue;
                }

                DrawZoomLine(view,
                             track.points[i - 1],
                             track.points[i],
                             TrackColor(track.pdg),
                             TrackAlpha(track.pdg),
                             std::max(2, TrackWidth(track) - 1));
            }
        }
    }

    void DrawZoomLambdaOverlays(const ZoomView& view,
                                const BoxGeometry& box,
                                const std::vector<LambdaDecayInfo>& lambdas)
    {
        (void)box;
        for (const auto& lambda : lambdas)
        {
            if (!lambda.has_lambda)
            {
                continue;
            }

            const std::array<double, 3> start = {lambda.start_x, lambda.start_y, lambda.start_z};
            const std::array<double, 3> decay = {lambda.decay_x, lambda.decay_y, lambda.decay_z};
            if (lambda.decay_inside)
            {
                DrawZoomDashedLine(view,
                                   start,
                                   decay,
                                   TrackColor(lambda.pdg),
                                   TrackAlpha(lambda.pdg),
                                   3,
                                   kNeutralDashCm,
                                   kNeutralGapCm);
            }
            else
            {
                DrawZoomDashedLine(view,
                                   start,
                                   decay,
                                   TrackColor(lambda.pdg),
                                   TrackAlpha(lambda.pdg),
                                   3,
                                   kNeutralDashCm,
                                   kNeutralGapCm);
            }
        }
    }

    void DrawFallbackZoomInset(const BoxGeometry& box,
                               const TruthInfo& truth,
                               const std::vector<TrackPath>& tracks,
                               const std::vector<LambdaDecayInfo>& lambdas)
    {
        const auto* lambda = FirstLambda(lambdas);
        if (lambda == nullptr)
        {
            return;
        }

        const ZoomView view = MakeZoomView(box, truth, lambdas);
        auto* bg = new TBox(view.left, view.bottom, view.left + view.width, view.bottom + view.height);
        bg->SetFillColor(kWhite);
        bg->SetFillStyle(1001);
        bg->SetLineColor(kGray + 1);
        bg->SetLineWidth(1);
        bg->Draw();

        DrawZoomActiveBoundaries(view, box);
        DrawZoomTracks(view, tracks);
        DrawZoomLambdaOverlays(view, box, lambdas);

        if (truth.has_truth)
        {
            double px = 0.0;
            double py = 0.0;
            ProjectZoom(view, truth.vertex_x, truth.vertex_y, truth.vertex_z, px, py);
            if (InZoomPanel(view, px, py, 20.0))
            {
                auto* vertex = new TMarker(px, py, 20);
                vertex->SetMarkerColor(kBlack);
                vertex->SetMarkerSize(1.1);
                vertex->Draw();
            }
        }

        TLatex label;
        label.SetTextSize(0.022);
        label.SetTextColor(kGray + 2);
        label.DrawLatex(view.left + 13.0, view.bottom + view.height - 25.0, "zoom: #Lambda decay");
        label.SetTextColor(kGray + 2);
        label.DrawLatex(view.left + 13.0,
                        view.bottom + view.height - 51.0,
                        lambda->decay_inside ? "decay inside active" : "decay outside active");
        if (lambda->charged_p_pi_mode)
        {
            label.SetTextColor(kGray + 2);
            label.DrawLatex(view.left + 13.0,
                            view.bottom + view.height - 77.0,
                            lambda->charged_daughter_end_inside ? "p#pi daughters contained"
                                                                : "p#pi daughter exits active");
        }

        auto* frame = new TBox(view.left, view.bottom, view.left + view.width, view.bottom + view.height);
        frame->SetFillStyle(0);
        frame->SetLineColor(kGray + 1);
        frame->SetLineWidth(1);
        frame->Draw();
    }

    void DrawFallbackTruthVertex(const BoxGeometry& box, const TruthInfo& truth)
    {
        if (truth.has_truth)
        {
            double px = 0.0;
            double py = 0.0;
            double depth = 0.0;
            ProjectFallback(box, truth.vertex_x, truth.vertex_y, truth.vertex_z, px, py, depth);

            auto* h = new TLine(px - 9.0, py, px + 9.0, py);
            auto* v = new TLine(px, py - 9.0, px, py + 9.0);
            h->SetLineColor(kBlack);
            v->SetLineColor(kBlack);
            h->SetLineWidth(2);
            v->SetLineWidth(2);
            h->Draw();
            v->Draw();
        }
    }

    void DrawFallbackDebugOverlays(const BoxGeometry& box,
                                   const std::vector<TrackPath>& tracks,
                                   const TruthInfo& truth,
                                   int event_index)
    {
        DrawFallbackTruthVertex(box, truth);

        TLatex label;
        label.SetTextSize(0.026);
        label.SetTextColor(kGray + 2);
        if (truth.has_neutrino && truth.has_incident_direction)
        {
            const double norm = std::sqrt(truth.incident_dir_x * truth.incident_dir_x +
                                          truth.incident_dir_y * truth.incident_dir_y +
                                          truth.incident_dir_z * truth.incident_dir_z);
            constexpr double direction_tolerance = 1.0e-3;
            const bool legacy_positive_z_beam =
                norm > 0.0 &&
                std::abs(truth.incident_dir_x / norm) < direction_tolerance &&
                std::abs(truth.incident_dir_y / norm) < direction_tolerance &&
                truth.incident_dir_z / norm > 1.0 - direction_tolerance;

            if (legacy_positive_z_beam)
            {
                auto* beam = new TArrow(125.0, 82.0, 360.0, 82.0, 0.015, "|>");
                beam->SetLineColorAlpha(kBlack, 0.62);
                beam->SetFillColorAlpha(kBlack, 0.62);
                beam->SetLineWidth(2);
                beam->Draw();
                label.DrawLatex(132.0, 108.0, "+z beam");
            }
        }

        label.SetNDC();
        label.SetTextSize(0.028);
        std::ostringstream title;
        title << "event " << event_index << ", " << tracks.size() << " drawn tracks";
        label.DrawLatex(0.035, 0.935, title.str().c_str());

        auto* legend = new TLegend(0.76, 0.69, 0.97, 0.95);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextSize(0.024);
        if (truth.has_neutrino)
        {
            auto* sample = new TLine(0.0, 0.0, 1.0, 1.0);
            sample->SetLineColor(NeutrinoLineColor());
            sample->SetLineWidth(4);
            sample->SetLineStyle(2);
            legend->AddEntry(sample, "incoming #nu", "l");
        }
        std::set<int> seen_pdgs;
        for (const auto& track : tracks)
        {
            if (seen_pdgs.count(track.pdg) != 0)
            {
                continue;
            }

            auto* sample = new TLine(0.0, 0.0, 1.0, 1.0);
            sample->SetLineColorAlpha(TrackColor(track.pdg), TrackAlpha(track.pdg));
            sample->SetLineWidth(TrackWidth(track));
            sample->SetLineStyle(TrackLineStyle(track.pdg));
            legend->AddEntry(sample, TrackLabel(track.pdg).c_str(), "l");
            seen_pdgs.insert(track.pdg);

            if (seen_pdgs.size() >= 10)
            {
                break;
            }
        }
        legend->Draw();
    }

    std::string OutputPrefixForMode(const std::string& output_prefix,
                                    const RenderMode& mode,
                                    bool multiple_modes)
    {
        if (!multiple_modes)
        {
            return output_prefix;
        }
        return output_prefix + "_" + mode.name;
    }

    bool HasDebugMode(const std::vector<RenderMode>& modes)
    {
        return std::any_of(modes.begin(), modes.end(), [](const RenderMode& mode) {
            return mode.debug;
        });
    }

    BoxGeometry MakeLocalEventBox(const BoxGeometry& full_box,
                                  const std::vector<TrackPath>& tracks)
    {
        const char* local_env = gSystem->Getenv("G4LARBOX_DISPLAY_LOCAL_EVENT");
        if (local_env == nullptr || std::string(local_env) == "0")
        {
            return full_box;
        }

        double xmin = std::numeric_limits<double>::infinity();
        double xmax = -std::numeric_limits<double>::infinity();
        double ymin = std::numeric_limits<double>::infinity();
        double ymax = -std::numeric_limits<double>::infinity();
        double zmin = std::numeric_limits<double>::infinity();
        double zmax = -std::numeric_limits<double>::infinity();

        for (const auto& track : tracks)
        {
            for (const auto& point : track.points)
            {
                xmin = std::min(xmin, point[0]);
                xmax = std::max(xmax, point[0]);
                ymin = std::min(ymin, point[1]);
                ymax = std::max(ymax, point[1]);
                zmin = std::min(zmin, point[2]);
                zmax = std::max(zmax, point[2]);
            }
        }

        if (!std::isfinite(xmin) || !std::isfinite(ymin) || !std::isfinite(zmin))
        {
            return full_box;
        }

        double padding = 120.0;
        if (const char* padding_env = gSystem->Getenv("G4LARBOX_DISPLAY_LOCAL_PADDING_CM"))
        {
            padding = std::max(0.0, std::atof(padding_env));
        }

        BoxGeometry local;
        local.x = 0.5 * (xmin + xmax);
        local.y = 0.5 * (ymin + ymax);
        local.z = 0.5 * (zmin + zmax);
        local.dx = std::max(120.0, 0.5 * (xmax - xmin) + padding);
        local.dy = std::max(120.0, 0.5 * (ymax - ymin) + padding);
        local.dz = std::max(120.0, 0.5 * (zmax - zmin) + padding);
        return local;
    }

    void RenderFallbackProjection(const std::vector<TrackPath>& tracks,
                                  const TruthInfo& truth,
                                  const std::vector<LambdaDecayInfo>& lambdas,
                                  const std::string& output_prefix,
                                  const std::vector<std::string>& formats,
                                  const std::vector<RenderMode>& modes,
                                  int event_index)
    {
        const BoxGeometry box = MakeLocalEventBox(LoadBoxGeometry(), tracks);
        const bool multiple_modes = modes.size() > 1;
        (void)lambdas;

        for (const auto& mode : modes)
        {
            const std::string canvas_name = "box_gdml_event_display_projection_" + mode.name;
            auto* canvas = new TCanvas(canvas_name.c_str(),
                                       "G4LArBox GDML event display projection",
                                       1600,
                                       900);
            canvas->SetFillColor(kWhite);
            canvas->SetFrameFillColor(kWhite);
            canvas->SetBorderMode(0);
            canvas->SetFrameBorderMode(0);
            canvas->Clear();
            canvas->SetMargin(0.0, 0.0, 0.0, 0.0);
            canvas->Range(0.0, 0.0, 1600.0, 900.0);
            gPad->SetFillColor(kWhite);
            gPad->SetFrameFillColor(kWhite);
            gPad->SetBorderMode(0);
            gPad->SetFrameBorderMode(0);

            auto* background = new TBox(0.0, 0.0, 1600.0, 900.0);
            background->SetFillColor(kWhite);
            background->SetLineColor(kWhite);
            background->SetFillStyle(1001);
            background->Draw();

            DrawFallbackGdmlBox(box);
            DrawFallbackNeutrinoFlight(box, truth);
            DrawFallbackTracks(box, tracks);
            if (mode.debug)
            {
                DrawFallbackDebugOverlays(box, tracks, truth, event_index);
            }

            const std::string prefix = OutputPrefixForMode(output_prefix, mode, multiple_modes);
            for (const auto& format : formats)
            {
                const std::string output = prefix + "." + format;
                EnsureOutputDirectory(output);
                gSystem->Unlink(output.c_str());
                canvas->Print(output.c_str());

                if (gSystem->AccessPathName(output.c_str()))
                {
                    std::cerr << "ROOT did not create " << output
                              << "; this ROOT build may not support that graphics backend." << std::endl;
                }
                else
                {
                    std::cout << "Saved " << output << std::endl;
                }
            }

            delete canvas;
        }
    }
}

void box_gdml_event_display(const char* input_file = "data/output.root",
                            int event_index = 0,
                            const char* gdml_file = "gdml/lar_box.gdml",
                            const char* output_prefix = "data/event_displays/box_event0_gdml",
                            const char* output_formats = "png,pdf",
                            int max_segments = 60000,
                            const char* render_modes = "clean",
                            const char* track_pdgs = "",
                            int primary_only = 0,
                            double min_step_cm = 0.005,
                            int max_tracks = 2000)
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);

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

    DisplayOptions display_options;
    display_options.pdg_filter = ParsePdgFilter(track_pdgs);
    display_options.primary_only = primary_only != 0;
    display_options.min_step_cm = min_step_cm;
    display_options.max_tracks = max_tracks;
    const std::vector<RenderMode> parsed_render_modes = ParseRenderModes(render_modes);
    const std::vector<std::string> formats = SplitFormats(output_formats);

    std::vector<TrackPath> tracks;
    if (!LoadTrackPaths(*step_tree, event_index, tracks, max_segments, display_options))
    {
        return;
    }

    const TruthInfo truth = LoadTruth(dynamic_cast<TTree*>(file.Get("truthTree")), event_index);

    gSystem->Load("libGeom");
    gSystem->Load("libGdml");
    gSystem->Load("libRGL");
    TGeoManager::Import(gdml_file);
    if (gGeoManager == nullptr)
    {
        std::cerr << "Failed to import GDML file " << gdml_file << std::endl;
        return;
    }

    StyleGeometry();
    const BoxGeometry box = LoadBoxGeometry();
    const std::vector<LambdaDecayInfo> lambdas =
        LoadLambdaDecays(dynamic_cast<TTree*>(file.Get("trackTree")), event_index, box);

    const char* force_projection_env = gSystem->Getenv("G4LARBOX_FORCE_PROJECTION");
    if (force_projection_env != nullptr && std::string(force_projection_env) != "0")
    {
        RenderFallbackProjection(tracks,
                                 truth,
                                 lambdas,
                                 output_prefix,
                                 formats,
                                 parsed_render_modes,
                                 event_index);
        return;
    }

    auto* top = gGeoManager->GetTopVolume();
    if (top == nullptr)
    {
        std::cerr << "Imported GDML has no top volume: " << gdml_file << std::endl;
        return;
    }

    auto* canvas = new TCanvas("box_gdml_event_display",
                               "G4LArBox GDML event display",
                               1600,
                               1100);
    canvas->SetFillColor(kWhite);
    canvas->SetFrameFillColor(kWhite);
    canvas->cd();

    top->Draw("ogl");
    DrawNeutrinoFlight(box, truth);
    DrawTracks(tracks, truth, HasDebugMode(parsed_render_modes));

    auto* viewer = ConfigureViewer(14.0, 1680.0, 0.08, 0.35);
    if (viewer == nullptr)
    {
        std::cerr << "No ROOT OpenGL viewer was available; using GDML-derived projection fallback." << std::endl;
        delete canvas;
        RenderFallbackProjection(tracks,
                                 truth,
                                 lambdas,
                                 output_prefix,
                                 formats,
                                 parsed_render_modes,
                                 event_index);
        return;
    }

    const bool multiple_modes = parsed_render_modes.size() > 1;
    for (const auto& mode : parsed_render_modes)
    {
        SaveViewer(*viewer,
                   OutputPrefixForMode(output_prefix, mode, multiple_modes),
                   formats,
                   4400,
                   3000);
    }

    delete canvas;
}

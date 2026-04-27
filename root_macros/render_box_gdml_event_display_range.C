#include "box_gdml_event_display.C"

#include "TSystem.h"

#include <string>

void render_box_gdml_event_display_range(const char* input_file = "data/output.root",
                                         int first_event = 0,
                                         int last_event = 99,
                                         const char* gdml_file = "gdml/lar_box.gdml",
                                         const char* output_prefix_template = "data/event_displays/box_event{event}_gdml",
                                         const char* output_formats = "png",
                                         int max_segments = 12000,
                                         const char* render_modes = "clean",
                                         const char* track_pdgs = "",
                                         int primary_only = 0,
                                         double min_step_cm = 0.1,
                                         int max_tracks = 900)
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);
    gSystem->Setenv("G4LARBOX_FORCE_PROJECTION", "1");

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
    auto* truth_tree = dynamic_cast<TTree*>(file.Get("truthTree"));
    auto* track_tree = dynamic_cast<TTree*>(file.Get("trackTree"));

    DisplayOptions display_options;
    display_options.pdg_filter = ParsePdgFilter(track_pdgs);
    display_options.primary_only = primary_only != 0;
    display_options.min_step_cm = min_step_cm;
    display_options.max_tracks = max_tracks;
    const std::vector<RenderMode> parsed_render_modes = ParseRenderModes(render_modes);
    const std::vector<std::string> formats = SplitFormats(output_formats);

    gSystem->Load("libGeom");
    gSystem->Load("libGdml");
    TGeoManager::Import(gdml_file);
    if (gGeoManager == nullptr)
    {
        std::cerr << "Failed to import GDML file " << gdml_file << std::endl;
        return;
    }
    StyleGeometry();

    for (int event_index = first_event; event_index <= last_event; ++event_index)
    {
        std::string output_prefix = output_prefix_template == nullptr ? "" : output_prefix_template;
        const std::string token = "{event}";
        const std::string event_text = std::to_string(event_index);
        std::size_t pos = 0;
        while ((pos = output_prefix.find(token, pos)) != std::string::npos)
        {
            output_prefix.replace(pos, token.size(), event_text);
            pos += event_text.size();
        }

        std::vector<TrackPath> tracks;
        if (!LoadTrackPaths(*step_tree, event_index, tracks, max_segments, display_options))
        {
            continue;
        }

        const TruthInfo truth = LoadTruth(truth_tree, event_index);
        const BoxGeometry box = LoadBoxGeometry();
        const std::vector<LambdaDecayInfo> lambdas = LoadLambdaDecays(track_tree, event_index, box);
        RenderFallbackProjection(tracks,
                                 truth,
                                 lambdas,
                                 output_prefix,
                                 formats,
                                 parsed_render_modes,
                                 event_index);
    }
}

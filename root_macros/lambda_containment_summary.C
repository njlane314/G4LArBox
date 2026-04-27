#include "TFile.h"
#include "TSystem.h"
#include "TTree.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    constexpr double kHalfWidthCm = 128.175;
    constexpr double kHalfHeightCm = 116.5;
    constexpr double kHalfLengthCm = 518.4;

    double ToCm(double value_mm)
    {
        return 0.1 * value_mm;
    }

    bool InsideActive(double x_cm, double y_cm, double z_cm)
    {
        return std::abs(x_cm) <= kHalfWidthCm &&
               std::abs(y_cm) <= kHalfHeightCm &&
               std::abs(z_cm) <= kHalfLengthCm;
    }

    const char* YesNo(bool value)
    {
        return value ? "yes" : "no";
    }

    const char* YesNoOrNA(bool applies, bool value)
    {
        return applies ? YesNo(value) : "n/a";
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
}

void lambda_containment_summary(const char* input_file = "data/output.root",
                                const char* output_csv = "data/lambda_containment_summary.csv")
{
    TFile file(input_file, "READ");
    if (file.IsZombie())
    {
        std::cerr << "Failed to open " << input_file << std::endl;
        return;
    }

    auto* track_tree = dynamic_cast<TTree*>(file.Get("trackTree"));
    if (track_tree == nullptr)
    {
        std::cerr << "Missing trackTree in " << input_file << std::endl;
        return;
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

    const std::string output_path(output_csv);
    const char* output_dir = gSystem->DirName(output_path.c_str());
    if (output_dir != nullptr && std::string(output_dir) != ".")
    {
        gSystem->mkdir(output_dir, true);
    }

    std::ofstream out(output_csv);
    out << "event,lambda_track_id,lambda_start_x_cm,lambda_start_y_cm,lambda_start_z_cm,"
        << "lambda_decay_x_cm,lambda_decay_y_cm,lambda_decay_z_cm,"
        << "lambda_decay_inside_active,charged_p_pi_mode,"
        << "charged_daughter_start_inside,charged_daughter_end_inside,daughter_pdgs\n";

    int lambda_count = 0;
    int contained_decay_count = 0;
    int charged_mode_count = 0;

    const Long64_t entries = track_tree->GetEntries();
    for (Long64_t event = 0; event < entries; ++event)
    {
        track_tree->GetEntry(event);
        if (pdg == nullptr || curid == nullptr || preid == nullptr)
        {
            continue;
        }

        const std::size_t count = std::min({pdg->size(), curid->size(), preid->size()});
        for (std::size_t i = 0; i < count; ++i)
        {
            if ((*pdg)[i] != 3122 && (*pdg)[i] != -3122)
            {
                continue;
            }

            const int lambda_id = (*curid)[i];
            const bool anti_lambda = (*pdg)[i] == -3122;
            const int proton_pdg = anti_lambda ? -2212 : 2212;
            const int pion_pdg = anti_lambda ? 211 : -211;

            const double lambda_start_x = ToCm(ValueOrNaN(xv, i));
            const double lambda_start_y = ToCm(ValueOrNaN(yv, i));
            const double lambda_start_z = ToCm(ValueOrNaN(zv, i));
            const double lambda_decay_x = ToCm(ValueOrNaN(xf != nullptr ? xf : xi, i));
            const double lambda_decay_y = ToCm(ValueOrNaN(yf != nullptr ? yf : yi, i));
            const double lambda_decay_z = ToCm(ValueOrNaN(zf != nullptr ? zf : zi, i));
            const bool decay_inside = InsideActive(lambda_decay_x, lambda_decay_y, lambda_decay_z);

            bool has_proton = false;
            bool has_pion = false;
            bool charged_daughter_start_inside = true;
            bool charged_daughter_end_inside = true;
            std::ostringstream daughter_pdgs;

            for (std::size_t j = 0; j < count; ++j)
            {
                if ((*preid)[j] != lambda_id)
                {
                    continue;
                }

                if (daughter_pdgs.tellp() > 0)
                {
                    daughter_pdgs << ";";
                }
                daughter_pdgs << (*pdg)[j];

                const bool charged_signal_daughter = (*pdg)[j] == proton_pdg || (*pdg)[j] == pion_pdg;
                if ((*pdg)[j] == proton_pdg)
                {
                    has_proton = true;
                }
                if ((*pdg)[j] == pion_pdg)
                {
                    has_pion = true;
                }

                if (charged_signal_daughter)
                {
                    const double start_x = ToCm(ValueOrNaN(xv, j));
                    const double start_y = ToCm(ValueOrNaN(yv, j));
                    const double start_z = ToCm(ValueOrNaN(zv, j));
                    const double end_x = ToCm(ValueOrNaN(xf != nullptr ? xf : xi, j));
                    const double end_y = ToCm(ValueOrNaN(yf != nullptr ? yf : yi, j));
                    const double end_z = ToCm(ValueOrNaN(zf != nullptr ? zf : zi, j));

                    charged_daughter_start_inside =
                        charged_daughter_start_inside && InsideActive(start_x, start_y, start_z);
                    charged_daughter_end_inside =
                        charged_daughter_end_inside && InsideActive(end_x, end_y, end_z);
                }
            }

            const bool charged_mode = has_proton && has_pion;
            ++lambda_count;
            contained_decay_count += decay_inside ? 1 : 0;
            charged_mode_count += charged_mode ? 1 : 0;

            out << event << ","
                << lambda_id << ","
                << lambda_start_x << ","
                << lambda_start_y << ","
                << lambda_start_z << ","
                << lambda_decay_x << ","
                << lambda_decay_y << ","
                << lambda_decay_z << ","
                << YesNo(decay_inside) << ","
                << YesNo(charged_mode) << ","
                << YesNoOrNA(charged_mode, charged_daughter_start_inside) << ","
                << YesNoOrNA(charged_mode, charged_daughter_end_inside) << ","
                << daughter_pdgs.str() << "\n";
        }
    }

    std::cout << "Wrote " << output_csv << std::endl;
    std::cout << "Lambda decays: " << lambda_count
              << ", inside active: " << contained_decay_count
              << ", charged p pi mode: " << charged_mode_count << std::endl;
}

#include "PhotonLibrary.hh"

#include "TDirectory.h"
#include "TFile.h"
#include "TKey.h"
#include "TTree.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace G4LArBox
{
    namespace
    {
        TTree* FindPhotonLibraryTree(TDirectory* directory)
        {
            if (directory == nullptr)
            {
                return nullptr;
            }

            if (auto* tree = dynamic_cast<TTree*>(directory->Get("PhotonLibraryData")))
            {
                return tree;
            }

            TIter next(directory->GetListOfKeys());
            while (auto* object = next())
            {
                auto* key = dynamic_cast<TKey*>(object);
                if (key == nullptr)
                {
                    continue;
                }

                std::unique_ptr<TObject> value(key->ReadObj());
                if (auto* tree = dynamic_cast<TTree*>(value.get()))
                {
                    if (std::string(tree->GetName()) == "PhotonLibraryData")
                    {
                        value.release();
                        return tree;
                    }
                }

                if (auto* child = dynamic_cast<TDirectory*>(value.get()))
                {
                    if (auto* tree = FindPhotonLibraryTree(child))
                    {
                        value.release();
                        return tree;
                    }
                }
            }

            return nullptr;
        }

        const char* FirstExistingBranch(TTree* tree,
                                        std::initializer_list<const char*> names)
        {
            for (const char* name : names)
            {
                if (tree->GetBranch(name) != nullptr)
                {
                    return name;
                }
            }

            return nullptr;
        }
    }

    PhotonLibrary::PhotonLibrary()
        : PhotonLibrary(Config())
    {}

    PhotonLibrary::PhotonLibrary(const Config& config)
        : config_(config)
    {}

    PhotonLibrary::~PhotonLibrary()
    {}

    bool PhotonLibrary::Load()
    {
        loaded_ = false;
        channel_count_ = 0;
        nvoxels_ = 0;
        direct_.clear();
        reflected_.clear();

        if (config_.file_path.empty())
        {
            return false;
        }

        std::unique_ptr<TFile> file(TFile::Open(config_.file_path.c_str(), "READ"));
        if (!file || file->IsZombie())
        {
            std::cerr << "PhotonLibrary: could not open " << config_.file_path << std::endl;
            return false;
        }

        TTree* tree = FindPhotonLibraryTree(file.get());
        if (tree == nullptr)
        {
            std::cerr << "PhotonLibrary: PhotonLibraryData tree not found in "
                      << config_.file_path << std::endl;
            return false;
        }

        const char* voxel_branch = FirstExistingBranch(tree, {"Voxel", "vox", "voxel"});
        const char* channel_branch = FirstExistingBranch(tree, {"OpChannel", "OpDet", "Channel", "channel"});
        const char* visibility_branch = FirstExistingBranch(tree, {"Visibility", "visibility"});
        const char* reflected_branch = FirstExistingBranch(tree, {"ReflVisibility", "ReflectedVisibility"});

        if (voxel_branch == nullptr || channel_branch == nullptr || visibility_branch == nullptr)
        {
            std::cerr << "PhotonLibrary: required Voxel/OpChannel/Visibility branches are missing in "
                      << config_.file_path << std::endl;
            return false;
        }

        Int_t voxel = 0;
        Int_t channel = 0;
        Float_t visibility = 0.0f;
        Float_t refl_visibility = 0.0f;
        tree->SetBranchAddress(voxel_branch, &voxel);
        tree->SetBranchAddress(channel_branch, &channel);
        tree->SetBranchAddress(visibility_branch, &visibility);
        if (reflected_branch != nullptr)
        {
            tree->SetBranchAddress(reflected_branch, &refl_visibility);
        }

        int max_voxel = -1;
        int max_channel = -1;
        const Long64_t entries = tree->GetEntries();
        for (Long64_t entry = 0; entry < entries; ++entry)
        {
            tree->GetEntry(entry);
            max_voxel = std::max(max_voxel, static_cast<int>(voxel));
            max_channel = std::max(max_channel, static_cast<int>(channel));
        }

        const int configured_voxels = config_.nx * config_.ny * config_.nz;
        nvoxels_ = configured_voxels > 0 ? configured_voxels : max_voxel + 1;
        channel_count_ = config_.channel_count > 0 ? config_.channel_count : max_channel + 1;
        if (nvoxels_ <= 0 || channel_count_ <= 0)
        {
            return false;
        }

        direct_.assign(static_cast<size_t>(nvoxels_) * channel_count_, 0.0f);
        if (config_.load_reflected && reflected_branch != nullptr)
        {
            reflected_.assign(static_cast<size_t>(nvoxels_) * channel_count_, 0.0f);
        }

        for (Long64_t entry = 0; entry < entries; ++entry)
        {
            tree->GetEntry(entry);
            if (voxel < 0 || voxel >= nvoxels_ || channel < 0 || channel >= channel_count_)
            {
                continue;
            }

            const size_t index = static_cast<size_t>(voxel) * channel_count_ + channel;
            direct_[index] = visibility;
            if (!reflected_.empty())
            {
                reflected_[index] = refl_visibility;
            }
        }

        loaded_ = true;
        std::cout << "PhotonLibrary: loaded " << config_.file_path << " with "
                  << nvoxels_ << " voxels and " << channel_count_ << " channels"
                  << std::endl;
        return true;
    }

    std::vector<double>
    PhotonLibrary::Visibilities(double x_mm, double y_mm, double z_mm, bool reflected) const
    {
        if (!loaded_)
        {
            return {};
        }

        const double x_cm = x_mm / 10.0;
        const double y_cm = y_mm / 10.0;
        const double z_cm = z_mm / 10.0;
        if (!IsInside(x_cm, y_cm, z_cm))
        {
            return {};
        }

        if (config_.interpolate)
        {
            return InterpolatedVisibilities(x_cm, y_cm, z_cm, reflected);
        }

        const int voxel = VoxelId(x_cm, y_cm, z_cm);
        if (voxel < 0)
        {
            return {};
        }

        std::vector<double> output(channel_count_, 0.0);
        for (int channel = 0; channel < channel_count_; ++channel)
        {
            output[channel] = VisibilityAt(voxel, channel, reflected);
        }
        return output;
    }

    bool PhotonLibrary::IsInside(double x_cm, double y_cm, double z_cm) const
    {
        return x_cm >= config_.x_min_cm && x_cm < config_.x_max_cm &&
               y_cm >= config_.y_min_cm && y_cm < config_.y_max_cm &&
               z_cm >= config_.z_min_cm && z_cm < config_.z_max_cm;
    }

    int PhotonLibrary::VoxelId(double x_cm, double y_cm, double z_cm) const
    {
        if (!IsInside(x_cm, y_cm, z_cm))
        {
            return -1;
        }

        const double x_step = (x_cm - config_.x_min_cm) /
                              (config_.x_max_cm - config_.x_min_cm) * config_.nx;
        const double y_step = (y_cm - config_.y_min_cm) /
                              (config_.y_max_cm - config_.y_min_cm) * config_.ny;
        const double z_step = (z_cm - config_.z_min_cm) /
                              (config_.z_max_cm - config_.z_min_cm) * config_.nz;

        const int ix = std::clamp(static_cast<int>(x_step), 0, config_.nx - 1);
        const int iy = std::clamp(static_cast<int>(y_step), 0, config_.ny - 1);
        const int iz = std::clamp(static_cast<int>(z_step), 0, config_.nz - 1);
        return ix + iy * config_.nx + iz * config_.nx * config_.ny;
    }

    float PhotonLibrary::VisibilityAt(int voxel, int channel, bool reflected) const
    {
        if (voxel < 0 || voxel >= nvoxels_ || channel < 0 || channel >= channel_count_)
        {
            return 0.0f;
        }

        const auto& table = reflected && !reflected_.empty() ? reflected_ : direct_;
        const size_t index = static_cast<size_t>(voxel) * channel_count_ + channel;
        return index < table.size() ? table[index] : 0.0f;
    }

    std::vector<double>
    PhotonLibrary::InterpolatedVisibilities(double x_cm,
                                            double y_cm,
                                            double z_cm,
                                            bool reflected) const
    {
        const double fx = (x_cm - config_.x_min_cm) /
                          (config_.x_max_cm - config_.x_min_cm) * config_.nx;
        const double fy = (y_cm - config_.y_min_cm) /
                          (config_.y_max_cm - config_.y_min_cm) * config_.ny;
        const double fz = (z_cm - config_.z_min_cm) /
                          (config_.z_max_cm - config_.z_min_cm) * config_.nz;

        const int x0 = std::clamp(static_cast<int>(std::floor(fx)), 0, config_.nx - 2);
        const int y0 = std::clamp(static_cast<int>(std::floor(fy)), 0, config_.ny - 2);
        const int z0 = std::clamp(static_cast<int>(std::floor(fz)), 0, config_.nz - 2);
        const double tx = std::clamp(fx - x0, 0.0, 1.0);
        const double ty = std::clamp(fy - y0, 0.0, 1.0);
        const double tz = std::clamp(fz - z0, 0.0, 1.0);

        std::vector<double> output(channel_count_, 0.0);
        for (int dx = 0; dx <= 1; ++dx)
        {
            for (int dy = 0; dy <= 1; ++dy)
            {
                for (int dz = 0; dz <= 1; ++dz)
                {
                    const double weight =
                        (dx ? tx : 1.0 - tx) *
                        (dy ? ty : 1.0 - ty) *
                        (dz ? tz : 1.0 - tz);
                    const int voxel = (x0 + dx) +
                                      (y0 + dy) * config_.nx +
                                      (z0 + dz) * config_.nx * config_.ny;
                    for (int channel = 0; channel < channel_count_; ++channel)
                    {
                        output[channel] += weight * VisibilityAt(voxel, channel, reflected);
                    }
                }
            }
        }

        return output;
    }
}

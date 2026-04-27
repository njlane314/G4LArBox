#ifndef PHOTONLIBRARY_HH
#define PHOTONLIBRARY_HH

#include <string>
#include <vector>

namespace G4LArBox
{
    class PhotonLibrary
    {
    public:
        struct Config
        {
            std::string file_path;

            int nx = 85;
            int ny = 174;
            int nz = 220;
            double x_min_cm = -425.0;
            double x_max_cm = 425.0;
            double y_min_cm = -781.26;
            double y_max_cm = 781.26;
            double z_min_cm = -104.0305;
            double z_max_cm = 2195.6405;

            bool interpolate = false;
            bool load_reflected = false;
            int channel_count = 0;
        };

        PhotonLibrary();
        explicit PhotonLibrary(const Config& config);
        ~PhotonLibrary();

        bool Load();
        bool IsLoaded() const { return loaded_; }
        int ChannelCount() const { return channel_count_; }

        std::vector<double> Visibilities(double x_mm,
                                         double y_mm,
                                         double z_mm,
                                         bool reflected = false) const;

    private:
        Config config_;
        bool loaded_ = false;
        int channel_count_ = 0;
        int nvoxels_ = 0;
        std::vector<float> direct_;
        std::vector<float> reflected_;

        int VoxelId(double x_cm, double y_cm, double z_cm) const;
        bool IsInside(double x_cm, double y_cm, double z_cm) const;
        float VisibilityAt(int voxel, int channel, bool reflected) const;
        std::vector<double> InterpolatedVisibilities(double x_cm,
                                                     double y_cm,
                                                     double z_cm,
                                                     bool reflected) const;
    };
}

#endif // PHOTONLIBRARY_HH

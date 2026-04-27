#ifndef CORSIKAREADER_HH
#define CORSIKAREADER_HH

#include <string>
#include <vector>

class TFile;
class TTree;

namespace G4LArBox
{
    struct CorsikaPrimary
    {
        int pdg = 0;
        double px = 0.0;
        double py = 0.0;
        double pz = 0.0;
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        double t = 0.0;
    };

    struct CorsikaEvent
    {
        int iev = -1;
        std::vector<CorsikaPrimary> primaries;
    };

    class CorsikaReader
    {
    public:
        CorsikaReader();
        ~CorsikaReader();

        void Configure(const std::string& file_path, const std::string& tree_name);
        CorsikaEvent ReadNext(bool cycle_events);

    private:
        void Close();
        void Open();
        void EnsureOpen();
        void BindRequiredBranch(const char* branch_name, void* address);

        std::string requested_file_path_;
        std::string requested_tree_name_;

        TFile* input_file_;
        TTree* input_tree_;
        long long current_entry_;
        long long total_entries_;

        static constexpr int kMaxParticles = 256;

        int iev_;
        int nprimary_;
        int pdg_[kMaxParticles];
        double px_[kMaxParticles];
        double py_[kMaxParticles];
        double pz_[kMaxParticles];
        double x_[kMaxParticles];
        double y_[kMaxParticles];
        double z_[kMaxParticles];
        double t_[kMaxParticles];
    };
}

#endif // CORSIKAREADER_HH

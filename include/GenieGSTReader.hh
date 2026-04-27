#ifndef GENIEGSTREADER_HH
#define GENIEGSTREADER_HH

#include <string>
#include <vector>

class TFile;
class TTree;

namespace G4LArBox
{
    struct GenieGSTEvent
    {
        int iev = -1;
        int neu = 0;
        int tgt = 0;
        int target_z = 0;
        int target_a = 0;
        bool cc = false;
        bool nc = false;
        bool qel = false;
        bool res = false;
        bool dis = false;
        bool coh = false;
        bool nuel = false;
        bool imd = false;
        bool em = false;
        double weight = 1.0;
        double xs = 0.0;
        double Ev = 0.0;
        double El = 0.0;
        double pxl = 0.0;
        double pyl = 0.0;
        double pzl = 0.0;
        double vtxx = 0.0;
        double vtxy = 0.0;
        double vtxz = 0.0;
        double vtxt = 0.0;
        std::vector<int> pdgf;
        std::vector<double> Ef;
        std::vector<double> pxf;
        std::vector<double> pyf;
        std::vector<double> pzf;
    };

    class GenieGSTReader
    {
    public:
        GenieGSTReader();
        ~GenieGSTReader();

        void Configure(const std::string& file_path,
                       const std::string& tree_name,
                       bool final_state_only);
        GenieGSTEvent ReadNext(bool cycle_events);

    private:
        void Close();
        void Open();
        void EnsureOpen();
        void BindRequiredBranch(const char* branch_name, void* address);
        void BindOptionalBranch(const char* branch_name, void* address);
        void ResetEventStorage();

        std::string requested_file_path_;
        std::string requested_tree_name_;
        bool requested_final_state_only_;

        TFile* input_file_;
        TTree* input_tree_;
        long long current_entry_;
        long long total_entries_;

        static constexpr int kMaxParticles = 256;

        int iev_;
        int neu_;
        int tgt_;
        int target_z_;
        int target_a_;
        bool cc_;
        bool nc_;
        bool qel_;
        bool res_;
        bool dis_;
        bool coh_;
        bool nuel_;
        bool imd_;
        bool em_;
        double weight_;
        double xs_;
        double Ev_;
        double El_;
        double pxl_;
        double pyl_;
        double pzl_;
        double vtxx_;
        double vtxy_;
        double vtxz_;
        double vtxt_;
        int nf_;
        int pdgf_[kMaxParticles];
        double Ef_[kMaxParticles];
        double pxf_[kMaxParticles];
        double pyf_[kMaxParticles];
        double pzf_[kMaxParticles];
    };
}

#endif // GENIEGSTREADER_HH

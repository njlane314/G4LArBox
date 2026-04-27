#include "CorsikaReader.hh"

#include "TFile.h"
#include "TTree.h"

#include <stdexcept>

namespace G4LArBox
{
    CorsikaReader::CorsikaReader()
        : input_file_(nullptr),
          input_tree_(nullptr),
          current_entry_(0),
          total_entries_(0),
          iev_(-1),
          nprimary_(0)
    {}

    CorsikaReader::~CorsikaReader()
    {
        Close();
    }

    void CorsikaReader::Configure(const std::string& file_path, const std::string& tree_name)
    {
        const std::string normalised_tree_name = tree_name.empty() ? "corsika" : tree_name;
        if (file_path == requested_file_path_ && normalised_tree_name == requested_tree_name_)
        {
            return;
        }

        requested_file_path_ = file_path;
        requested_tree_name_ = normalised_tree_name;
        Close();
    }

    CorsikaEvent CorsikaReader::ReadNext(bool cycle_events)
    {
        EnsureOpen();

        if (total_entries_ == 0)
        {
            throw std::runtime_error("CORSIKA primary tree is empty: " + requested_file_path_);
        }

        if (current_entry_ >= total_entries_)
        {
            if (!cycle_events)
            {
                throw std::runtime_error(
                    "CORSIKA input exhausted after " + std::to_string(total_entries_) +
                    " events. Enable /generator/corsika/cycleEvents to reuse the file.");
            }

            current_entry_ = 0;
        }

        if (input_tree_->GetEntry(current_entry_) <= 0)
        {
            throw std::runtime_error(
                "Failed to read CORSIKA entry " + std::to_string(current_entry_) +
                " from " + requested_file_path_);
        }
        ++current_entry_;

        if (nprimary_ < 0 || nprimary_ > kMaxParticles)
        {
            throw std::runtime_error(
                "CORSIKA event reports nprimary=" + std::to_string(nprimary_) +
                ", outside the supported range 0-" + std::to_string(kMaxParticles) + ".");
        }

        CorsikaEvent event;
        event.iev = iev_;
        event.primaries.reserve(nprimary_);
        for (int i = 0; i < nprimary_; ++i)
        {
            CorsikaPrimary primary;
            primary.pdg = pdg_[i];
            primary.px = px_[i];
            primary.py = py_[i];
            primary.pz = pz_[i];
            primary.x = x_[i];
            primary.y = y_[i];
            primary.z = z_[i];
            primary.t = t_[i];
            event.primaries.push_back(primary);
        }
        return event;
    }

    void CorsikaReader::Close()
    {
        input_tree_ = nullptr;
        current_entry_ = 0;
        total_entries_ = 0;

        if (input_file_ != nullptr)
        {
            input_file_->Close();
            delete input_file_;
            input_file_ = nullptr;
        }
    }

    void CorsikaReader::Open()
    {
        if (requested_file_path_.empty())
        {
            throw std::runtime_error("CORSIKA mode requires /generator/corsika/file to be set.");
        }

        input_file_ = TFile::Open(requested_file_path_.c_str(), "READ");
        if (input_file_ == nullptr || input_file_->IsZombie())
        {
            throw std::runtime_error("Failed to open CORSIKA file: " + requested_file_path_);
        }

        input_tree_ = dynamic_cast<TTree*>(input_file_->Get(requested_tree_name_.c_str()));
        if (input_tree_ == nullptr)
        {
            throw std::runtime_error(
                "Failed to find tree '" + requested_tree_name_ +
                "' in CORSIKA file: " + requested_file_path_);
        }

        input_tree_->SetBranchStatus("*", 0);

        BindRequiredBranch("iev", &iev_);
        BindRequiredBranch("nprimary", &nprimary_);
        BindRequiredBranch("pdg", pdg_);
        BindRequiredBranch("px", px_);
        BindRequiredBranch("py", py_);
        BindRequiredBranch("pz", pz_);
        BindRequiredBranch("x", x_);
        BindRequiredBranch("y", y_);
        BindRequiredBranch("z", z_);
        BindRequiredBranch("t", t_);

        total_entries_ = input_tree_->GetEntries();
        current_entry_ = 0;
    }

    void CorsikaReader::EnsureOpen()
    {
        if (input_tree_ == nullptr)
        {
            Open();
        }
    }

    void CorsikaReader::BindRequiredBranch(const char* branch_name, void* address)
    {
        if (input_tree_->GetBranch(branch_name) == nullptr)
        {
            throw std::runtime_error(
                "CORSIKA tree '" + requested_tree_name_ +
                "' is missing required branch '" + branch_name + "'.");
        }

        input_tree_->SetBranchStatus(branch_name, 1);
        input_tree_->SetBranchAddress(branch_name, address);
    }
}

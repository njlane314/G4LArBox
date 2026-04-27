#include "GenieGSTReader.hh"

#include "TBranch.h"
#include "TFile.h"
#include "TTree.h"

#include <algorithm>
#include <stdexcept>

namespace G4LArBox
{
    GenieGSTReader::GenieGSTReader()
        : requested_final_state_only_(false),
          input_file_(nullptr),
          input_tree_(nullptr),
          current_entry_(0),
          total_entries_(0),
          iev_(-1),
          neu_(0),
          tgt_(0),
          target_z_(0),
          target_a_(0),
          cc_(false),
          nc_(false),
          qel_(false),
          res_(false),
          dis_(false),
          coh_(false),
          nuel_(false),
          imd_(false),
          em_(false),
          weight_(1.0),
          xs_(0.0),
          Ev_(0.0),
          El_(0.0),
          pxl_(0.0),
          pyl_(0.0),
          pzl_(0.0),
          vtxx_(0.0),
          vtxy_(0.0),
          vtxz_(0.0),
          vtxt_(0.0),
          nf_(0)
    {}

    GenieGSTReader::~GenieGSTReader()
    {
        Close();
    }

    void GenieGSTReader::Configure(const std::string& file_path,
                                   const std::string& tree_name,
                                   bool final_state_only)
    {
        const std::string normalised_tree_name = tree_name.empty() ? "gst" : tree_name;
        if (file_path == requested_file_path_ &&
            normalised_tree_name == requested_tree_name_ &&
            final_state_only == requested_final_state_only_)
        {
            return;
        }

        requested_file_path_ = file_path;
        requested_tree_name_ = normalised_tree_name;
        requested_final_state_only_ = final_state_only;
        Close();
    }

    GenieGSTEvent GenieGSTReader::ReadNext(bool cycle_events)
    {
        EnsureOpen();

        if (total_entries_ == 0)
        {
            throw std::runtime_error("GENIE GST tree is empty: " + requested_file_path_);
        }

        if (current_entry_ >= total_entries_)
        {
            if (!cycle_events)
            {
                throw std::runtime_error(
                    "GENIE GST input exhausted after " + std::to_string(total_entries_) +
                    " events. Enable /generator/genie/cycleEvents to reuse the file.");
            }

            current_entry_ = 0;
        }

        ResetEventStorage();
        if (input_tree_->GetEntry(current_entry_) <= 0)
        {
            throw std::runtime_error(
                "Failed to read GENIE GST entry " + std::to_string(current_entry_) +
                " from " + requested_file_path_);
        }
        ++current_entry_;

        if (nf_ < 0 || nf_ > kMaxParticles)
        {
            throw std::runtime_error(
                "GENIE GST event reports nf=" + std::to_string(nf_) +
                ", outside the supported range 0-" + std::to_string(kMaxParticles) + ".");
        }

        GenieGSTEvent event;
        event.iev = iev_;
        event.neu = neu_;
        event.tgt = tgt_;
        event.target_z = target_z_;
        event.target_a = target_a_;
        event.cc = cc_;
        event.nc = nc_;
        event.qel = qel_;
        event.res = res_;
        event.dis = dis_;
        event.coh = coh_;
        event.nuel = nuel_;
        event.imd = imd_;
        event.em = em_;
        event.weight = weight_;
        event.xs = xs_;
        event.Ev = Ev_;
        event.El = El_;
        event.pxl = pxl_;
        event.pyl = pyl_;
        event.pzl = pzl_;
        event.vtxx = vtxx_;
        event.vtxy = vtxy_;
        event.vtxz = vtxz_;
        event.vtxt = vtxt_;

        event.pdgf.assign(pdgf_, pdgf_ + nf_);
        event.Ef.assign(Ef_, Ef_ + nf_);
        event.pxf.assign(pxf_, pxf_ + nf_);
        event.pyf.assign(pyf_, pyf_ + nf_);
        event.pzf.assign(pzf_, pzf_ + nf_);
        return event;
    }

    void GenieGSTReader::Close()
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

    void GenieGSTReader::Open()
    {
        if (requested_file_path_.empty())
        {
            throw std::runtime_error("GENIE GST mode requires /generator/genie/file to be set.");
        }

        input_file_ = TFile::Open(requested_file_path_.c_str(), "READ");
        if (input_file_ == nullptr || input_file_->IsZombie())
        {
            throw std::runtime_error("Failed to open GENIE GST file: " + requested_file_path_);
        }

        input_tree_ = dynamic_cast<TTree*>(input_file_->Get(requested_tree_name_.c_str()));
        if (input_tree_ == nullptr)
        {
            throw std::runtime_error(
                "Failed to find tree '" + requested_tree_name_ +
                "' in GENIE GST file: " + requested_file_path_);
        }

        input_tree_->SetBranchStatus("*", 0);

        if (requested_final_state_only_)
        {
            BindOptionalBranch("iev", &iev_);
            BindOptionalBranch("neu", &neu_);
            BindOptionalBranch("tgt", &tgt_);
            BindOptionalBranch("Z", &target_z_);
            BindOptionalBranch("A", &target_a_);
            BindOptionalBranch("cc", &cc_);
            BindOptionalBranch("nc", &nc_);
            BindOptionalBranch("qel", &qel_);
            BindOptionalBranch("res", &res_);
            BindOptionalBranch("dis", &dis_);
            BindOptionalBranch("coh", &coh_);
            BindOptionalBranch("nuel", &nuel_);
            BindOptionalBranch("imd", &imd_);
            BindOptionalBranch("em", &em_);
            BindOptionalBranch("wght", &weight_);
            BindOptionalBranch("xs", &xs_);
            BindOptionalBranch("Ev", &Ev_);
            BindOptionalBranch("El", &El_);
            BindOptionalBranch("pxl", &pxl_);
            BindOptionalBranch("pyl", &pyl_);
            BindOptionalBranch("pzl", &pzl_);
            BindOptionalBranch("vtxx", &vtxx_);
            BindOptionalBranch("vtxy", &vtxy_);
            BindOptionalBranch("vtxz", &vtxz_);
            BindOptionalBranch("vtxt", &vtxt_);
        }
        else
        {
            BindRequiredBranch("iev", &iev_);
            BindRequiredBranch("neu", &neu_);
            BindRequiredBranch("tgt", &tgt_);
            BindRequiredBranch("Z", &target_z_);
            BindRequiredBranch("A", &target_a_);
            BindRequiredBranch("cc", &cc_);
            BindRequiredBranch("nc", &nc_);
            BindRequiredBranch("qel", &qel_);
            BindRequiredBranch("res", &res_);
            BindRequiredBranch("dis", &dis_);
            BindRequiredBranch("coh", &coh_);
            BindRequiredBranch("nuel", &nuel_);
            BindRequiredBranch("imd", &imd_);
            BindRequiredBranch("em", &em_);
            BindRequiredBranch("wght", &weight_);
            BindRequiredBranch("xs", &xs_);
            BindRequiredBranch("Ev", &Ev_);
            BindRequiredBranch("El", &El_);
            BindRequiredBranch("pxl", &pxl_);
            BindRequiredBranch("pyl", &pyl_);
            BindRequiredBranch("pzl", &pzl_);
            BindRequiredBranch("vtxx", &vtxx_);
            BindRequiredBranch("vtxy", &vtxy_);
            BindRequiredBranch("vtxz", &vtxz_);
            BindRequiredBranch("vtxt", &vtxt_);
        }
        BindRequiredBranch("nf", &nf_);
        BindRequiredBranch("pdgf", pdgf_);
        BindRequiredBranch("Ef", Ef_);
        BindRequiredBranch("pxf", pxf_);
        BindRequiredBranch("pyf", pyf_);
        BindRequiredBranch("pzf", pzf_);

        total_entries_ = input_tree_->GetEntries();
    }

    void GenieGSTReader::EnsureOpen()
    {
        if (input_tree_ == nullptr)
        {
            Open();
        }
    }

    void GenieGSTReader::BindRequiredBranch(const char* branch_name, void* address)
    {
        TBranch* branch = input_tree_->GetBranch(branch_name);
        if (branch == nullptr)
        {
            throw std::runtime_error(
                "GENIE GST tree '" + requested_tree_name_ +
                "' is missing required branch '" + std::string(branch_name) + "'.");
        }

        input_tree_->SetBranchStatus(branch_name, 1);
        input_tree_->SetBranchAddress(branch_name, address);
    }

    void GenieGSTReader::BindOptionalBranch(const char* branch_name, void* address)
    {
        TBranch* branch = input_tree_->GetBranch(branch_name);
        if (branch == nullptr)
        {
            return;
        }

        input_tree_->SetBranchStatus(branch_name, 1);
        input_tree_->SetBranchAddress(branch_name, address);
    }

    void GenieGSTReader::ResetEventStorage()
    {
        iev_ = -1;
        neu_ = 0;
        tgt_ = 0;
        target_z_ = 0;
        target_a_ = 0;
        cc_ = false;
        nc_ = false;
        qel_ = false;
        res_ = false;
        dis_ = false;
        coh_ = false;
        nuel_ = false;
        imd_ = false;
        em_ = false;
        weight_ = 1.0;
        xs_ = 0.0;
        Ev_ = 0.0;
        El_ = 0.0;
        pxl_ = 0.0;
        pyl_ = 0.0;
        pzl_ = 0.0;
        vtxx_ = 0.0;
        vtxy_ = 0.0;
        vtxz_ = 0.0;
        vtxt_ = 0.0;
        nf_ = 0;
        std::fill_n(pdgf_, kMaxParticles, 0);
        std::fill_n(Ef_, kMaxParticles, 0.0);
        std::fill_n(pxf_, kMaxParticles, 0.0);
        std::fill_n(pyf_, kMaxParticles, 0.0);
        std::fill_n(pzf_, kMaxParticles, 0.0);
    }
}

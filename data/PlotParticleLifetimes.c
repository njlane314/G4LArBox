// PlotParticleLifetimes.c
// To run:
// $ root -l -b -q 'PlotParticleLifetimes.c("your_output_file.root")'

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <algorithm>

void PlotParticleLifetimes(const char* filename) {
    TFile* file = new TFile(filename);
    TTree* stepTree = (TTree*)file->Get("stepTree");

    std::vector<double> *ta_ = 0;

    stepTree->SetBranchAddress("ta", &ta_);

    std::vector<double> lifetimes;

    for (int i = 0; i < stepTree->GetEntries(); ++i) {
        stepTree->GetEntry(i);
        
        double creationTime = (*ta_)[0];
        double endTime = (*ta_)[ta_->size() - 1];
        
        lifetimes.push_back(endTime - creationTime);
    }

    TH1F* histLifetime = new TH1F("histLifetime", "Particle Lifetimes; Lifetime [ns]; Count", 
                                  100, 0, *std::max_element(lifetimes.begin(), lifetimes.end()));

    for (const double &lifetime : lifetimes) {
        histLifetime->Fill(lifetime);
    }

    TCanvas* cLifetime = new TCanvas("cLifetime", "Particle Lifetime Distribution", 800, 600);
    cLifetime->SetLogy();
    histLifetime->Draw();
    cLifetime->SaveAs("particle_lifetimes.png");

    delete cLifetime;
    delete histLifetime;
    file->Close();
    delete file;
}

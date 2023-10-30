// PlotParticleTrajectories.c
// To run:
// $ root -l -b -q 'PlotParticleTrajectories.c("your_output_file.root")'

#include <TFile.h>
#include <TTree.h>
#include <TH2F.h>
#include <TH3F.h>
#include <TCanvas.h>

void PlotParticleTrajectories(const char* filename) {
    TFile* file = new TFile(filename);
    TTree* tree = (TTree*)file->Get("stepTree");  

    TH2F* histXZ = new TH2F("histXZ", "x vs z; z [mm]; x [mm]", 
                            1000, -500, 500,  
                            1000, -500, 500); 
    TH2F* histXY = new TH2F("histXY", "x vs y; y [mm]; x [mm]", 
                            1000, -500, 500,  
                            31000, -500, 500); 
    
    tree->Draw("xs:zs>>histXZ", "", "colz");
    tree->Draw("xs:ys>>histXY", "", "colz");

    TCanvas* c1 = new TCanvas("c1", "Canvas for x vs z", 800, 600);
    histXZ->Draw("colz");
    c1->SaveAs("plot_x_vs_z.png");
    
    TCanvas* c2 = new TCanvas("c2", "Canvas for x vs y", 800, 600);
    histXY->Draw("colz");
    c2->SaveAs("plot_x_vs_y.png");

    TH3F* histXYZ = new TH3F("histXYZ", "", 500, -500, 500, 1000, -500, 500, 1000, -500, 500);
    tree->Draw("xs:ys:zs:edep>>histXYZ");

    TCanvas* c3 = new TCanvas("c3", "Canvas for x vs y vs z", 1000, 1000);
    histXYZ->Draw("BOX2");
    c3->SaveAs("plot_x_vs_y_vs_z.png");

    delete c3;
    delete histXYZ;
    
    delete c2;
    delete c1;
    delete histXY;
    delete histXZ;
    file->Close();
    delete file;
}

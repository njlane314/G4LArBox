// PlotParticleTrajectories.c
// To run:
// $ root -l -b -q 'PlotParticleTrajectories.c("your_output_file.root")'

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TH2D.h>
#include <TH3D.h>
#include <TH1.h>
#include <TPaletteAxis.h>
#include <TPad.h>
#include <TPaveStats.h>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>

struct DisplaySummary {
    double totalEnergyMeV = 0.0;
    double displayedEnergyMeV = 0.0;
    int occupiedBins = 0;
    int displayedBins = 0;

    double RetainedFraction() const {
        return totalEnergyMeV > 0.0 ? displayedEnergyMeV / totalEnergyMeV : 0.0;
    }
};

DisplaySummary ApplyDisplayFloor(TH1& histogram, double floorMeV) {
    Double_t originalStats[13] = {};
    histogram.GetStats(originalStats);
    const double entries = histogram.GetEntries();
    DisplaySummary summary;

    for (int bin = 0; bin < histogram.GetNcells(); ++bin) {
        const double energy = histogram.GetBinContent(bin);
        summary.totalEnergyMeV += energy;
        if (energy <= 0.0) {
            continue;
        }
        ++summary.occupiedBins;
        if (energy < floorMeV) {
            histogram.SetBinContent(bin, 0.0);
            histogram.SetBinError(bin, 0.0);
        } else {
            ++summary.displayedBins;
            summary.displayedEnergyMeV += energy;
        }
    }

    histogram.PutStats(originalStats);
    histogram.SetEntries(entries);
    return summary;
}

void PlotParticleTrajectories(const char* filename, double halfSizeMm = 500.0) {
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(1110);

    constexpr int canvasWidth = 2400;
    constexpr int canvasHeight = 1350;

    TFile file(filename, "READ");
    TTree* tree = nullptr;
    file.GetObject("stepTree", tree);
    if (!tree) {
        Error("PlotParticleTrajectories", "missing stepTree in %s", filename);
        return;
    }

    const int bins2D = 300;
    const int bins3D = 72;
    const double displayFloor2DMeV = 0.05;
    const double displayFloor3DMeV = 0.20;
    const double displayMaximum2DMeV = 200.0;
    const double displayMaximum3DMeV = 30.0;
    const TString activeCut = TString::Format(
        "abs(xs)<=%.17g && abs(ys)<=%.17g && abs(zs)<=%.17g",
        halfSizeMm, halfSizeMm, halfSizeMm);
    const TString energyWeight = TString::Format("edep*(%s)", activeCut.Data());

    TH2D histXY("histXY", "x vs y;y [mm];x [mm]",
                bins2D, -halfSizeMm, halfSizeMm,
                bins2D, -halfSizeMm, halfSizeMm);
    TH2D histXZ("histXZ", "x vs z;z [mm];x [mm]",
                bins2D, -halfSizeMm, halfSizeMm,
                bins2D, -halfSizeMm, halfSizeMm);
    TH2D histYZ("histYZ", "y vs z;z [mm];y [mm]",
                bins2D, -halfSizeMm, halfSizeMm,
                bins2D, -halfSizeMm, halfSizeMm);
    histXY.Sumw2();
    histXZ.Sumw2();
    histYZ.Sumw2();

    tree->Draw("xs:ys>>histXY", energyWeight, "goff");
    tree->Draw("xs:zs>>histXZ", energyWeight, "goff");
    tree->Draw("ys:zs>>histYZ", energyWeight, "goff");

    const DisplaySummary summaryXY = ApplyDisplayFloor(histXY, displayFloor2DMeV);
    const DisplaySummary summaryXZ = ApplyDisplayFloor(histXZ, displayFloor2DMeV);
    const DisplaySummary summaryYZ = ApplyDisplayFloor(histYZ, displayFloor2DMeV);
    for (TH2D* histogram : {&histXY, &histXZ, &histYZ}) {
        histogram->SetMinimum(displayFloor2DMeV);
        histogram->SetMaximum(displayMaximum2DMeV);
    }
    Info("PlotParticleTrajectories",
         "2D display floor %.3g MeV retains XY %.2f%%, XZ %.2f%%, YZ %.2f%%",
         displayFloor2DMeV, 100.0 * summaryXY.RetainedFraction(),
         100.0 * summaryXZ.RetainedFraction(),
         100.0 * summaryYZ.RetainedFraction());

    auto drawProjection = [=](const char* canvasName, TH2D& histogram,
                              const char* output) {
        TCanvas canvas(canvasName, histogram.GetTitle(), canvasWidth, canvasHeight);
        canvas.SetCanvasSize(canvasWidth, canvasHeight);
        const TString padName = TString::Format("%sPad", canvasName);
        TPad pad(padName, "", 0.15, 0.0, 0.85, 1.0);
        // Keep equal physical ranges at equal pixel scales inside the 16:9 canvas.
        pad.SetLeftMargin(0.14);
        pad.SetRightMargin(0.24);
        pad.SetBottomMargin(0.13);
        pad.SetTopMargin(0.0984444444);
        pad.SetTicks(1, 1);
        pad.SetLogz();
        pad.Draw();
        pad.cd();
        histogram.Draw("COLZ");
        pad.Update();
        if (auto* palette = dynamic_cast<TPaletteAxis*>(
                histogram.GetListOfFunctions()->FindObject("palette"))) {
            palette->SetX1NDC(0.77);
            palette->SetX2NDC(0.81);
            palette->SetY1NDC(0.13);
            palette->SetY2NDC(0.70);
        }
        if (auto* stats = dynamic_cast<TPaveStats*>(
                histogram.GetListOfFunctions()->FindObject("stats"))) {
            stats->SetX1NDC(0.82);
            stats->SetX2NDC(0.98);
            stats->SetY1NDC(0.72);
            stats->SetY2NDC(0.96);
        }
        pad.Modified();
        pad.Update();
        canvas.SaveAs(output);
    };

    drawProjection("canvasXY", histXY, "plots/plot_x_vs_y.png");
    drawProjection("canvasXZ", histXZ, "plots/plot_x_vs_z.png");
    drawProjection("canvasYZ", histYZ, "plots/plot_y_vs_z.png");

    TH3D histXYZ("histXYZ", "xs:ys:zs:edep;x [mm];y [mm];z [mm]",
                 bins3D, -halfSizeMm, halfSizeMm,
                 bins3D, -halfSizeMm, halfSizeMm,
                 bins3D, -halfSizeMm, halfSizeMm);
    histXYZ.Sumw2();
    tree->Draw("zs:ys:xs>>histXYZ", energyWeight, "goff");
    const DisplaySummary summaryXYZ =
        ApplyDisplayFloor(histXYZ, displayFloor3DMeV);
    histXYZ.SetMinimum(displayFloor3DMeV);
    histXYZ.SetMaximum(displayMaximum3DMeV);
    histXYZ.SetStats(kFALSE);
    for (TAxis* axis : {histXYZ.GetXaxis(), histXYZ.GetYaxis(),
                        histXYZ.GetZaxis()}) {
        axis->SetNdivisions(505);
        axis->SetLabelSize(0.028);
        axis->SetTitleSize(0.032);
    }
    histXYZ.GetXaxis()->SetTitleOffset(1.75);
    histXYZ.GetYaxis()->SetTitleOffset(1.75);
    histXYZ.GetZaxis()->SetTitleOffset(1.35);
    Info("PlotParticleTrajectories",
         "3D display floor %.3g MeV retains %.2f%%",
         displayFloor3DMeV, 100.0 * summaryXYZ.RetainedFraction());

    TCanvas canvasXYZ("canvasXYZ", "Canvas for x vs y vs z",
                      canvasWidth, canvasHeight);
    canvasXYZ.SetCanvasSize(canvasWidth, canvasHeight);
    TPad padXYZ("canvasXYZPad", "", 0.03, 0.0, 0.97, 1.0);
    padXYZ.SetLeftMargin(0.10);
    padXYZ.SetRightMargin(0.17);
    padXYZ.SetBottomMargin(0.12);
    padXYZ.SetTopMargin(0.08);
    padXYZ.SetTicks(1, 1);
    padXYZ.Draw();
    padXYZ.cd();
    histXYZ.Draw("BOX2Z");
    padXYZ.Update();
    if (auto* palette = dynamic_cast<TPaletteAxis*>(
            histXYZ.GetListOfFunctions()->FindObject("palette"))) {
        palette->SetX1NDC(0.86);
        palette->SetX2NDC(0.89);
        palette->SetY1NDC(0.15);
        palette->SetY2NDC(0.85);
    }
    padXYZ.Modified();
    padXYZ.Update();
    canvasXYZ.SaveAs("plots/plot_x_vs_y_vs_z.png");
}

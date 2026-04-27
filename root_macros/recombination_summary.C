#include "TCanvas.h"
#include "TBox.h"
#include "TColor.h"
#include "TGraph.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TLine.h"
#include "TPad.h"
#include "TStyle.h"
#include "TSystem.h"

#include <cmath>
#include <string>
#include <vector>

namespace
{
    double BirksChargeFraction(double dEdx, double eField)
    {
        constexpr double kARecomb = 0.800;
        constexpr double kKRecomb = 0.0486;
        return kARecomb / (1.0 + dEdx * kKRecomb / eField);
    }

    double EscapeChargeFraction(double dEdx, double eField)
    {
        constexpr double kChi0A = 0.00338427;
        constexpr double kChi0B = -6.57037;
        constexpr double kChi0C = 1.88418;
        constexpr double kChi0D = 0.000129379;
        constexpr double kAlpha = 0.0372;
        constexpr double kBeta = 0.0124;

        const double safe_dEdx = std::max(dEdx, 1.0);
        const double escaping_fraction = kChi0A / (kChi0B + std::exp(kChi0C + kChi0D * safe_dEdx));
        const double field_correction = std::exp(-eField / (kAlpha * std::log(safe_dEdx) + kBeta));
        return escaping_fraction * field_correction;
    }

    double ChargeFraction(double dEdx, double eField)
    {
        return BirksChargeFraction(dEdx, eField) + EscapeChargeFraction(dEdx, eField);
    }

    void EnsureDirectory(const std::string& path)
    {
        const char* dir_name = gSystem->DirName(path.c_str());
        if (dir_name != nullptr && std::string(dir_name) != ".")
        {
            gSystem->mkdir(dir_name, true);
        }
    }
}

void recombination_summary(const char* output_file = "data/recombination_summary.pdf")
{
    gStyle->SetOptStat(0);
    gStyle->SetTitleFont(42, "XYZ");
    gStyle->SetLabelFont(42, "XYZ");
    gStyle->SetTitleSize(0.052, "XYZ");
    gStyle->SetLabelSize(0.045, "XYZ");
    gStyle->SetTitleOffset(1.05, "X");
    gStyle->SetTitleOffset(1.15, "Y");

    constexpr double kField = 0.5;
    constexpr double kInitialIons = 1.0e4;
    constexpr double kBandMin = 1.9;
    constexpr double kBandMax = 4.6;

    std::vector<double> dEdx;
    std::vector<double> charge_fraction;
    std::vector<double> recombined_fraction;
    std::vector<double> birks_fraction;
    std::vector<double> escape_fraction;
    std::vector<double> collected_electrons;
    std::vector<double> recombination_light;

    for (int i = 0; i <= 250; ++i)
    {
        const double value = 1.0 + 0.076 * i;
        const double charge = ChargeFraction(value, kField);

        dEdx.push_back(value);
        charge_fraction.push_back(charge);
        recombined_fraction.push_back(1.0 - charge);
        birks_fraction.push_back(BirksChargeFraction(value, kField));
        escape_fraction.push_back(EscapeChargeFraction(value, kField));
        collected_electrons.push_back(kInitialIons * charge);
        recombination_light.push_back(kInitialIons * (1.0 - charge));
    }

    TGraph gChargeFraction(static_cast<int>(dEdx.size()), dEdx.data(), charge_fraction.data());
    TGraph gRecombinedFraction(static_cast<int>(dEdx.size()), dEdx.data(), recombined_fraction.data());
    TGraph gBirksFraction(static_cast<int>(dEdx.size()), dEdx.data(), birks_fraction.data());
    TGraph gEscapeFraction(static_cast<int>(dEdx.size()), dEdx.data(), escape_fraction.data());
    TGraph gCollectedElectrons(static_cast<int>(dEdx.size()), dEdx.data(), collected_electrons.data());
    TGraph gRecombinationLight(static_cast<int>(dEdx.size()), dEdx.data(), recombination_light.data());

    gChargeFraction.SetLineColor(kAzure + 2);
    gChargeFraction.SetLineWidth(3);
    gRecombinedFraction.SetLineColor(kOrange + 7);
    gRecombinedFraction.SetLineWidth(3);
    gBirksFraction.SetLineColor(kAzure - 4);
    gBirksFraction.SetLineStyle(2);
    gBirksFraction.SetLineWidth(2);
    gEscapeFraction.SetLineColor(kGreen + 2);
    gEscapeFraction.SetLineStyle(7);
    gEscapeFraction.SetLineWidth(2);
    gCollectedElectrons.SetLineColor(kAzure + 2);
    gCollectedElectrons.SetLineWidth(3);
    gRecombinationLight.SetLineColor(kOrange + 7);
    gRecombinationLight.SetLineWidth(3);

    EnsureDirectory(output_file);

    TCanvas canvas("canvas", "Recombination Summary", 1200, 860);
    auto* top_pad = new TPad("top_pad", "top_pad", 0.0, 0.50, 1.0, 1.0);
    auto* bottom_pad = new TPad("bottom_pad", "bottom_pad", 0.0, 0.0, 1.0, 0.50);
    top_pad->Draw();
    bottom_pad->Draw();

    top_pad->cd();
    gPad->SetLeftMargin(0.12);
    gPad->SetRightMargin(0.05);
    gPad->SetBottomMargin(0.10);
    gPad->SetTopMargin(0.12);
    gChargeFraction.SetTitle("Charge survival and recombination fractions;dE/dx [MeV/cm];fraction of initial ion pairs");
    gChargeFraction.GetYaxis()->SetRangeUser(0.0, 1.0);
    gChargeFraction.GetXaxis()->SetRangeUser(1.0, 20.0);
    gChargeFraction.Draw("AL");

    TBox mip_band_top(kBandMin, 0.0, kBandMax, 1.0);
    mip_band_top.SetFillColorAlpha(kGray + 1, 0.10);
    mip_band_top.SetLineColorAlpha(kGray + 1, 0.0);
    mip_band_top.Draw();
    gChargeFraction.Draw("L same");
    gRecombinedFraction.Draw("L same");
    gBirksFraction.Draw("L same");
    gEscapeFraction.Draw("L same");

    TLegend legTop(0.58, 0.48, 0.89, 0.84);
    legTop.SetBorderSize(0);
    legTop.SetFillStyle(0);
    legTop.SetTextSize(0.040);
    legTop.AddEntry(&gChargeFraction, "Collected charge fraction", "l");
    legTop.AddEntry(&gRecombinedFraction, "Recombined fraction", "l");
    legTop.AddEntry(&gBirksFraction, "Birks-like term", "l");
    legTop.AddEntry(&gEscapeFraction, "Escape term", "l");
    legTop.Draw();

    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.042);
    latex.DrawLatex(0.14, 0.91, "G4LArBox recombination model at E = 0.5 kV/cm");
    latex.SetTextSize(0.035);
    latex.DrawLatex(0.17, 0.82, "Typical stopping-power band for through-going muons and contained protons");

    bottom_pad->cd();
    gPad->SetLeftMargin(0.12);
    gPad->SetRightMargin(0.05);
    gPad->SetBottomMargin(0.16);
    gPad->SetTopMargin(0.05);
    gCollectedElectrons.SetTitle("Illustrative yield split for 10^{4} initial ion pairs;dE/dx [MeV/cm];quanta");
    gCollectedElectrons.GetYaxis()->SetRangeUser(0.0, kInitialIons);
    gCollectedElectrons.GetXaxis()->SetRangeUser(1.0, 20.0);
    gCollectedElectrons.Draw("AL");

    TBox mip_band_bottom(kBandMin, 0.0, kBandMax, kInitialIons);
    mip_band_bottom.SetFillColorAlpha(kGray + 1, 0.10);
    mip_band_bottom.SetLineColorAlpha(kGray + 1, 0.0);
    mip_band_bottom.Draw();
    gCollectedElectrons.Draw("L same");
    gRecombinationLight.Draw("L same");

    TLegend legBottom(0.56, 0.68, 0.89, 0.84);
    legBottom.SetBorderSize(0);
    legBottom.SetFillStyle(0);
    legBottom.SetTextSize(0.040);
    legBottom.AddEntry(&gCollectedElectrons, "Collected thermal electrons", "l");
    legBottom.AddEntry(&gRecombinationLight, "Extra scintillation from recombination", "l");
    legBottom.Draw();

    canvas.SaveAs(output_file);
}

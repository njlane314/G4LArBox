#include <TCanvas.h>
#include <TColor.h>
#include <TError.h>
#include <TBox.h>
#include <TGeoBBox.h>
#include <TGeoManager.h>
#include <TGeoVolume.h>
#include <TLatex.h>
#include <TObjArray.h>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TView.h>

namespace {

bool Eq(const TString& text, const char* value) {
  return text.EqualTo(value);
}

bool Starts(const TString& text, const char* value) {
  return text.BeginsWith(value);
}

bool Contains(const TString& text, const char* value) {
  return text.Contains(value, TString::kIgnoreCase);
}

void StyleVolume(TGeoVolume* volume, int color, int transparency, int lineWidth = 1) {
  volume->SetVisibility(kTRUE);
  volume->SetVisContainers(kTRUE);
  volume->SetVisDaughters(kTRUE);
  volume->SetLineColor(color);
  volume->SetFillColor(color);
  volume->SetTransparency(transparency);
  volume->SetLineWidth(lineWidth);
}

void HideVolume(TGeoVolume* volume) {
  volume->SetVisibility(kFALSE);
  volume->SetVisContainers(kFALSE);
}

bool IsContainer(const TString& name) {
  return Eq(name, "volWorld") ||
         Eq(name, "volNDBucket") ||
         Eq(name, "volArgonColumn") ||
         Eq(name, "volInnerDetector") ||
         Eq(name, "volHalfDetector") ||
         Eq(name, "volFieldcage") ||
         Eq(name, "volLAr") ||
         Eq(name, "volTPC") ||
         Eq(name, "volTPCActive");
}

bool IsArCLight(const TString& name) {
  return Eq(name, "volOpticalDet") ||
         Eq(name, "volArCLight") ||
         Eq(name, "volWLS") ||
         Eq(name, "volSiPM") ||
         Eq(name, "volSiPM_Sens") ||
         Eq(name, "volSiPM_Mask") ||
         Eq(name, "volSiPM_PCB") ||
         Eq(name, "volPCBBar");
}

void SetView(Double_t longitude, Double_t latitude, Double_t psi) {
  gPad->Update();
  TView* view = gPad->GetView();
  if (!view) return;
  int irep = 0;
  view->SetView(longitude, latitude, psi, irep);
  gPad->Modified();
  gPad->Update();
}

void ApplyFullStyle(TGeoManager* geom) {
  const int blue = TColor::GetColor("#2f80ed");
  const int teal = TColor::GetColor("#1aa6b7");
  const int orange = TColor::GetColor("#ff7a1a");
  const int magenta = TColor::GetColor("#d000ff");
  const int red = TColor::GetColor("#d90429");
  const int gray = TColor::GetColor("#8d99ae");

  TObjArray* volumes = geom->GetListOfVolumes();
  for (int i = 0; i < volumes->GetEntries(); ++i) {
    auto* volume = static_cast<TGeoVolume*>(volumes->At(i));
    if (!volume) continue;

    const TString name(volume->GetName());
    volume->SetVisDaughters(kTRUE);

    if (Eq(name, "volFastDPSUHousing")) {
      StyleVolume(volume, magenta, 0, 5);
    } else if (Eq(name, "volOpDetSensitive_FastDPSU") || Starts(name, "volOpDetSensitive_")) {
      StyleVolume(volume, red, 0, 5);
    } else if (IsContainer(name)) {
      const int color = Eq(name, "volTPCActive") ? blue : gray;
      StyleVolume(volume, color, Eq(name, "volTPCActive") ? 82 : 90, Eq(name, "volTPCActive") ? 2 : 1);
    } else if (IsArCLight(name)) {
      StyleVolume(volume, Eq(name, "volOpticalDet") ? teal : orange, Eq(name, "volOpticalDet") ? 65 : 35, 2);
    } else if (Contains(name, "Pixel") || Contains(name, "Asic") || Contains(name, "PCB")) {
      HideVolume(volume);
    } else {
      HideVolume(volume);
    }
  }
}

void PrepareCanvas(TCanvas* canvas) {
  canvas->SetFillColor(kWhite);
  canvas->SetFrameFillColor(kWhite);
  canvas->SetBorderMode(0);
  canvas->SetMargin(0.0, 0.0, 0.0, 0.0);
}

TGeoBBox* Box(TGeoVolume* volume) {
  if (!volume) return nullptr;
  return dynamic_cast<TGeoBBox*>(volume->GetShape());
}

void DrawOutlineBox(Double_t xmin, Double_t ymin, Double_t xmax, Double_t ymax,
                    Int_t lineColor, Int_t fillColor, Int_t fillStyle, Int_t lineWidth) {
  TBox box(xmin, ymin, xmax, ymax);
  box.SetLineColor(lineColor);
  box.SetFillColor(fillColor);
  box.SetFillStyle(fillStyle);
  box.SetLineWidth(lineWidth);
  box.DrawClone();
}

} // namespace

void render_fastdpsu_geometry(const char* gdmlFile = "gdml/ndlar_single_module_fastdpsu.gdml",
                              const char* outputBase = "data/fastdpsu_renders/fastdpsu",
                              Int_t width = 1800,
                              Int_t height = 1100) {
  gROOT->SetBatch(kTRUE);
  gStyle->SetCanvasColor(kWhite);
  gStyle->SetPadColor(kWhite);
  gStyle->SetFrameFillColor(kWhite);
  gStyle->SetOptStat(0);

  TGeoManager::Import(gdmlFile);
  if (!gGeoManager) {
    Error("render_fastdpsu_geometry", "Failed to import %s", gdmlFile);
    return;
  }

  gSystem->mkdir(gSystem->DirName(outputBase), kTRUE);
  gGeoManager->SetTopVisible(kFALSE);
  gGeoManager->SetVisLevel(100);
  gGeoManager->SetVisOption(1);
  ApplyFullStyle(gGeoManager);

  auto* full = new TCanvas("fastdpsu_full", "FastDPSU full geometry", width, height);
  PrepareCanvas(full);
  gGeoManager->GetTopVolume()->Draw();
  SetView(35.0, 25.0, 0.0);
  full->Print(TString::Format("%s_full.png", outputBase));
  full->Print(TString::Format("%s_full.pdf", outputBase));

  auto* active = gGeoManager->FindVolumeFast("volTPCActive");
  if (active) {
    auto* tpc = new TCanvas("fastdpsu_tpcactive", "FastDPSU in active TPC", 1400, 1000);
    PrepareCanvas(tpc);
    active->SetVisibility(kTRUE);
    active->SetVisContainers(kTRUE);
    active->SetVisDaughters(kTRUE);
    active->SetTransparency(88);
    active->SetLineColor(TColor::GetColor("#2f80ed"));
    active->SetFillColor(TColor::GetColor("#2f80ed"));
    active->SetLineWidth(2);
    active->Draw();
    SetView(35.0, 25.0, 0.0);
    tpc->Print(TString::Format("%s_tpcactive.png", outputBase));
    tpc->Print(TString::Format("%s_tpcactive.pdf", outputBase));
  }

  auto* housing = gGeoManager->FindVolumeFast("volFastDPSUHousing");
  if (!housing) {
    Error("render_fastdpsu_geometry", "Could not find volFastDPSUHousing");
    return;
  }

  auto* close = new TCanvas("fastdpsu_closeup", "FastDPSU marker close-up", 1200, 900);
  PrepareCanvas(close);
  housing->SetVisibility(kTRUE);
  housing->SetVisContainers(kTRUE);
  housing->SetVisDaughters(kTRUE);
  housing->SetTransparency(72);
  housing->SetLineColor(TColor::GetColor("#d000ff"));
  housing->SetFillColor(TColor::GetColor("#d000ff"));
  housing->SetLineWidth(4);

  auto* sensor = gGeoManager->FindVolumeFast("volOpDetSensitive_FastDPSU");
  if (sensor) {
    sensor->SetVisibility(kTRUE);
    sensor->SetVisContainers(kTRUE);
    sensor->SetLineColor(TColor::GetColor("#d90429"));
    sensor->SetFillColor(TColor::GetColor("#d90429"));
    sensor->SetTransparency(0);
    sensor->SetLineWidth(5);
  }

  housing->Draw();
  SetView(35.0, 25.0, 0.0);
  close->Print(TString::Format("%s_closeup.png", outputBase));
  close->Print(TString::Format("%s_closeup.pdf", outputBase));

  auto* activeBox = Box(active);
  auto* housingBox = Box(housing);
  auto* sensorBox = Box(sensor);
  if (activeBox && housingBox && sensorBox) {
    const int blue = TColor::GetColor("#2f80ed");
    const int magenta = TColor::GetColor("#d000ff");
    const int red = TColor::GetColor("#d90429");
    const int gray = TColor::GetColor("#6b7280");

    auto* projection = new TCanvas("fastdpsu_projection", "FastDPSU marker projection", 1500, 760);
    projection->SetFillColor(kWhite);
    projection->Divide(2, 1, 0.02, 0.0);

    projection->cd(1);
    gPad->SetFillColor(kWhite);
    gPad->SetFrameFillColor(kWhite);
    gPad->SetMargin(0.05, 0.02, 0.06, 0.05);
    const Double_t zHalf = activeBox->GetDZ();
    const Double_t yHalf = activeBox->GetDY();
    gPad->Range(-zHalf * 1.12, -yHalf * 1.12, zHalf * 1.12, yHalf * 1.12);
    DrawOutlineBox(-zHalf, -yHalf, zHalf, yHalf, blue, kWhite, 0, 3);
    DrawOutlineBox(-housingBox->GetDZ(), -housingBox->GetDY(),
                   housingBox->GetDZ(), housingBox->GetDY(), magenta, magenta, 3004, 3);
    DrawOutlineBox(-sensorBox->GetDZ(), -sensorBox->GetDY(),
                   sensorBox->GetDZ(), sensorBox->GetDY(), red, red, 1001, 2);
    TLatex title1;
    title1.SetTextSize(0.035);
    title1.DrawLatex(-zHalf * 1.05, yHalf * 1.03, "volTPCActive local Z-Y projection");

    projection->cd(2);
    gPad->SetFillColor(kWhite);
    gPad->SetFrameFillColor(kWhite);
    gPad->SetMargin(0.05, 0.05, 0.06, 0.05);
    const Double_t hX = housingBox->GetDX();
    const Double_t hY = housingBox->GetDY();
    const Double_t sX = sensorBox->GetDX();
    const Double_t sY = sensorBox->GetDY();
    const Double_t sensorCenterX = hX - sX - 1.0e-4;
    gPad->Range(-2.2, -2.0, 2.2, 2.0);
    DrawOutlineBox(-hX, -hY, hX, hY, magenta, kWhite, 0, 4);
    DrawOutlineBox(sensorCenterX - sX, -sY, sensorCenterX + sX, sY, red, red, 1001, 3);
    TLatex title2;
    title2.SetTextSize(0.035);
    title2.DrawLatex(-2.0, 1.75, "FastDPSU housing and sensitive face, local X-Y");

    projection->Print(TString::Format("%s_projection.png", outputBase));
    projection->Print(TString::Format("%s_projection.pdf", outputBase));
  }
}

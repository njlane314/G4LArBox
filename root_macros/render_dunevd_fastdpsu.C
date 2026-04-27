#include <TBox.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TError.h>
#include <TGeoBBox.h>
#include <TGeoManager.h>
#include <TGeoMatrix.h>
#include <TGeoNode.h>
#include <TGeoVolume.h>
#include <TLatex.h>
#include <TLine.h>
#include <TObjArray.h>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TView.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace {

struct Drop {
  double xTop = 0.0;
  double xBottom = 0.0;
  double y = 0.0;
  double z = 0.0;
};

struct Point3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

struct Point2 {
  double x = 0.0;
  double y = 0.0;
};

struct Bounds2D {
  double xmin = std::numeric_limits<double>::max();
  double xmax = -std::numeric_limits<double>::max();
  double ymin = std::numeric_limits<double>::max();
  double ymax = -std::numeric_limits<double>::max();
};

struct WireBox {
  std::array<Point3, 8> corners;
  int color = kBlack;
  int width = 1;
};

bool Eq(const TString& text, const char* value) {
  return text.EqualTo(value);
}

bool Starts(const TString& text, const char* value) {
  return text.BeginsWith(value);
}

bool Contains(const TString& text, const char* value) {
  return text.Contains(value, TString::kIgnoreCase);
}

int Color(const char* hex) {
  return TColor::GetColor(hex);
}

TGeoBBox* Box(TGeoVolume* volume) {
  if (!volume) return nullptr;
  return dynamic_cast<TGeoBBox*>(volume->GetShape());
}

void StyleVolume(TGeoVolume* volume, int color, int transparency, int width = 1) {
  volume->SetVisibility(kTRUE);
  volume->SetVisContainers(kTRUE);
  volume->SetVisDaughters(kTRUE);
  volume->SetLineColor(color);
  volume->SetFillColor(color);
  volume->SetTransparency(transparency);
  volume->SetLineWidth(width);
}

void HideVolume(TGeoVolume* volume) {
  volume->SetVisibility(kFALSE);
  volume->SetVisContainers(kFALSE);
  volume->SetVisDaughters(kTRUE);
}

void ApplyDuneVdStyle(TGeoManager* geom) {
  const int cryo = Color("#5b8def");
  const int active = Color("#2780e3");
  const int pds = Color("#ff8a00");
  const int fastCable = Color("#c000ff");
  const int fastNode = Color("#d0002a");
  const int gray = Color("#9ca3af");

  TObjArray* volumes = geom->GetListOfVolumes();
  for (int i = 0; i < volumes->GetEntries(); ++i) {
    auto* volume = static_cast<TGeoVolume*>(volumes->At(i));
    if (!volume) continue;

    const TString name(volume->GetName());
    volume->SetVisDaughters(kTRUE);

    if (Eq(name, "volWorld") || Eq(name, "volDetEnclosure")) {
      HideVolume(volume);
    } else if (Eq(name, "volCryostat") || Eq(name, "volSteelShell")) {
      StyleVolume(volume, cryo, 92, 1);
    } else if (Eq(name, "volTPCActive") || Eq(name, "volTPC")) {
      StyleVolume(volume, active, Eq(name, "volTPCActive") ? 93 : 96, 1);
    } else if (Eq(name, "volFastDPSUDrop")) {
      StyleVolume(volume, fastCable, 96, 1);
    } else if (Eq(name, "volFastDPSUCable")) {
      StyleVolume(volume, fastCable, 0, 3);
    } else if (Eq(name, "volFastDPSUHousing") || Starts(name, "volOpDetSensitive_FastDPSU")) {
      StyleVolume(volume, fastNode, 0, 4);
    } else if (Contains(name, "Arapuca") || Contains(name, "OpDetSensitive_Arapuca")) {
      StyleVolume(volume, pds, 75, 1);
    } else if (Contains(name, "FieldShaper")) {
      StyleVolume(volume, gray, 82, 1);
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

void SetView(double longitude, double latitude, double psi) {
  gPad->Update();
  TView* view = gPad->GetView();
  if (!view) return;
  int irep = 0;
  view->SetView(longitude, latitude, psi, irep);
  gPad->Modified();
  gPad->Update();
}

std::vector<Drop> CollectDrops(TGeoManager* geom) {
  std::vector<Drop> drops;
  TGeoVolume* top = geom->GetTopVolume();
  auto* dropVolume = geom->FindVolumeFast("volFastDPSUDrop");
  auto* dropBox = Box(dropVolume);
  if (!top || !dropBox) return drops;

  const double localTop[3] = {dropBox->GetDX(), 0.0, 0.0};
  const double localBottom[3] = {-dropBox->GetDX(), 0.0, 0.0};
  const double localCenter[3] = {0.0, 0.0, 0.0};

  TGeoIterator iterator(top);
  TGeoNode* node = nullptr;
  while ((node = iterator())) {
    TGeoVolume* volume = node->GetVolume();
    if (!volume || !Eq(volume->GetName(), "volFastDPSUDrop")) continue;

    const TGeoMatrix* matrix = iterator.GetCurrentMatrix();
    if (!matrix) continue;

    double topPoint[3];
    double bottomPoint[3];
    double centerPoint[3];
    matrix->LocalToMaster(localTop, topPoint);
    matrix->LocalToMaster(localBottom, bottomPoint);
    matrix->LocalToMaster(localCenter, centerPoint);
    drops.push_back({topPoint[0], bottomPoint[0], centerPoint[1], centerPoint[2]});
  }

  return drops;
}

void DrawRect(double xmin, double ymin, double xmax, double ymax, int color, int fillStyle, int width) {
  TBox box(xmin, ymin, xmax, ymax);
  box.SetLineColor(color);
  box.SetFillColor(kWhite);
  box.SetFillStyle(fillStyle);
  box.SetLineWidth(width);
  box.DrawClone();
}

Point2 Project(const Point3& p) {
  return {p.z + 0.34 * p.y, p.x - 0.18 * p.y};
}

void Extend(Bounds2D& bounds, const Point2& p) {
  bounds.xmin = std::min(bounds.xmin, p.x);
  bounds.xmax = std::max(bounds.xmax, p.x);
  bounds.ymin = std::min(bounds.ymin, p.y);
  bounds.ymax = std::max(bounds.ymax, p.y);
}

void Extend(Bounds2D& bounds, const WireBox& box) {
  for (const auto& corner : box.corners) {
    Extend(bounds, Project(corner));
  }
}

void Extend(Bounds2D& bounds, const Drop& drop) {
  Extend(bounds, Project({drop.xTop, drop.y, drop.z}));
  Extend(bounds, Project({drop.xBottom, drop.y, drop.z}));
}

void SetProjectedRange(const Bounds2D& raw, double aspect, double margin = 0.08) {
  Bounds2D bounds = raw;
  double width = bounds.xmax - bounds.xmin;
  double height = bounds.ymax - bounds.ymin;
  if (width <= 0.0 || height <= 0.0) return;

  bounds.xmin -= margin * width;
  bounds.xmax += margin * width;
  bounds.ymin -= margin * height;
  bounds.ymax += margin * height;
  width = bounds.xmax - bounds.xmin;
  height = bounds.ymax - bounds.ymin;

  const double currentAspect = width / height;
  if (currentAspect < aspect) {
    const double targetWidth = height * aspect;
    const double extra = 0.5 * (targetWidth - width);
    bounds.xmin -= extra;
    bounds.xmax += extra;
  } else {
    const double targetHeight = width / aspect;
    const double extra = 0.5 * (targetHeight - height);
    bounds.ymin -= extra;
    bounds.ymax += extra;
  }
  gPad->Range(bounds.xmin, bounds.ymin, bounds.xmax, bounds.ymax);
}

std::array<Point3, 8> BoxCorners(const TGeoMatrix* matrix, const TGeoBBox* box) {
  std::array<Point3, 8> corners;
  int index = 0;
  for (int sx : {-1, 1}) {
    for (int sy : {-1, 1}) {
      for (int sz : {-1, 1}) {
        const double local[3] = {sx * box->GetDX(), sy * box->GetDY(), sz * box->GetDZ()};
        double master[3] = {0.0, 0.0, 0.0};
        matrix->LocalToMaster(local, master);
        corners[index++] = {master[0], master[1], master[2]};
      }
    }
  }
  return corners;
}

std::vector<WireBox> CollectWireBoxes(TGeoManager* geom, const char* volumeName, int color, int width) {
  std::vector<WireBox> boxes;
  TGeoVolume* top = geom->GetTopVolume();
  if (!top) return boxes;

  TGeoIterator iterator(top);
  TGeoNode* node = nullptr;
  while ((node = iterator())) {
    TGeoVolume* volume = node->GetVolume();
    if (!volume || !Eq(volume->GetName(), volumeName)) continue;
    auto* bbox = Box(volume);
    const TGeoMatrix* matrix = iterator.GetCurrentMatrix();
    if (!bbox || !matrix) continue;
    boxes.push_back({BoxCorners(matrix, bbox), color, width});
  }
  return boxes;
}

void DrawWireBox(const WireBox& box) {
  static const int edges[12][2] = {
    {0, 1}, {0, 2}, {0, 4}, {3, 1}, {3, 2}, {3, 7},
    {5, 1}, {5, 4}, {5, 7}, {6, 2}, {6, 4}, {6, 7},
  };
  for (const auto& edge : edges) {
    const Point2 a = Project(box.corners[edge[0]]);
    const Point2 b = Project(box.corners[edge[1]]);
    TLine line(a.x, a.y, b.x, b.y);
    line.SetLineColor(box.color);
    line.SetLineWidth(box.width);
    line.DrawClone();
  }
}

void DrawObliqueSummary(TGeoManager* geom,
                        const std::vector<Drop>& drops,
                        const char* outputBase,
                        int width,
                        int height) {
  auto cryostat = CollectWireBoxes(geom, "volCryostat", Color("#5b8def"), 3);
  auto active = CollectWireBoxes(geom, "volTPCActive", Color("#2780e3"), 1);
  if (cryostat.empty()) return;

  Bounds2D bounds;
  for (const auto& box : cryostat) Extend(bounds, box);
  for (const auto& drop : drops) Extend(bounds, drop);

  auto* canvas = new TCanvas("dunevd_fastdpsu_oblique", "DUNE VD FastDPSU oblique wireframe", width, height);
  canvas->SetFillColor(kWhite);
  canvas->SetFrameFillColor(kWhite);
  canvas->SetMargin(0.02, 0.02, 0.04, 0.04);
  SetProjectedRange(bounds, static_cast<double>(width) / height);

  for (const auto& box : cryostat) DrawWireBox(box);
  for (const auto& box : active) DrawWireBox(box);

  const int cableColor = Color("#c000ff");
  const int nodeColor = Color("#d0002a");
  for (const auto& drop : drops) {
    const Point2 top = Project({drop.xTop, drop.y, drop.z});
    const Point2 bottom = Project({drop.xBottom, drop.y, drop.z});
    TLine line(top.x, top.y, bottom.x, bottom.y);
    line.SetLineColor(cableColor);
    line.SetLineWidth(2);
    line.DrawClone();
    TBox node(bottom.x - 5.0, bottom.y - 5.0, bottom.x + 5.0, bottom.y + 5.0);
    node.SetFillColor(nodeColor);
    node.SetLineColor(nodeColor);
    node.SetFillStyle(1001);
    node.DrawClone();
  }

  TLatex title;
  title.SetNDC(kTRUE);
  title.SetTextSize(0.028);
  title.DrawLatex(0.03, 0.95, "DUNE VD 1x8x14 nowires with repeated FastDPSU dangling nodes");
  canvas->Print(TString::Format("%s_oblique.png", outputBase));
  canvas->Print(TString::Format("%s_oblique.pdf", outputBase));
}

} // namespace

void render_dunevd_fastdpsu(const char* gdmlFile = "gdml/dunevd10kt_2view_v2_refactored_1x8x14ref_nowires_fastdpsu_dangle.gdml",
                            const char* outputBase = "data/dunevd_fastdpsu_renders/dunevd_fastdpsu",
                            int width = 1900,
                            int height = 1150) {
  gROOT->SetBatch(kTRUE);
  gStyle->SetCanvasColor(kWhite);
  gStyle->SetPadColor(kWhite);
  gStyle->SetFrameFillColor(kWhite);
  gStyle->SetOptStat(0);

  TGeoManager::Import(gdmlFile);
  if (!gGeoManager) {
    Error("render_dunevd_fastdpsu", "Failed to import %s", gdmlFile);
    return;
  }

  gSystem->mkdir(gSystem->DirName(outputBase), kTRUE);
  gGeoManager->SetTopVisible(kFALSE);
  gGeoManager->SetVisLevel(100);
  gGeoManager->SetVisOption(1);
  ApplyDuneVdStyle(gGeoManager);

  const auto drops = CollectDrops(gGeoManager);
  auto* cryoBox = Box(gGeoManager->FindVolumeFast("volCryostat"));
  auto* activeBox = Box(gGeoManager->FindVolumeFast("volTPCActive"));
  if (drops.empty() || !cryoBox || !activeBox) {
    Error("render_dunevd_fastdpsu", "Could not collect FastDPSU drops or DUNE VD boxes");
    return;
  }

  DrawObliqueSummary(gGeoManager, drops, outputBase, width, height);

  auto* side = new TCanvas("dunevd_fastdpsu_xz", "DUNE VD FastDPSU X-Z side projection", 1800, 950);
  side->SetFillColor(kWhite);
  side->SetFrameFillColor(kWhite);
  side->SetMargin(0.05, 0.02, 0.06, 0.04);
  const double zMax = cryoBox->GetDZ() * 1.08;
  const double xMin = -cryoBox->GetDX() * 1.12;
  const double xMax = cryoBox->GetDX() * 1.12;
  gPad->Range(-zMax, xMin, zMax, xMax);
  DrawRect(-cryoBox->GetDZ(), -cryoBox->GetDX(), cryoBox->GetDZ(), cryoBox->GetDX(), Color("#5b8def"), 0, 3);

  const int cableColor = Color("#c000ff");
  const int nodeColor = Color("#d0002a");
  for (const auto& drop : drops) {
    TLine line(drop.z, drop.xTop, drop.z, drop.xBottom);
    line.SetLineColor(cableColor);
    line.SetLineWidth(2);
    line.DrawClone();
    TBox node(drop.z - 10.0, drop.xBottom - 10.0, drop.z + 10.0, drop.xBottom + 10.0);
    node.SetFillColor(nodeColor);
    node.SetLineColor(nodeColor);
    node.SetFillStyle(1001);
    node.DrawClone();
  }

  TLatex sideTitle;
  sideTitle.SetTextSize(0.03);
  sideTitle.DrawLatex(-0.95 * zMax, 0.93 * xMax, "DUNE VD 1x8x14 nowires: FastDPSU strings dangling from +X readout/anode side toward -X");
  side->Print(TString::Format("%s_dangling_xz.png", outputBase));
  side->Print(TString::Format("%s_dangling_xz.pdf", outputBase));

  auto* map = new TCanvas("dunevd_fastdpsu_yz", "DUNE VD FastDPSU Y-Z map", 1800, 950);
  map->SetFillColor(kWhite);
  map->SetFrameFillColor(kWhite);
  map->SetMargin(0.05, 0.02, 0.06, 0.04);
  const double yMax = cryoBox->GetDY() * 1.08;
  gPad->Range(-zMax, -yMax, zMax, yMax);
  DrawRect(-cryoBox->GetDZ(), -cryoBox->GetDY(), cryoBox->GetDZ(), cryoBox->GetDY(), Color("#5b8def"), 0, 3);
  for (const auto& drop : drops) {
    TBox node(drop.z - 8.0, drop.y - 8.0, drop.z + 8.0, drop.y + 8.0);
    node.SetFillColor(nodeColor);
    node.SetLineColor(nodeColor);
    node.SetFillStyle(1001);
    node.DrawClone();
  }
  TLatex mapTitle;
  mapTitle.SetTextSize(0.03);
  mapTitle.DrawLatex(-0.95 * zMax, 0.93 * yMax, "FastDPSU node map across the repeated VD active volumes");
  map->Print(TString::Format("%s_node_yz_map.png", outputBase));
  map->Print(TString::Format("%s_node_yz_map.pdf", outputBase));
}

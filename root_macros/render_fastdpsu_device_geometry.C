#include <TBox.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TError.h>
#include <TGeoBBox.h>
#include <TGeoManager.h>
#include <TGeoMatrix.h>
#include <TGeoNode.h>
#include <TGeoSphere.h>
#include <TGeoVolume.h>
#include <TGraph.h>
#include <TLine.h>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TPolyLine.h>
#include <TEllipse.h>

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

namespace {

struct Point3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

struct Point2 {
  double u = 0.0;
  double v = 0.0;
};

struct Bounds2 {
  double umin = std::numeric_limits<double>::max();
  double umax = -std::numeric_limits<double>::max();
  double vmin = std::numeric_limits<double>::max();
  double vmax = -std::numeric_limits<double>::max();
};

struct Bounds3 {
  double xmin = std::numeric_limits<double>::max();
  double xmax = -std::numeric_limits<double>::max();
  double ymin = std::numeric_limits<double>::max();
  double ymax = -std::numeric_limits<double>::max();
  double zmin = std::numeric_limits<double>::max();
  double zmax = -std::numeric_limits<double>::max();
};

struct WireBox {
  TString name;
  std::array<Point3, 8> corners;
};

struct WireSphere {
  TString name;
  Point3 center;
  double radius = 0.0;
};

struct Style {
  int line = kBlack;
  int fill = kWhite;
  double alpha = 0.0;
  int width = 1;
  int priority = 0;
};

bool Eq(const TString& text, const char* value) {
  return text.EqualTo(value);
}

int Color(const char* hex) {
  return TColor::GetColor(hex);
}

TGeoBBox* Box(TGeoVolume* volume) {
  if (!volume) return nullptr;
  return dynamic_cast<TGeoBBox*>(volume->GetShape());
}

TGeoSphere* Sphere(TGeoVolume* volume) {
  if (!volume) return nullptr;
  return dynamic_cast<TGeoSphere*>(volume->GetShape());
}

void Extend(Bounds2& bounds, const Point2& p) {
  bounds.umin = std::min(bounds.umin, p.u);
  bounds.umax = std::max(bounds.umax, p.u);
  bounds.vmin = std::min(bounds.vmin, p.v);
  bounds.vmax = std::max(bounds.vmax, p.v);
}

void Extend(Bounds3& bounds, const Point3& p) {
  bounds.xmin = std::min(bounds.xmin, p.x);
  bounds.xmax = std::max(bounds.xmax, p.x);
  bounds.ymin = std::min(bounds.ymin, p.y);
  bounds.ymax = std::max(bounds.ymax, p.y);
  bounds.zmin = std::min(bounds.zmin, p.z);
  bounds.zmax = std::max(bounds.zmax, p.z);
}

Point2 Project(const Point3& p) {
  return {p.x + 0.55 * p.y, p.z + 0.28 * p.y};
}

Bounds3 BoundsOf(const WireBox& box) {
  Bounds3 bounds;
  for (const auto& corner : box.corners) Extend(bounds, corner);
  return bounds;
}

void PrepareCanvas(TCanvas* canvas) {
  canvas->SetFillColor(kWhite);
  canvas->SetFrameFillColor(kWhite);
  canvas->SetBorderMode(0);
  canvas->SetMargin(0.0, 0.0, 0.0, 0.0);
}

void RangeWithAspect(const Bounds2& raw, double aspect, double margin = 0.12) {
  Bounds2 bounds = raw;
  double width = bounds.umax - bounds.umin;
  double height = bounds.vmax - bounds.vmin;
  if (width <= 0.0 || height <= 0.0) return;

  bounds.umin -= margin * width;
  bounds.umax += margin * width;
  bounds.vmin -= margin * height;
  bounds.vmax += margin * height;
  width = bounds.umax - bounds.umin;
  height = bounds.vmax - bounds.vmin;

  const double currentAspect = width / height;
  if (currentAspect < aspect) {
    const double target = height * aspect;
    const double extra = 0.5 * (target - width);
    bounds.umin -= extra;
    bounds.umax += extra;
  } else {
    const double target = width / aspect;
    const double extra = 0.5 * (target - height);
    bounds.vmin -= extra;
    bounds.vmax += extra;
  }
  gPad->Range(bounds.umin, bounds.vmin, bounds.umax, bounds.vmax);
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

std::vector<WireBox> CollectBoxes(TGeoManager* geom) {
  std::vector<WireBox> boxes;
  TGeoVolume* top = geom->GetTopVolume();
  if (!top) return boxes;

  TGeoIterator iterator(top);
  TGeoNode* node = nullptr;
  while ((node = iterator())) {
    TGeoVolume* volume = node->GetVolume();
    if (!volume || Eq(volume->GetName(), "volWorld")) continue;
    if (Eq(volume->GetName(), "volFastDPSUSphericalNode")) continue;
    auto* bbox = Box(volume);
    const TGeoMatrix* matrix = iterator.GetCurrentMatrix();
    if (!bbox || !matrix) continue;
    boxes.push_back({volume->GetName(), BoxCorners(matrix, bbox)});
  }
  return boxes;
}

std::vector<WireSphere> CollectSpheres(TGeoManager* geom) {
  std::vector<WireSphere> spheres;
  TGeoVolume* top = geom->GetTopVolume();
  if (!top) return spheres;

  const double local[3] = {0.0, 0.0, 0.0};
  TGeoIterator iterator(top);
  TGeoNode* node = nullptr;
  while ((node = iterator())) {
    TGeoVolume* volume = node->GetVolume();
    if (!volume || Eq(volume->GetName(), "volWorld")) continue;
    auto* sphere = Sphere(volume);
    const TGeoMatrix* matrix = iterator.GetCurrentMatrix();
    if (!sphere || !matrix) continue;
    double master[3] = {0.0, 0.0, 0.0};
    matrix->LocalToMaster(local, master);
    spheres.push_back({volume->GetName(), {master[0], master[1], master[2]}, sphere->GetRmax()});
  }
  return spheres;
}

Style BoxStyle(const TString& name) {
  if (Eq(name, "volFastDPSUPowerFiber")) {
    return {Color("#9a5a16"), Color("#d58b3a"), 0.55, 2, 1};
  }
  if (Eq(name, "volFastDPSUSignalFiber")) {
    return {Color("#0e7490"), Color("#a5f3fc"), 0.65, 2, 2};
  }
  if (Eq(name, "volFastDPSUSphericalNode")) {
    return {Color("#2f7d5b"), Color("#d9fbe8"), 0.24, 3, 3};
  }
  if (Eq(name, "volFastDPSUFiberClip")) {
    return {Color("#4b5563"), Color("#cbd5e1"), 0.55, 2, 4};
  }
  if (Eq(name, "volFastDPSUCarrierPlate")) {
    return {Color("#2f7d5b"), Color("#d9fbe8"), 0.45, 2, 5};
  }
  if (Eq(name, "volFastDPSUReadoutElectronics")) {
    return {Color("#7b57ff"), Color("#eee7ff"), 0.82, 2, 6};
  }
  if (Eq(name, "volFastDPSUOpticalPowerConverter")) {
    return {Color("#ff8c1a"), Color("#ffd166"), 0.86, 2, 7};
  }
  if (Eq(name, "volFastDPSUSiliconPhotonics")) {
    return {Color("#ff6f3c"), Color("#ffb86b"), 0.86, 2, 7};
  }
  if (Eq(name, "volFastDPSUFiberFerrule")) {
    return {Color("#b45309"), Color("#fde68a"), 0.80, 2, 8};
  }
  if (Eq(name, "volOpDetSensitive_FastDPSU")) {
    return {Color("#155e75"), Color("#2b6f91"), 0.95, 3, 9};
  }
  return {Color("#64748b"), Color("#e2e8f0"), 0.4, 1, 0};
}

void DrawGrid(const Bounds2& b, double spacing, int minorColor, int majorColor) {
  const double u0 = std::floor(b.umin / spacing) * spacing;
  for (double u = u0; u <= b.umax + 0.001; u += spacing) {
    const bool major = std::abs(std::fmod(std::abs(u), spacing * 5.0)) < 0.001;
    TLine line(u, b.vmin, u, b.vmax);
    line.SetLineColor(major ? majorColor : minorColor);
    line.SetLineStyle(major ? 1 : 3);
    line.SetLineWidth(1);
    line.DrawClone();
  }
  const double v0 = std::floor(b.vmin / spacing) * spacing;
  for (double v = v0; v <= b.vmax + 0.001; v += spacing) {
    const bool major = std::abs(std::fmod(std::abs(v), spacing * 5.0)) < 0.001;
    TLine line(b.umin, v, b.umax, v);
    line.SetLineColor(major ? majorColor : minorColor);
    line.SetLineStyle(major ? 1 : 3);
    line.SetLineWidth(1);
    line.DrawClone();
  }
}

void DrawSideBox(const WireBox& box) {
  const auto b = BoundsOf(box);
  const auto s = BoxStyle(box.name);
  TBox fill(b.xmin, b.zmin, b.xmax, b.zmax);
  fill.SetLineColor(s.fill);
  fill.SetFillColorAlpha(s.fill, s.alpha);
  fill.DrawClone("f");

  TBox outline(b.xmin, b.zmin, b.xmax, b.zmax);
  outline.SetLineColor(s.line);
  outline.SetLineWidth(s.width);
  outline.SetFillStyle(0);
  outline.DrawClone("l");
}

void DrawSideSphere(const WireSphere& sphere) {
  const auto s = BoxStyle(sphere.name);
  TEllipse fill(sphere.center.x, sphere.center.z, sphere.radius, sphere.radius);
  fill.SetLineColor(s.fill);
  fill.SetFillColorAlpha(s.fill, s.alpha);
  fill.DrawClone();

  TEllipse outline(sphere.center.x, sphere.center.z, sphere.radius, sphere.radius);
  outline.SetLineColor(s.line);
  outline.SetLineWidth(s.width);
  outline.SetFillStyle(0);
  outline.DrawClone();
}

void DrawTopBox(const WireBox& box) {
  const auto b = BoundsOf(box);
  const auto s = BoxStyle(box.name);
  TBox fill(b.xmin, b.ymin, b.xmax, b.ymax);
  fill.SetLineColor(s.fill);
  fill.SetFillColorAlpha(s.fill, s.alpha);
  fill.DrawClone("f");

  TBox outline(b.xmin, b.ymin, b.xmax, b.ymax);
  outline.SetLineColor(s.line);
  outline.SetLineWidth(s.width);
  outline.SetFillStyle(0);
  outline.DrawClone("l");
}

void DrawTopSphere(const WireSphere& sphere) {
  const auto s = BoxStyle(sphere.name);
  TEllipse fill(sphere.center.x, sphere.center.y, sphere.radius, sphere.radius);
  fill.SetLineColor(s.fill);
  fill.SetFillColorAlpha(s.fill, s.alpha);
  fill.DrawClone();

  TEllipse outline(sphere.center.x, sphere.center.y, sphere.radius, sphere.radius);
  outline.SetLineColor(s.line);
  outline.SetLineWidth(s.width);
  outline.SetFillStyle(0);
  outline.DrawClone();
}

void DrawWireBox(const WireBox& box) {
  static const int edges[12][2] = {
      {0, 1}, {0, 2}, {0, 4}, {3, 1}, {3, 2}, {3, 7},
      {5, 1}, {5, 4}, {5, 7}, {6, 2}, {6, 4}, {6, 7},
  };
  const auto s = BoxStyle(box.name);
  for (const auto& edge : edges) {
    const auto a = Project(box.corners[edge[0]]);
    const auto b = Project(box.corners[edge[1]]);
    TLine line(a.u, a.v, b.u, b.v);
    line.SetLineColor(s.line);
    line.SetLineWidth(s.width);
    line.DrawClone();
  }
}

void DrawWireSphere(const WireSphere& sphere) {
  const auto s = BoxStyle(sphere.name);
  const auto center = Project(sphere.center);
  TEllipse outline(center.u, center.v, sphere.radius, sphere.radius);
  outline.SetLineColor(s.line);
  outline.SetLineWidth(s.width);
  outline.SetFillColorAlpha(s.fill, s.alpha);
  outline.DrawClone();
  outline.SetFillStyle(0);
  outline.DrawClone();
}

std::vector<WireBox> Ordered(std::vector<WireBox> boxes) {
  std::stable_sort(boxes.begin(), boxes.end(), [](const WireBox& a, const WireBox& b) {
    return BoxStyle(a.name).priority < BoxStyle(b.name).priority;
  });
  return boxes;
}

Bounds3 OverallBounds(const std::vector<WireBox>& boxes) {
  Bounds3 b;
  for (const auto& box : boxes) {
    for (const auto& corner : box.corners) Extend(b, corner);
  }
  return b;
}

Bounds3 OverallBounds(const std::vector<WireBox>& boxes, const std::vector<WireSphere>& spheres) {
  Bounds3 b = OverallBounds(boxes);
  for (const auto& sphere : spheres) {
    Extend(b, {sphere.center.x - sphere.radius, sphere.center.y - sphere.radius, sphere.center.z - sphere.radius});
    Extend(b, {sphere.center.x + sphere.radius, sphere.center.y + sphere.radius, sphere.center.z + sphere.radius});
  }
  return b;
}

void DrawElevation(
    const char* outPath,
    const std::vector<WireBox>& boxes,
    const std::vector<WireSphere>& spheres,
    int width,
    int height) {
  TCanvas canvas("fastdpsu_device_elevation", "fastdpsu_device_elevation", width, height);
  PrepareCanvas(&canvas);
  const auto bounds3 = OverallBounds(boxes, spheres);
  Bounds2 b;
  Extend(b, {bounds3.xmin, bounds3.zmin});
  Extend(b, {bounds3.xmax, bounds3.zmax});
  RangeWithAspect(b, static_cast<double>(width) / height, 0.20);
  DrawGrid(b, 0.5, Color("#eef2ff"), Color("#7b57ff"));
  for (const auto& sphere : spheres) DrawSideSphere(sphere);
  for (const auto& box : Ordered(boxes)) DrawSideBox(box);
  canvas.SaveAs(outPath);
}

void DrawPlan(
    const char* outPath,
    const std::vector<WireBox>& boxes,
    const std::vector<WireSphere>& spheres,
    int width,
    int height) {
  TCanvas canvas("fastdpsu_device_plan", "fastdpsu_device_plan", width, height);
  PrepareCanvas(&canvas);
  const auto bounds3 = OverallBounds(boxes, spheres);
  Bounds2 b;
  Extend(b, {bounds3.xmin, bounds3.ymin});
  Extend(b, {bounds3.xmax, bounds3.ymax});
  RangeWithAspect(b, static_cast<double>(width) / height, 0.20);
  DrawGrid(b, 0.25, Color("#eef2ff"), Color("#7b57ff"));
  for (const auto& sphere : spheres) DrawTopSphere(sphere);
  for (const auto& box : Ordered(boxes)) DrawTopBox(box);
  canvas.SaveAs(outPath);
}

void DrawOblique(
    const char* outPath,
    const std::vector<WireBox>& boxes,
    const std::vector<WireSphere>& spheres,
    int width,
    int height) {
  TCanvas canvas("fastdpsu_device_oblique", "fastdpsu_device_oblique", width, height);
  PrepareCanvas(&canvas);
  Bounds2 b;
  for (const auto& box : boxes) {
    for (const auto& corner : box.corners) Extend(b, Project(corner));
  }
  for (const auto& sphere : spheres) {
    const auto center = Project(sphere.center);
    Extend(b, {center.u - sphere.radius, center.v - sphere.radius});
    Extend(b, {center.u + sphere.radius, center.v + sphere.radius});
  }
  RangeWithAspect(b, static_cast<double>(width) / height, 0.20);
  DrawGrid(b, 0.5, Color("#eef2ff"), Color("#7b57ff"));
  for (const auto& sphere : spheres) DrawWireSphere(sphere);
  for (const auto& box : Ordered(boxes)) DrawWireBox(box);
  canvas.SaveAs(outPath);
}

}  // namespace

void render_fastdpsu_device_geometry(
    const char* gdmlPath = "gdml/fastdpsu_device_concept.gdml",
    const char* outPrefix = "data/fastdpsu_device_renders/fastdpsu_device",
    int width = 1200,
    int height = 850) {
  gROOT->SetBatch(kTRUE);
  gStyle->SetOptStat(0);
  gErrorIgnoreLevel = kWarning;

  TGeoManager* geom = TGeoManager::Import(gdmlPath);
  if (!geom) {
    Error("render_fastdpsu_device_geometry", "Could not import %s", gdmlPath);
    return;
  }

  auto boxes = CollectBoxes(geom);
  auto spheres = CollectSpheres(geom);
  if (boxes.empty() && spheres.empty()) {
    Error("render_fastdpsu_device_geometry", "No drawable device components found");
    return;
  }

  gSystem->mkdir(gSystem->DirName(outPrefix), kTRUE);
  DrawElevation(TString::Format("%s_elevation.png", outPrefix), boxes, spheres, width, height);
  DrawPlan(TString::Format("%s_plan.png", outPrefix), boxes, spheres, width, 520);
  DrawOblique(TString::Format("%s_oblique.png", outPrefix), boxes, spheres, width, height);
}

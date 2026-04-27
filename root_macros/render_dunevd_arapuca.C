#include <TCanvas.h>
#include <TBox.h>
#include <TColor.h>
#include <TError.h>
#include <TGeoBBox.h>
#include <TGeoManager.h>
#include <TGeoMatrix.h>
#include <TGeoNode.h>
#include <TGeoVolume.h>
#include <TLine.h>
#include <TPolyLine.h>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>

#include <algorithm>
#include <array>
#include <cmath>
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
  std::array<Point3, 8> corners;
  bool valid = false;
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

Point2 Project(const Point3& p) {
  return {p.z + 0.30 * p.y, p.x - 0.17 * p.y};
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

std::array<Point3, 8> BoundsCorners(const Bounds3& b) {
  return {{
      {b.xmin, b.ymin, b.zmin}, {b.xmin, b.ymin, b.zmax},
      {b.xmin, b.ymax, b.zmin}, {b.xmin, b.ymax, b.zmax},
      {b.xmax, b.ymin, b.zmin}, {b.xmax, b.ymin, b.zmax},
      {b.xmax, b.ymax, b.zmin}, {b.xmax, b.ymax, b.zmax},
  }};
}

void ExtendProjected(Bounds2& bounds, const Bounds3& b) {
  for (const auto& corner : BoundsCorners(b)) Extend(bounds, Project(corner));
}

Bounds3 BoundsOf(const WireBox& box) {
  Bounds3 bounds;
  for (const auto& corner : box.corners) Extend(bounds, corner);
  return bounds;
}

Point3 CenterOf(const WireBox& box) {
  const Bounds3 b = BoundsOf(box);
  return {
      0.5 * (b.xmin + b.xmax),
      0.5 * (b.ymin + b.ymax),
      0.5 * (b.zmin + b.zmax),
  };
}

bool CenterInsideYZ(const WireBox& box, const Bounds3& active, double tolerance = 5.0) {
  const Point3 c = CenterOf(box);
  return c.y >= active.ymin - tolerance && c.y <= active.ymax + tolerance &&
         c.z >= active.zmin - tolerance && c.z <= active.zmax + tolerance;
}

bool CenterInsideXZ(const WireBox& box, const Bounds3& active, double tolerance = 5.0) {
  const Point3 c = CenterOf(box);
  return c.x >= active.xmin - tolerance && c.x <= active.xmax + tolerance &&
         c.z >= active.zmin - tolerance && c.z <= active.zmax + tolerance;
}

void RangeWithAspect(const Bounds2& raw, double aspect, double margin = 0.04) {
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

WireBox CollectFirstBox(TGeoManager* geom, const char* volumeName) {
  WireBox result;
  TGeoVolume* top = geom->GetTopVolume();
  if (!top) return result;

  TGeoIterator iterator(top);
  TGeoNode* node = nullptr;
  while ((node = iterator())) {
    TGeoVolume* volume = node->GetVolume();
    if (!volume || !Eq(volume->GetName(), volumeName)) continue;
    auto* bbox = Box(volume);
    const TGeoMatrix* matrix = iterator.GetCurrentMatrix();
    if (!bbox || !matrix) continue;
    result.corners = BoxCorners(matrix, bbox);
    result.valid = true;
    return result;
  }
  return result;
}

std::vector<WireBox> CollectBoxes(TGeoManager* geom, const char* volumeName) {
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
    boxes.push_back({BoxCorners(matrix, bbox), true});
  }
  return boxes;
}

void PrepareCanvas(TCanvas* canvas) {
  canvas->SetFillColor(kWhite);
  canvas->SetFrameFillColor(kWhite);
  canvas->SetBorderMode(0);
  canvas->SetMargin(0.0, 0.0, 0.0, 0.0);
}

void DrawLine2(const Point2& a, const Point2& b, int color, int width, int style = 1) {
  TLine line(a.u, a.v, b.u, b.v);
  line.SetLineColor(color);
  line.SetLineWidth(width);
  line.SetLineStyle(style);
  line.DrawClone();
}

void DrawWireBoxProjected(const WireBox& box, int color, int width, int style = 1) {
  static const int edges[12][2] = {
      {0, 1}, {0, 2}, {0, 4}, {3, 1}, {3, 2}, {3, 7},
      {5, 1}, {5, 4}, {5, 7}, {6, 2}, {6, 4}, {6, 7},
  };
  for (const auto& edge : edges) {
    DrawLine2(Project(box.corners[edge[0]]), Project(box.corners[edge[1]]), color, width, style);
  }
}

void DrawBoxEdges(const Bounds3& b, int color, int width, int style = 1) {
  const std::array<Point3, 8> c = {{
      {b.xmin, b.ymin, b.zmin}, {b.xmin, b.ymin, b.zmax},
      {b.xmin, b.ymax, b.zmin}, {b.xmin, b.ymax, b.zmax},
      {b.xmax, b.ymin, b.zmin}, {b.xmax, b.ymin, b.zmax},
      {b.xmax, b.ymax, b.zmin}, {b.xmax, b.ymax, b.zmax},
  }};
  const int edges[12][2] = {
      {0, 1}, {0, 2}, {0, 4}, {3, 1}, {3, 2}, {3, 7},
      {5, 1}, {5, 4}, {5, 7}, {6, 2}, {6, 4}, {6, 7},
  };
  for (const auto& edge : edges) {
    DrawLine2(Project(c[edge[0]]), Project(c[edge[1]]), color, width, style);
  }
}

void AddUnique(std::vector<double>& values, double value, double tolerance = 0.5) {
  for (const double existing : values) {
    if (std::abs(existing - value) < tolerance) return;
  }
  values.push_back(value);
}

std::vector<double> UniqueEdges(const std::vector<WireBox>& boxes, char axis) {
  std::vector<double> values;
  for (const auto& box : boxes) {
    const Bounds3 bounds = BoundsOf(box);
    if (axis == 'x') {
      AddUnique(values, bounds.xmin);
      AddUnique(values, bounds.xmax);
    } else if (axis == 'y') {
      AddUnique(values, bounds.ymin);
      AddUnique(values, bounds.ymax);
    } else {
      AddUnique(values, bounds.zmin);
      AddUnique(values, bounds.zmax);
    }
  }
  std::sort(values.begin(), values.end());
  return values;
}

void DrawObliqueSegmentationGrid(const Bounds3& active, const std::vector<WireBox>& activeBoxes) {
  const int gridColor = Color("#7a54ff");
  const int centerColor = Color("#2857ff");
  const double frontY = active.ymax;
  const auto xEdges = UniqueEdges(activeBoxes, 'x');
  const auto zEdges = UniqueEdges(activeBoxes, 'z');

  for (const double z : zEdges) {
    DrawLine2(Project({active.xmin, frontY, z}), Project({active.xmax, frontY, z}), gridColor, 1);
  }
  for (const double x : xEdges) {
    DrawLine2(Project({x, frontY, active.zmin}), Project({x, frontY, active.zmax}), gridColor, 1);
  }

  const double centerX = 0.5 * (active.xmin + active.xmax);
  DrawLine2(Project({centerX, active.ymin, active.zmin}), Project({centerX, active.ymin, active.zmax}), centerColor, 1, 2);
  DrawLine2(Project({centerX, active.ymax, active.zmin}), Project({centerX, active.ymax, active.zmax}), centerColor, 1, 2);
}

void DrawPlanBox(const WireBox& box, int color, int width) {
  const Bounds3 b = BoundsOf(box);
  TLine bottom(b.zmin, b.ymin, b.zmax, b.ymin);
  TLine top(b.zmin, b.ymax, b.zmax, b.ymax);
  TLine left(b.zmin, b.ymin, b.zmin, b.ymax);
  TLine right(b.zmax, b.ymin, b.zmax, b.ymax);
  for (auto* line : {&bottom, &top, &left, &right}) {
    line->SetLineColor(color);
    line->SetLineWidth(width);
    line->DrawClone();
  }
}

void DrawPlanEngineeringGrid(const Bounds3& b) {
  const int minorColor = Color("#f0ebff");
  const int majorColor = Color("#7b57ff");
  const double pitch = 300.0;
  for (double z = std::ceil(b.zmin / pitch) * pitch; z <= b.zmax + 1.0; z += pitch) {
    const bool major = std::abs(std::fmod(std::abs(z), 900.0)) < 1.0;
    TLine line(z, b.ymin, z, b.ymax);
    line.SetLineColor(major ? majorColor : minorColor);
    line.SetLineStyle(major ? 1 : 3);
    line.SetLineWidth(1);
    line.DrawClone();
  }
  for (double y = std::ceil(b.ymin / pitch) * pitch; y <= b.ymax + 1.0; y += pitch) {
    const bool major = std::abs(std::fmod(std::abs(y), 900.0)) < 1.0;
    TLine line(b.zmin, y, b.zmax, y);
    line.SetLineColor(major ? majorColor : minorColor);
    line.SetLineStyle(major ? 1 : 3);
    line.SetLineWidth(1);
    line.DrawClone();
  }
}

void DrawPlanTicks(const Bounds3& b, int color) {
  const double zPitch = 600.0;
  const double yPitch = 300.0;
  const double yTick = 0.035 * (b.ymax - b.ymin);
  const double zTick = 0.012 * (b.zmax - b.zmin);
  for (double z = std::ceil(b.zmin / zPitch) * zPitch; z <= b.zmax + 1.0; z += zPitch) {
    TLine top(z, b.ymax + 0.45 * yTick, z, b.ymax + yTick);
    TLine bottom(z, b.ymin - 0.45 * yTick, z, b.ymin - yTick);
    for (auto* line : {&top, &bottom}) {
      line->SetLineColor(color);
      line->SetLineWidth(2);
      line->DrawClone();
    }
  }
  for (double y = std::ceil(b.ymin / yPitch) * yPitch; y <= b.ymax + 1.0; y += yPitch) {
    TLine left(b.zmin - 0.45 * zTick, y, b.zmin - zTick, y);
    TLine right(b.zmax + 0.45 * zTick, y, b.zmax + zTick, y);
    for (auto* line : {&left, &right}) {
      line->SetLineColor(color);
      line->SetLineWidth(2);
      line->DrawClone();
    }
  }
}

void DrawPlanBoxFilled(const WireBox& box, int lineColor, int fillColor, double alpha, int width) {
  const Bounds3 b = BoundsOf(box);
  TBox fill(b.zmin, b.ymin, b.zmax, b.ymax);
  fill.SetLineColor(fillColor);
  fill.SetFillColorAlpha(fillColor, alpha);
  fill.DrawClone("f");

  TBox outline(b.zmin, b.ymin, b.zmax, b.ymax);
  outline.SetLineColor(lineColor);
  outline.SetLineWidth(width);
  outline.SetFillStyle(0);
  outline.DrawClone("l");
}

void DrawSideBox(const WireBox& box, int color, int width) {
  const Bounds3 b = BoundsOf(box);
  TLine bottom(b.zmin, b.xmin, b.zmax, b.xmin);
  TLine top(b.zmin, b.xmax, b.zmax, b.xmax);
  TLine left(b.zmin, b.xmin, b.zmin, b.xmax);
  TLine right(b.zmax, b.xmin, b.zmax, b.xmax);
  for (auto* line : {&bottom, &top, &left, &right}) {
    line->SetLineColor(color);
    line->SetLineWidth(width);
    line->DrawClone();
  }
}

void DrawSideEngineeringGrid(const Bounds3& b) {
  const int minorColor = Color("#f0ebff");
  const int majorColor = Color("#7b57ff");
  const double zPitch = 300.0;
  const double xPitch = 150.0;
  for (double z = std::ceil(b.zmin / zPitch) * zPitch; z <= b.zmax + 1.0; z += zPitch) {
    const bool major = std::abs(std::fmod(std::abs(z), 900.0)) < 1.0;
    TLine line(z, b.xmin, z, b.xmax);
    line.SetLineColor(major ? majorColor : minorColor);
    line.SetLineStyle(major ? 1 : 3);
    line.SetLineWidth(1);
    line.DrawClone();
  }
  for (double x = std::ceil(b.xmin / xPitch) * xPitch; x <= b.xmax + 1.0; x += xPitch) {
    const bool major = std::abs(std::fmod(std::abs(x), 450.0)) < 1.0;
    TLine line(b.zmin, x, b.zmax, x);
    line.SetLineColor(major ? majorColor : minorColor);
    line.SetLineStyle(major ? 1 : 3);
    line.SetLineWidth(1);
    line.DrawClone();
  }
}

void DrawSideTicks(const Bounds3& b, int color) {
  const double zPitch = 600.0;
  const double xPitch = 150.0;
  const double xTick = 0.035 * (b.xmax - b.xmin);
  const double zTick = 0.012 * (b.zmax - b.zmin);
  for (double z = std::ceil(b.zmin / zPitch) * zPitch; z <= b.zmax + 1.0; z += zPitch) {
    TLine top(z, b.xmax + 0.45 * xTick, z, b.xmax + xTick);
    TLine bottom(z, b.xmin - 0.45 * xTick, z, b.xmin - xTick);
    for (auto* line : {&top, &bottom}) {
      line->SetLineColor(color);
      line->SetLineWidth(2);
      line->DrawClone();
    }
  }
  for (double x = std::ceil(b.xmin / xPitch) * xPitch; x <= b.xmax + 1.0; x += xPitch) {
    TLine left(b.zmin - 0.45 * zTick, x, b.zmin - zTick, x);
    TLine right(b.zmax + 0.45 * zTick, x, b.zmax + zTick, x);
    for (auto* line : {&left, &right}) {
      line->SetLineColor(color);
      line->SetLineWidth(2);
      line->DrawClone();
    }
  }
}

void DrawSideBoxFilled(const WireBox& box, int lineColor, int fillColor, double alpha, int width) {
  const Bounds3 b = BoundsOf(box);
  TBox fill(b.zmin, b.xmin, b.zmax, b.xmax);
  fill.SetLineColor(fillColor);
  fill.SetFillColorAlpha(fillColor, alpha);
  fill.DrawClone("f");

  TBox outline(b.zmin, b.xmin, b.zmax, b.xmax);
  outline.SetLineColor(lineColor);
  outline.SetLineWidth(width);
  outline.SetFillStyle(0);
  outline.DrawClone("l");
}

void DrawPlanActiveFill(const Bounds3& active, int fillColor, double alpha) {
  TBox fill(active.zmin, active.ymin, active.zmax, active.ymax);
  fill.SetLineColor(fillColor);
  fill.SetFillColorAlpha(fillColor, alpha);
  fill.DrawClone("f");
}

void DrawSideActiveFill(const Bounds3& active, int fillColor, double alpha) {
  TBox fill(active.zmin, active.xmin, active.zmax, active.xmax);
  fill.SetLineColor(fillColor);
  fill.SetFillColorAlpha(fillColor, alpha);
  fill.DrawClone("f");
}

void DrawPlaneX(const Bounds3& b, double x, int fillColor, double alpha, int lineColor, int lineWidth) {
  const std::array<Point3, 5> corners = {{
      {x, b.ymin, b.zmin},
      {x, b.ymax, b.zmin},
      {x, b.ymax, b.zmax},
      {x, b.ymin, b.zmax},
      {x, b.ymin, b.zmin},
  }};
  double us[5];
  double vs[5];
  for (int i = 0; i < 5; ++i) {
    const auto p = Project(corners[i]);
    us[i] = p.u;
    vs[i] = p.v;
  }
  TPolyLine poly(5, us, vs);
  poly.SetFillColorAlpha(fillColor, alpha);
  poly.SetLineColor(lineColor);
  poly.SetLineWidth(lineWidth);
  poly.DrawClone("f");
  poly.SetFillStyle(0);
  poly.DrawClone("l");
}

void DrawOblique(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const WireBox& cryostatBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes) {
  TCanvas canvas("dunevd_arapuca_oblique", "dunevd_arapuca_oblique", width, height);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  const Bounds3 cryo = BoundsOf(cryostatBox.valid ? cryostatBox : activeBox);
  Bounds2 projected;
  for (const auto& corner : activeBox.corners) Extend(projected, Project(corner));
  for (const auto& box : arapucaBoxes) {
    for (const auto& corner : box.corners) Extend(projected, Project(corner));
  }
  for (const auto& box : arapucaDoubleBoxes) {
    for (const auto& corner : box.corners) Extend(projected, Project(corner));
  }
  RangeWithAspect(projected, static_cast<double>(width) / height, 0.04);

  DrawPlaneX(active, active.xmin, Color("#ffffff"), 0.02, Color("#789b58"), 1);
  DrawPlaneX(active, 0.5 * (active.xmin + active.xmax), Color("#ffffff"), 0.02, Color("#a855f7"), 1);
  DrawPlaneX(active, active.xmax, Color("#ffffff"), 0.02, Color("#789b58"), 1);
  DrawObliqueSegmentationGrid(active, activeBoxes);
  for (const auto& box : arapucaBoxes) DrawWireBoxProjected(box, Color("#ff6f3c"), 3);
  for (const auto& box : arapucaDoubleBoxes) DrawWireBoxProjected(box, Color("#ff8c1a"), 3);
  DrawBoxEdges(cryo, Color("#b7c2cc"), 1, 2);
  DrawBoxEdges(active, Color("#2f7d5b"), 3);
  canvas.SaveAs(outPath);
}

void DrawComparisonBaseline(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const WireBox& cryostatBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes) {
  TCanvas canvas("dunevd_arapuca_comparison", "dunevd_arapuca_comparison", width, height);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  const Bounds3 cryo = BoundsOf(cryostatBox.valid ? cryostatBox : activeBox);
  Bounds2 projected;
  ExtendProjected(projected, active);
  RangeWithAspect(projected, static_cast<double>(width) / height, 0.04);

  DrawPlaneX(active, active.xmin, Color("#ffffff"), 0.02, Color("#789b58"), 1);
  DrawPlaneX(active, 0.5 * (active.xmin + active.xmax), Color("#ffffff"), 0.02, Color("#a855f7"), 1);
  DrawPlaneX(active, active.xmax, Color("#ffffff"), 0.02, Color("#789b58"), 1);
  DrawObliqueSegmentationGrid(active, activeBoxes);
  for (const auto& box : arapucaBoxes) DrawWireBoxProjected(box, Color("#ff6f3c"), 2);
  for (const auto& box : arapucaDoubleBoxes) DrawWireBoxProjected(box, Color("#ff8c1a"), 2);
  DrawBoxEdges(cryo, Color("#b7c2cc"), 1, 2);
  DrawBoxEdges(active, Color("#2f7d5b"), 3);

  canvas.SaveAs(outPath);
}

void DrawPlan(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes) {
  const int canvasHeight = std::min(height, 520);
  TCanvas canvas("dunevd_arapuca_yz", "dunevd_arapuca_yz", width, canvasHeight);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  Bounds2 b;
  Extend(b, {active.zmin, active.ymin});
  Extend(b, {active.zmax, active.ymax});
  RangeWithAspect(b, static_cast<double>(width) / canvasHeight, 0.035);

  DrawPlanActiveFill(active, Color("#ffffff"), 0.01);
  DrawPlanEngineeringGrid(active);
  for (const auto& box : activeBoxes) DrawPlanBox(box, Color("#7b57ff"), 1);
  for (const auto& box : arapucaBoxes) {
    if (CenterInsideYZ(box, active)) DrawPlanBoxFilled(box, Color("#ff6f3c"), Color("#ff9b55"), 0.045, 2);
  }
  for (const auto& box : arapucaDoubleBoxes) {
    if (CenterInsideYZ(box, active)) DrawPlanBoxFilled(box, Color("#ff8c1a"), Color("#ffd166"), 0.045, 2);
  }
  DrawPlanBox(activeBox, Color("#2f7d5b"), 3);
  canvas.SaveAs(outPath);
}

void DrawSide(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes) {
  const int canvasHeight = std::min(height, 420);
  TCanvas canvas("dunevd_arapuca_xz", "dunevd_arapuca_xz", width, canvasHeight);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  Bounds2 b;
  Extend(b, {active.zmin, active.xmin});
  Extend(b, {active.zmax, active.xmax});
  RangeWithAspect(b, static_cast<double>(width) / canvasHeight, 0.04);

  DrawSideActiveFill(active, Color("#ffffff"), 0.01);
  DrawSideEngineeringGrid(active);
  for (const auto& box : activeBoxes) DrawSideBox(box, Color("#7b57ff"), 1);
  for (const auto& box : arapucaBoxes) {
    if (CenterInsideXZ(box, active)) DrawSideBoxFilled(box, Color("#ff6f3c"), Color("#ff9b55"), 0.045, 2);
  }
  for (const auto& box : arapucaDoubleBoxes) {
    if (CenterInsideXZ(box, active)) DrawSideBoxFilled(box, Color("#ff8c1a"), Color("#ffd166"), 0.045, 2);
  }
  DrawSideBox(activeBox, Color("#2f7d5b"), 3);
  canvas.SaveAs(outPath);
}

}  // namespace

void render_dunevd_arapuca(
    const char* gdmlPath = "extern/dunecore/dunecore/Geometry/gdml/dunevd10kt_3view_30deg_v7_refactored_2x8x40_nowires.gdml",
    const char* outPrefix = "data/dunevd_arapuca_renders/dunevd_arapuca",
    int width = 1900,
    int height = 720) {
  gROOT->SetBatch(kTRUE);
  gStyle->SetOptStat(0);
  gErrorIgnoreLevel = kWarning;

  TGeoManager* geom = TGeoManager::Import(gdmlPath);
  if (!geom) {
    Error("render_dunevd_arapuca", "Could not import %s", gdmlPath);
    return;
  }

  const auto activeBox = CollectFirstBox(geom, "volEnclosureTPC");
  const auto cryostatBox = CollectFirstBox(geom, "volCryostat");
  const auto activeBoxes = CollectBoxes(geom, "volTPCActive");
  const auto arapucaBoxes = CollectBoxes(geom, "volArapuca");
  const auto arapucaDoubleBoxes = CollectBoxes(geom, "volArapucaDouble");

  if (!activeBox.valid || arapucaBoxes.empty()) {
    Error(
        "render_dunevd_arapuca",
        "Missing required geometry: active=%d arapuca=%zu double=%zu",
        activeBox.valid,
        arapucaBoxes.size(),
        arapucaDoubleBoxes.size());
    return;
  }

  gSystem->mkdir(gSystem->DirName(outPrefix), kTRUE);

  TString oblique = TString::Format("%s_oblique.png", outPrefix);
  TString plan = TString::Format("%s_yz_plan.png", outPrefix);
  TString side = TString::Format("%s_xz_side.png", outPrefix);
  TString comparison = TString::Format("%s_comparison_baseline.png", outPrefix);

  printf(
      "Rendering baseline DUNE VD with %zu active regions, %zu ARAPUCA boxes, %zu double-ARAPUCA boxes\n",
      activeBoxes.size(),
      arapucaBoxes.size(),
      arapucaDoubleBoxes.size());

  DrawOblique(oblique.Data(), width, height, activeBox, cryostatBox, activeBoxes, arapucaBoxes, arapucaDoubleBoxes);
  DrawPlan(plan.Data(), width, height, activeBox, activeBoxes, arapucaBoxes, arapucaDoubleBoxes);
  DrawSide(side.Data(), width, height, activeBox, activeBoxes, arapucaBoxes, arapucaDoubleBoxes);
  DrawComparisonBaseline(
      comparison.Data(),
      width,
      height,
      activeBox,
      cryostatBox,
      activeBoxes,
      arapucaBoxes,
      arapucaDoubleBoxes);
}

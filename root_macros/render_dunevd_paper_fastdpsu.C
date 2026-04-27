#include <TCanvas.h>
#include <TBox.h>
#include <TColor.h>
#include <TError.h>
#include <TGeoBBox.h>
#include <TGeoManager.h>
#include <TGeoMatrix.h>
#include <TGeoNode.h>
#include <TGeoVolume.h>
#include <TGraph.h>
#include <TLine.h>
#include <TObjArray.h>
#include <TPad.h>
#include <TPolyLine.h>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TSystem.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <set>
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

struct Segment {
  Point3 a;
  Point3 b;
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
  return {p.z + 0.30 * p.y, p.x - 0.17 * p.y};
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

bool PointInside(const Point3& point, const Bounds3& region, double tolerance = 1.0) {
  return point.x >= region.xmin - tolerance && point.x <= region.xmax + tolerance &&
         point.y >= region.ymin - tolerance && point.y <= region.ymax + tolerance &&
         point.z >= region.zmin - tolerance && point.z <= region.zmax + tolerance;
}

bool PointInsideYZ(const Point3& point, const Bounds3& region, double tolerance = 1.0) {
  return point.y >= region.ymin - tolerance && point.y <= region.ymax + tolerance &&
         point.z >= region.zmin - tolerance && point.z <= region.zmax + tolerance;
}

bool SegmentInsideYZ(const Segment& segment, const Bounds3& region, double tolerance = 1.0) {
  const Point3 c = {
      0.5 * (segment.a.x + segment.b.x),
      0.5 * (segment.a.y + segment.b.y),
      0.5 * (segment.a.z + segment.b.z),
  };
  return PointInsideYZ(c, region, tolerance);
}

bool BoxCenterInside(const WireBox& box, const Bounds3& region, double tolerance = 1.0) {
  return PointInside(CenterOf(box), region, tolerance);
}

std::vector<WireBox> SelectBoxes(const std::vector<WireBox>& boxes, const Bounds3& region, double tolerance = 1.0) {
  std::vector<WireBox> selected;
  for (const auto& box : boxes) {
    if (BoxCenterInside(box, region, tolerance)) selected.push_back(box);
  }
  return selected;
}

std::vector<WireBox> SelectBoxesYZ(const std::vector<WireBox>& boxes, const Bounds3& region, double tolerance = 1.0) {
  std::vector<WireBox> selected;
  for (const auto& box : boxes) {
    const Point3 c = CenterOf(box);
    if (PointInsideYZ(c, region, tolerance)) selected.push_back(box);
  }
  return selected;
}

std::vector<WireBox> SelectBoxesXZ(const std::vector<WireBox>& boxes, const Bounds3& region, double tolerance = 1.0) {
  std::vector<WireBox> selected;
  for (const auto& box : boxes) {
    const Point3 c = CenterOf(box);
    if (c.x >= region.xmin - tolerance && c.x <= region.xmax + tolerance &&
        c.z >= region.zmin - tolerance && c.z <= region.zmax + tolerance) {
      selected.push_back(box);
    }
  }
  return selected;
}

std::vector<WireBox> SelectBoxesXZOverlap(
    const std::vector<WireBox>& boxes,
    const Bounds3& region,
    double tolerance = 1.0) {
  std::vector<WireBox> selected;
  for (const auto& box : boxes) {
    const Bounds3 b = BoundsOf(box);
    const bool xOverlap = b.xmax >= region.xmin - tolerance && b.xmin <= region.xmax + tolerance;
    const bool zOverlap = b.zmax >= region.zmin - tolerance && b.zmin <= region.zmax + tolerance;
    if (xOverlap && zOverlap) selected.push_back(box);
  }
  return selected;
}

std::vector<Segment> SelectStringsYZ(const std::vector<Segment>& strings, const Bounds3& region, double tolerance = 1.0) {
  std::vector<Segment> selected;
  for (const auto& segment : strings) {
    if (SegmentInsideYZ(segment, region, tolerance)) selected.push_back(segment);
  }
  return selected;
}

std::vector<Point3> SelectPoints(const std::vector<Point3>& points, const Bounds3& region, double tolerance = 1.0) {
  std::vector<Point3> selected;
  for (const auto& point : points) {
    if (PointInside(point, region, tolerance)) selected.push_back(point);
  }
  return selected;
}

std::vector<Point3> SelectPointsYZ(const std::vector<Point3>& points, const Bounds3& region, double tolerance = 1.0) {
  std::vector<Point3> selected;
  for (const auto& point : points) {
    if (PointInsideYZ(point, region, tolerance)) selected.push_back(point);
  }
  return selected;
}

void RangeWithAspect(const Bounds2& raw, double aspect, double margin = 0.08) {
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

std::vector<Segment> CollectSegments(TGeoManager* geom, const char* volumeName) {
  std::vector<Segment> strings;
  TGeoVolume* top = geom->GetTopVolume();
  auto* stringVolume = geom->FindVolumeFast(volumeName);
  auto* stringBox = Box(stringVolume);
  if (!top || !stringBox) return strings;

  const double localA[3] = {-stringBox->GetDX(), 0.0, 0.0};
  const double localB[3] = { stringBox->GetDX(), 0.0, 0.0};

  TGeoIterator iterator(top);
  TGeoNode* node = nullptr;
  while ((node = iterator())) {
    TGeoVolume* volume = node->GetVolume();
    if (!volume || !Eq(volume->GetName(), volumeName)) continue;
    const TGeoMatrix* matrix = iterator.GetCurrentMatrix();
    if (!matrix) continue;
    double a[3] = {0.0, 0.0, 0.0};
    double b[3] = {0.0, 0.0, 0.0};
    matrix->LocalToMaster(localA, a);
    matrix->LocalToMaster(localB, b);
    strings.push_back({{a[0], a[1], a[2]}, {b[0], b[1], b[2]}});
  }
  return strings;
}

std::vector<Segment> CollectStrings(TGeoManager* geom) {
  auto strings = CollectSegments(geom, "volFastDPSUStringCoreCable");
  if (strings.empty()) strings = CollectSegments(geom, "volFastDPSUString");
  return strings;
}

std::vector<Point3> CollectCenters(TGeoManager* geom, const char* volumeName) {
  std::vector<Point3> points;
  TGeoVolume* top = geom->GetTopVolume();
  if (!top) return points;

  const double local[3] = {0.0, 0.0, 0.0};
  TGeoIterator iterator(top);
  TGeoNode* node = nullptr;
  while ((node = iterator())) {
    TGeoVolume* volume = node->GetVolume();
    if (!volume || !Eq(volume->GetName(), volumeName)) continue;
    const TGeoMatrix* matrix = iterator.GetCurrentMatrix();
    if (!matrix) continue;
    double master[3] = {0.0, 0.0, 0.0};
    matrix->LocalToMaster(local, master);
    points.push_back({master[0], master[1], master[2]});
  }
  return points;
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

void DrawWireBox(const WireBox& box, int color, int width, int style = 1) {
  if (!box.valid) return;
  static const int edges[12][2] = {
      {0, 1}, {0, 2}, {0, 4}, {3, 1}, {3, 2}, {3, 7},
      {5, 1}, {5, 4}, {5, 7}, {6, 2}, {6, 4}, {6, 7},
  };
  for (const auto& edge : edges) {
    DrawLine2(Project(box.corners[edge[0]]), Project(box.corners[edge[1]]), color, width, style);
  }
}

void DrawWireBoxes(const std::vector<WireBox>& boxes, int color, int width, int style = 1) {
  for (const auto& box : boxes) {
    DrawWireBox(box, color, width, style);
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

void DrawProjectedPoints(const std::vector<Point3>& points, int color, int marker, double size) {
  if (points.empty()) return;
  std::vector<double> us;
  std::vector<double> vs;
  us.reserve(points.size());
  vs.reserve(points.size());
  for (const auto& point : points) {
    const auto p = Project(point);
    us.push_back(p.u);
    vs.push_back(p.v);
  }
  TGraph graph(static_cast<int>(us.size()), us.data(), vs.data());
  graph.SetMarkerColor(color);
  graph.SetMarkerStyle(marker);
  graph.SetMarkerSize(size);
  graph.DrawClone("P");
}

void DrawPlanRect(const Bounds3& b, int lineColor, int fillColor, double alpha, int width) {
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

void DrawPlanOutline(const Bounds3& b, int color, int width, int style = 1) {
  TLine bottom(b.zmin, b.ymin, b.zmax, b.ymin);
  TLine top(b.zmin, b.ymax, b.zmax, b.ymax);
  TLine left(b.zmin, b.ymin, b.zmin, b.ymax);
  TLine right(b.zmax, b.ymin, b.zmax, b.ymax);
  for (auto* line : {&bottom, &top, &left, &right}) {
    line->SetLineColor(color);
    line->SetLineWidth(width);
    line->SetLineStyle(style);
    line->DrawClone();
  }
}

void DrawPlanEngineeringGrid(const Bounds3& b) {
  const int minorColor = Color("#f0ebff");
  const int majorColor = Color("#7b57ff");
  const double pitch = 300.0;
  const double firstZ = std::ceil(b.zmin / pitch) * pitch;
  for (double z = firstZ; z <= b.zmax + 1.0; z += pitch) {
    const bool major = std::abs(std::fmod(std::abs(z), 900.0)) < 1.0;
    TLine line(z, b.ymin, z, b.ymax);
    line.SetLineColor(major ? majorColor : minorColor);
    line.SetLineWidth(1);
    line.SetLineStyle(major ? 1 : 3);
    line.DrawClone();
  }

  const double firstY = std::ceil(b.ymin / pitch) * pitch;
  for (double y = firstY; y <= b.ymax + 1.0; y += pitch) {
    const bool major = std::abs(std::fmod(std::abs(y), 900.0)) < 1.0;
    TLine line(b.zmin, y, b.zmax, y);
    line.SetLineColor(major ? majorColor : minorColor);
    line.SetLineWidth(1);
    line.SetLineStyle(major ? 1 : 3);
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

void DrawPlanMarkers(const std::vector<Segment>& strings, int haloColor, int markerColor) {
  std::vector<double> zs;
  std::vector<double> ys;
  zs.reserve(strings.size());
  ys.reserve(strings.size());
  for (const auto& segment : strings) {
    zs.push_back(0.5 * (segment.a.z + segment.b.z));
    ys.push_back(0.5 * (segment.a.y + segment.b.y));
  }

  TGraph halo(static_cast<int>(zs.size()), zs.data(), ys.data());
  halo.SetMarkerStyle(20);
  halo.SetMarkerColor(haloColor);
  halo.SetMarkerSize(0.82);
  halo.DrawClone("P");

  TGraph graph(static_cast<int>(zs.size()), zs.data(), ys.data());
  graph.SetMarkerStyle(20);
  graph.SetMarkerColor(markerColor);
  graph.SetMarkerSize(0.48);
  graph.DrawClone("P");
}

std::vector<double> UniqueStringZs(const std::vector<Segment>& strings) {
  std::vector<double> zs;
  for (const auto& segment : strings) {
    AddUnique(zs, 0.5 * (segment.a.z + segment.b.z), 1.0);
  }
  std::sort(zs.begin(), zs.end());
  return zs;
}

std::vector<double> UniqueNodeXs(const std::vector<Point3>& nodes) {
  std::vector<double> xs;
  for (const auto& node : nodes) {
    AddUnique(xs, node.x, 1.0);
  }
  std::sort(xs.begin(), xs.end());
  return xs;
}

std::vector<double> SelectEvery(const std::vector<double>& values, int stride) {
  std::vector<double> selected;
  if (values.empty()) return selected;
  stride = std::max(stride, 1);
  const int offset = stride > 1 ? stride / 2 : 0;
  for (int i = offset; i < static_cast<int>(values.size()); i += stride) selected.push_back(values[i]);
  if (selected.empty()) selected.push_back(values[values.size() / 2]);
  return selected;
}

std::vector<double> DensifyValues(const std::vector<double>& values) {
  std::vector<double> dense;
  if (values.empty()) return dense;
  for (size_t i = 0; i < values.size(); ++i) {
    dense.push_back(values[i]);
    if (i + 1 < values.size()) dense.push_back(0.5 * (values[i] + values[i + 1]));
  }
  return dense;
}

bool NearAny(double value, const std::vector<double>& anchors, double tolerance) {
  for (const double anchor : anchors) {
    if (std::abs(value - anchor) <= tolerance) return true;
  }
  return false;
}

void DrawOblique(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const WireBox& cryostatBox,
    const std::vector<WireBox>& tpcBoxes,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& anodeBoxes,
    const std::vector<WireBox>& anodeBottomBoxes,
    const std::vector<WireBox>& fieldShaperBoxes,
    const std::vector<WireBox>& fieldShaperSlimBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes,
    const std::vector<Segment>& strings,
    const std::vector<Point3>& nodes) {
  TCanvas canvas("paper_fastdpsu_oblique", "paper_fastdpsu_oblique", width, height);
  PrepareCanvas(&canvas);

  const int activeLine = Color("#2f7d5b");
  const int cryoLine = Color("#b7c2cc");
  const int planeLine = Color("#789b58");
  const int centerLine = Color("#a855f7");
  const int stringColor = Color("#9a5a16");
  const int nodeColor = Color("#2b6f91");

  const Bounds3 active = BoundsOf(activeBox);
  const Bounds3 cryo = BoundsOf(cryostatBox.valid ? cryostatBox : activeBox);

  Bounds2 projected;
  for (const auto& corner : activeBox.corners) Extend(projected, Project(corner));
  for (const auto& segment : strings) {
    Extend(projected, Project(segment.a));
    Extend(projected, Project(segment.b));
  }
  RangeWithAspect(projected, static_cast<double>(width) / height, 0.04);

  DrawPlaneX(active, active.xmin, Color("#ffffff"), 0.02, planeLine, 1);
  DrawPlaneX(active, 0.5 * (active.xmin + active.xmax), Color("#b8bcc2"), 0.16, centerLine, 1);
  DrawPlaneX(active, active.xmax, Color("#ffffff"), 0.02, planeLine, 1);
  DrawObliqueSegmentationGrid(active, activeBoxes);

  for (const auto& segment : strings) {
    DrawLine2(Project(segment.a), Project(segment.b), stringColor, 1, 1);
  }
  DrawProjectedPoints(nodes, nodeColor, 20, 0.34);
  DrawBoxEdges(cryo, cryoLine, 1, 2);
  DrawBoxEdges(active, activeLine, 3);

  canvas.SaveAs(outPath);
}

void DrawProjectedArapucas(
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes,
    int lineWidth = 2) {
  for (const auto& box : arapucaBoxes) DrawWireBox(box, Color("#ff6f3c"), lineWidth);
  for (const auto& box : arapucaDoubleBoxes) DrawWireBox(box, Color("#ff8c1a"), lineWidth);
}

void DrawObliqueLocalGrid(const Bounds3& region, const std::vector<WireBox>& boxes, int color) {
  DrawBoxEdges(region, Color("#2f7d5b"), 3);
  for (const auto& box : boxes) DrawWireBox(box, color, 1);
  const double centerX = 0.5 * (region.xmin + region.xmax);
  DrawPlaneX(region, centerX, Color("#ffffff"), 0.015, Color("#a855f7"), 1);
}

void DrawSingleModuleCutaway(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const WireBox& cryostatBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes,
    const std::vector<Segment>& strings,
    const std::vector<Point3>& nodes) {
  const int canvasWidth = std::min(width, 1200);
  const int canvasHeight = height;
  TCanvas canvas("paper_fastdpsu_single_module", "paper_fastdpsu_single_module", canvasWidth, canvasHeight);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  const double centerY = 0.5 * (active.ymin + active.ymax);
  const double centerZ = 0.5 * (active.zmin + active.zmax);
  Bounds3 region = active;
  region.ymin = centerY - 255.0;
  region.ymax = centerY + 255.0;
  region.zmin = centerZ - 390.0;
  region.zmax = centerZ + 390.0;

  const auto localActive = SelectBoxesYZ(activeBoxes, region, 120.0);
  const auto localArapuca = SelectBoxesYZ(arapucaBoxes, region, 80.0);
  const auto localArapucaDouble = SelectBoxesYZ(arapucaDoubleBoxes, region, 80.0);
  const auto localStrings = SelectStringsYZ(strings, region, 80.0);
  const auto localNodes = SelectPointsYZ(nodes, region, 80.0);

  Bounds2 projected;
  ExtendProjected(projected, region);
  for (const auto& segment : localStrings) {
    Extend(projected, Project(segment.a));
    Extend(projected, Project(segment.b));
  }
  RangeWithAspect(projected, static_cast<double>(canvasWidth) / canvasHeight, 0.12);

  DrawObliqueLocalGrid(region, localActive, Color("#7b57ff"));
  DrawProjectedArapucas(localArapuca, localArapucaDouble, 2);
  for (const auto& segment : localStrings) {
    DrawLine2(Project(segment.a), Project(segment.b), Color("#9a5a16"), 2);
  }
  DrawProjectedPoints(localNodes, Color("#2b6f91"), 20, 0.62);

  canvas.SaveAs(outPath);
}

void DrawHalfDetectorCutaway(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const WireBox& cryostatBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes,
    const std::vector<Segment>& strings,
    const std::vector<Point3>& nodes) {
  TCanvas canvas("paper_fastdpsu_half_cutaway", "paper_fastdpsu_half_cutaway", width, height);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  const Bounds3 cryo = BoundsOf(cryostatBox.valid ? cryostatBox : activeBox);
  Bounds3 region = active;
  region.ymin = 0.5 * (active.ymin + active.ymax);

  const auto localActive = SelectBoxes(activeBoxes, region, 5.0);
  const auto localArapuca = SelectBoxes(arapucaBoxes, region, 5.0);
  const auto localArapucaDouble = SelectBoxes(arapucaDoubleBoxes, region, 5.0);
  const auto localStrings = SelectStringsYZ(strings, region, 5.0);
  const auto localNodes = SelectPoints(nodes, region, 5.0);

  Bounds2 projected;
  ExtendProjected(projected, active);
  for (const auto& segment : localStrings) {
    Extend(projected, Project(segment.a));
    Extend(projected, Project(segment.b));
  }
  RangeWithAspect(projected, static_cast<double>(width) / height, 0.05);

  DrawBoxEdges(cryo, Color("#c8d2dc"), 1, 2);
  DrawPlaneX(region, region.xmin, Color("#ffffff"), 0.018, Color("#789b58"), 1);
  DrawPlaneX(region, 0.5 * (region.xmin + region.xmax), Color("#b8bcc2"), 0.14, Color("#a855f7"), 1);
  DrawObliqueLocalGrid(region, localActive, Color("#7b57ff"));
  DrawProjectedArapucas(localArapuca, localArapucaDouble, 2);
  for (const auto& segment : localStrings) {
    DrawLine2(Project(segment.a), Project(segment.b), Color("#9a5a16"), 1);
  }
  DrawProjectedPoints(localNodes, Color("#2b6f91"), 20, 0.38);
  DrawBoxEdges(active, Color("#b7c2cc"), 1, 2);

  canvas.SaveAs(outPath);
}

void DrawFastDPSUComparisonOverlay(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const WireBox& cryostatBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes,
    const std::vector<Segment>& strings,
    const std::vector<Point3>& nodes) {
  TCanvas canvas("paper_fastdpsu_comparison", "paper_fastdpsu_comparison", width, height);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  const Bounds3 cryo = BoundsOf(cryostatBox.valid ? cryostatBox : activeBox);
  Bounds2 projected;
  ExtendProjected(projected, active);
  RangeWithAspect(projected, static_cast<double>(width) / height, 0.04);

  DrawPlaneX(active, active.xmin, Color("#ffffff"), 0.02, Color("#789b58"), 1);
  DrawPlaneX(active, 0.5 * (active.xmin + active.xmax), Color("#b8bcc2"), 0.16, Color("#a855f7"), 1);
  DrawPlaneX(active, active.xmax, Color("#ffffff"), 0.02, Color("#789b58"), 1);
  DrawObliqueSegmentationGrid(active, activeBoxes);
  DrawProjectedArapucas(arapucaBoxes, arapucaDoubleBoxes, 2);
  for (const auto& segment : strings) {
    DrawLine2(Project(segment.a), Project(segment.b), Color("#9a5a16"), 1);
  }
  DrawProjectedPoints(nodes, Color("#2b6f91"), 20, 0.32);
  DrawBoxEdges(cryo, Color("#b7c2cc"), 1, 2);
  DrawBoxEdges(active, Color("#2f7d5b"), 3);

  canvas.SaveAs(outPath);
}

double NearestStringZ(const std::vector<Segment>& strings, double targetZ) {
  double nearest = targetZ;
  double best = std::numeric_limits<double>::max();
  for (const auto& segment : strings) {
    const double z = 0.5 * (segment.a.z + segment.b.z);
    const double distance = std::abs(z - targetZ);
    if (distance < best) {
      best = distance;
      nearest = z;
    }
  }
  return nearest;
}

void DrawEndOnDanglingSlice(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const std::vector<Segment>& strings,
    const std::vector<Point3>& nodes) {
  const int canvasWidth = std::min(width, 1050);
  const int canvasHeight = std::min(std::max(height, 780), 900);
  TCanvas canvas("paper_fastdpsu_end_on", "paper_fastdpsu_end_on", canvasWidth, canvasHeight);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  const double z0 = NearestStringZ(strings, 0.5 * (active.zmin + active.zmax));
  const double zTolerance = 55.0;

  Bounds2 b;
  Extend(b, {active.ymin, active.xmin});
  Extend(b, {active.ymax, active.xmax});
  RangeWithAspect(b, static_cast<double>(canvasWidth) / canvasHeight, 0.10);

  TBox fill(active.ymin, active.xmin, active.ymax, active.xmax);
  fill.SetLineColor(Color("#ffffff"));
  fill.SetFillColorAlpha(Color("#ffffff"), 0.01);
  fill.DrawClone("f");

  const int minorGrid = Color("#f0ebff");
  const int majorGrid = Color("#7b57ff");
  for (double y = std::ceil(active.ymin / 150.0) * 150.0; y <= active.ymax + 1.0; y += 150.0) {
    const bool major = std::abs(std::fmod(std::abs(y), 450.0)) < 1.0;
    TLine line(y, active.xmin, y, active.xmax);
    line.SetLineColor(major ? majorGrid : minorGrid);
    line.SetLineStyle(major ? 1 : 3);
    line.SetLineWidth(1);
    line.DrawClone();
  }
  for (double x = std::ceil(active.xmin / 150.0) * 150.0; x <= active.xmax + 1.0; x += 150.0) {
    const bool major = std::abs(std::fmod(std::abs(x), 450.0)) < 1.0;
    TLine line(active.ymin, x, active.ymax, x);
    line.SetLineColor(major ? majorGrid : minorGrid);
    line.SetLineStyle(major ? 1 : 3);
    line.SetLineWidth(1);
    line.DrawClone();
  }

  for (const auto& segment : strings) {
    const double z = 0.5 * (segment.a.z + segment.b.z);
    if (std::abs(z - z0) > zTolerance) continue;
    TLine line(segment.a.y, segment.a.x, segment.b.y, segment.b.x);
    line.SetLineColor(Color("#9a5a16"));
    line.SetLineWidth(2);
    line.DrawClone();
  }

  std::vector<double> ys;
  std::vector<double> xs;
  for (const auto& node : nodes) {
    if (std::abs(node.z - z0) > zTolerance) continue;
    ys.push_back(node.y);
    xs.push_back(node.x);
  }
  if (!ys.empty()) {
    TGraph halo(static_cast<int>(ys.size()), ys.data(), xs.data());
    halo.SetMarkerStyle(20);
    halo.SetMarkerColor(Color("#d9edf7"));
    halo.SetMarkerSize(1.10);
    halo.DrawClone("P");

    TGraph graph(static_cast<int>(ys.size()), ys.data(), xs.data());
    graph.SetMarkerStyle(20);
    graph.SetMarkerColor(Color("#2b6f91"));
    graph.SetMarkerSize(0.68);
    graph.DrawClone("P");
  }

  TBox outline(active.ymin, active.xmin, active.ymax, active.xmax);
  outline.SetLineColor(Color("#2f7d5b"));
  outline.SetLineWidth(4);
  outline.SetFillStyle(0);
  outline.DrawClone("l");

  canvas.SaveAs(outPath);
}

void RangeXZ(const Bounds3& active, int width, int height) {
  Bounds2 b;
  Extend(b, {active.zmin, active.xmin});
  Extend(b, {active.zmax, active.xmax});
  RangeWithAspect(b, static_cast<double>(width) / height, 0.08);
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

void DrawSideOutline(const Bounds3& b, int color, int width, int style = 1) {
  TLine bottom(b.zmin, b.xmin, b.zmax, b.xmin);
  TLine top(b.zmin, b.xmax, b.zmax, b.xmax);
  TLine left(b.zmin, b.xmin, b.zmin, b.xmax);
  TLine right(b.zmax, b.xmin, b.zmax, b.xmax);
  for (auto* line : {&bottom, &top, &left, &right}) {
    line->SetLineColor(color);
    line->SetLineWidth(width);
    line->SetLineStyle(style);
    line->DrawClone();
  }
}

void DrawSideBox(const Bounds3& b, int lineColor, int fillColor, double alpha, int width, int style = 1) {
  TBox fill(b.zmin, b.xmin, b.zmax, b.xmax);
  fill.SetLineColor(fillColor);
  fill.SetFillColorAlpha(fillColor, alpha);
  fill.DrawClone("f");
  DrawSideOutline(b, lineColor, width, style);
}

void DrawSideCentralBand(const Bounds3& b, double halfThickness, int fillColor, int lineColor) {
  const double centerX = 0.5 * (b.xmin + b.xmax);
  TBox band(b.zmin, centerX - halfThickness, b.zmax, centerX + halfThickness);
  band.SetLineColor(fillColor);
  band.SetFillColorAlpha(fillColor, 0.14);
  band.DrawClone("f");

  TLine mid(b.zmin, centerX, b.zmax, centerX);
  mid.SetLineColor(lineColor);
  mid.SetLineWidth(2);
  mid.DrawClone();
}

void DrawSideBoxes(
    const std::vector<WireBox>& boxes,
    int lineColor,
    int fillColor,
    double alpha,
    int width,
    int style = 1) {
  for (const auto& box : boxes) DrawSideBox(BoundsOf(box), lineColor, fillColor, alpha, width, style);
}

void DrawLocalSideGrid(const Bounds3& b) {
  const int minorColor = Color("#f0ebff");
  const int majorColor = Color("#7b57ff");
  const double zPitch = 1.0;
  const double xPitch = 1.0;

  for (double z = std::ceil(b.zmin / zPitch) * zPitch; z <= b.zmax + 0.001; z += zPitch) {
    const bool major = std::abs(std::fmod(std::abs(z), 5.0)) < 0.001;
    TLine line(z, b.xmin, z, b.xmax);
    line.SetLineColor(major ? majorColor : minorColor);
    line.SetLineStyle(major ? 1 : 3);
    line.SetLineWidth(1);
    line.DrawClone();
  }
  for (double x = std::ceil(b.xmin / xPitch) * xPitch; x <= b.xmax + 0.001; x += xPitch) {
    const bool major = std::abs(std::fmod(std::abs(x), 5.0)) < 0.001;
    TLine line(b.zmin, x, b.zmax, x);
    line.SetLineColor(major ? majorColor : minorColor);
    line.SetLineStyle(major ? 1 : 3);
    line.SetLineWidth(1);
    line.DrawClone();
  }
}

void DrawFastDPSUDetailBoxes(
    const Bounds3& view,
    const std::vector<WireBox>& cableBoxes,
    const std::vector<WireBox>& fiberBoxes,
    const std::vector<WireBox>& /* housingBoxes */,
    const std::vector<WireBox>& carrierBoxes,
    const std::vector<WireBox>& boardBoxes,
    const std::vector<WireBox>& sensorBoxes,
    const std::vector<WireBox>& couplerBoxes,
    const std::vector<WireBox>& clampBoxes) {
  DrawSideBoxes(SelectBoxesXZOverlap(cableBoxes, view, 5.0), Color("#9a5a16"), Color("#d58b3a"), 0.18, 2);
  DrawSideBoxes(SelectBoxesXZOverlap(fiberBoxes, view, 5.0), Color("#0e7490"), Color("#a5f3fc"), 0.22, 2);
  DrawSideBoxes(SelectBoxesXZOverlap(carrierBoxes, view, 5.0), Color("#2f7d5b"), Color("#d9fbe8"), 0.16, 2);
  DrawSideBoxes(SelectBoxesXZOverlap(clampBoxes, view, 5.0), Color("#5b6472"), Color("#c8d2dc"), 0.18, 2);
  DrawSideBoxes(SelectBoxesXZOverlap(boardBoxes, view, 5.0), Color("#7b57ff"), Color("#eee7ff"), 0.22, 2);
  DrawSideBoxes(SelectBoxesXZOverlap(couplerBoxes, view, 5.0), Color("#ff8c1a"), Color("#ffd166"), 0.35, 2);
  DrawSideBoxes(SelectBoxesXZOverlap(sensorBoxes, view, 5.0), Color("#155e75"), Color("#2b6f91"), 0.38, 2);
}

void DrawXZ(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& anodeBoxes,
    const std::vector<WireBox>& anodeBottomBoxes,
    const std::vector<Segment>& strings,
    const std::vector<Point3>& nodes) {
  const int canvasHeight = std::min(height, 420);
  TCanvas canvas("paper_fastdpsu_xz", "paper_fastdpsu_xz", width, canvasHeight);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  const int activeFill = Color("#ffffff");
  const int stringColor = Color("#9a5a16");
  const int nodeColor = Color("#2b6f91");
  const int activeLine = Color("#2f7d5b");
  const int tpcActiveLine = Color("#7b57ff");
  const int anodeLine = Color("#ff6b4a");
  const int cathodeLine = Color("#a855f7");
  RangeXZ(active, width, canvasHeight);

  TBox lar(active.zmin, active.xmin, active.zmax, active.xmax);
  lar.SetLineColor(activeFill);
  lar.SetFillColorAlpha(activeFill, 0.01);
  lar.DrawClone("f");
  DrawSideEngineeringGrid(active);

  for (const double z : UniqueEdges(activeBoxes, 'z')) {
    TLine line(z, active.xmin, z, active.xmax);
    line.SetLineColor(tpcActiveLine);
    line.SetLineWidth(1);
    line.DrawClone();
  }
  for (const double x : UniqueEdges(activeBoxes, 'x')) {
    TLine line(active.zmin, x, active.zmax, x);
    line.SetLineColor(tpcActiveLine);
    line.SetLineWidth(1);
    line.DrawClone();
  }

  for (const auto& collection : {&anodeBoxes, &anodeBottomBoxes}) {
    for (const auto& box : *collection) {
      const Bounds3 b = BoundsOf(box);
      TLine line(b.zmin, 0.5 * (b.xmin + b.xmax), b.zmax, 0.5 * (b.xmin + b.xmax));
      line.SetLineColor(anodeLine);
      line.SetLineWidth(3);
      line.DrawClone();
    }
  }

  DrawSideCentralBand(active, 22.0, Color("#b8bcc2"), cathodeLine);

  std::set<int> drawnZ;
  for (const auto& segment : strings) {
    const int zKey = static_cast<int>(std::lround(0.5 * (segment.a.z + segment.b.z)));
    const bool firstAtZ = drawnZ.insert(zKey).second;
    if (!firstAtZ) continue;
    TLine line(segment.a.z, segment.a.x, segment.b.z, segment.b.x);
    line.SetLineColor(stringColor);
    line.SetLineWidth(1);
    line.DrawClone();
  }

  std::vector<double> zs;
  std::vector<double> xs;
  std::set<long long> uniqueNodeKeys;
  zs.reserve(320);
  xs.reserve(320);
  for (const auto& node : nodes) {
    const auto zKey = static_cast<long long>(std::llround(node.z * 10.0));
    const auto xKey = static_cast<long long>(std::llround(node.x * 10.0));
    const long long key = (zKey + 1000000LL) * 2000000LL + (xKey + 1000000LL);
    if (!uniqueNodeKeys.insert(key).second) continue;
    zs.push_back(node.z);
    xs.push_back(node.x);
  }

  TGraph halo(static_cast<int>(zs.size()), zs.data(), xs.data());
  halo.SetMarkerStyle(20);
  halo.SetMarkerColor(Color("#d9edf7"));
  halo.SetMarkerSize(0.90);
  halo.DrawClone("P");

  TGraph graph(static_cast<int>(zs.size()), zs.data(), xs.data());
  graph.SetMarkerStyle(20);
  graph.SetMarkerColor(nodeColor);
  graph.SetMarkerSize(0.50);
  graph.DrawClone("P");

  TLine top(active.zmin, active.xmax, active.zmax, active.xmax);
  top.SetLineColor(activeLine);
  top.SetLineWidth(4);
  top.DrawClone();
  TLine bottom(active.zmin, active.xmin, active.zmax, active.xmin);
  bottom.SetLineColor(activeLine);
  bottom.SetLineWidth(4);
  bottom.DrawClone();
  TLine left(active.zmin, active.xmin, active.zmin, active.xmax);
  TLine right(active.zmax, active.xmin, active.zmax, active.xmax);
  left.SetLineColor(activeLine);
  right.SetLineColor(activeLine);
  left.SetLineWidth(3);
  right.SetLineWidth(3);
  left.DrawClone();
  right.DrawClone();

  canvas.SaveAs(outPath);
}

void DrawYZ(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes,
    const std::vector<Segment>& strings) {
  const int canvasHeight = std::min(height, 520);
  TCanvas canvas("paper_fastdpsu_yz", "paper_fastdpsu_yz", width, canvasHeight);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  Bounds2 b;
  Extend(b, {active.zmin, active.ymin});
  Extend(b, {active.zmax, active.ymax});
  RangeWithAspect(b, static_cast<double>(width) / canvasHeight, 0.035);

  const int activeLine = Color("#2f7d5b");
  const int tpcActiveLine = Color("#7b57ff");
  const int pdsLine = Color("#ff6f3c");
  const int pdsFill = Color("#ff9b55");
  const int pdsDoubleLine = Color("#ff8c1a");
  const int pdsDoubleFill = Color("#ffd166");
  const int stringColor = Color("#2b6f91");

  TBox activeFill(active.zmin, active.ymin, active.zmax, active.ymax);
  activeFill.SetLineColor(Color("#ffffff"));
  activeFill.SetFillColorAlpha(Color("#ffffff"), 0.01);
  activeFill.DrawClone("f");
  DrawPlanEngineeringGrid(active);

  for (const auto& box : activeBoxes) {
    const Bounds3 tpc = BoundsOf(box);
    DrawPlanOutline(tpc, tpcActiveLine, 1);
  }

  for (const auto& box : arapucaBoxes) {
    if (CenterInsideYZ(box, active)) DrawPlanRect(BoundsOf(box), pdsLine, pdsFill, 0.045, 2);
  }
  for (const auto& box : arapucaDoubleBoxes) {
    if (CenterInsideYZ(box, active)) DrawPlanRect(BoundsOf(box), pdsDoubleLine, pdsDoubleFill, 0.045, 2);
  }

  DrawPlanMarkers(strings, Color("#d9edf7"), stringColor);
  DrawPlanOutline(active, activeLine, 3);

  canvas.SaveAs(outPath);
}

void DrawBaselineSameCamera(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const WireBox& cryostatBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes) {
  TCanvas canvas("paper_fastdpsu_baseline_same_camera", "paper_fastdpsu_baseline_same_camera", width, height);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  const Bounds3 cryo = BoundsOf(cryostatBox.valid ? cryostatBox : activeBox);
  Bounds2 projected;
  ExtendProjected(projected, active);
  RangeWithAspect(projected, static_cast<double>(width) / height, 0.04);

  DrawPlaneX(active, active.xmin, Color("#ffffff"), 0.02, Color("#789b58"), 1);
  DrawPlaneX(active, 0.5 * (active.xmin + active.xmax), Color("#b8bcc2"), 0.16, Color("#a855f7"), 1);
  DrawPlaneX(active, active.xmax, Color("#ffffff"), 0.02, Color("#789b58"), 1);
  DrawObliqueSegmentationGrid(active, activeBoxes);
  DrawProjectedArapucas(arapucaBoxes, arapucaDoubleBoxes, 2);
  DrawBoxEdges(cryo, Color("#b7c2cc"), 1, 2);
  DrawBoxEdges(active, Color("#2f7d5b"), 3);

  canvas.SaveAs(outPath);
}

void DrawArapucaLayerRendering(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes) {
  const int canvasHeight = std::min(height, 520);
  TCanvas canvas("paper_fastdpsu_arapuca_layer", "paper_fastdpsu_arapuca_layer", width, canvasHeight);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  Bounds2 b;
  Extend(b, {active.zmin, active.ymin});
  Extend(b, {active.zmax, active.ymax});
  RangeWithAspect(b, static_cast<double>(width) / canvasHeight, 0.035);

  TBox activeFill(active.zmin, active.ymin, active.zmax, active.ymax);
  activeFill.SetLineColor(Color("#ffffff"));
  activeFill.SetFillColorAlpha(Color("#ffffff"), 0.01);
  activeFill.DrawClone("f");

  DrawPlanEngineeringGrid(active);
  for (const auto& box : activeBoxes) {
    DrawPlanOutline(BoundsOf(box), Color("#7b57ff"), 1);
  }
  for (const auto& box : arapucaBoxes) {
    if (CenterInsideYZ(box, active)) DrawPlanRect(BoundsOf(box), Color("#ff5a2a"), Color("#ff9b55"), 0.07, 2);
  }
  for (const auto& box : arapucaDoubleBoxes) {
    if (CenterInsideYZ(box, active)) DrawPlanRect(BoundsOf(box), Color("#ff8c1a"), Color("#ffd166"), 0.08, 2);
  }
  DrawPlanTicks(active, Color("#2f7d5b"));
  DrawPlanOutline(active, Color("#2f7d5b"), 3);

  canvas.SaveAs(outPath);
}

void DrawFastDPSUStringDetail(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<Segment>& strings,
    const std::vector<Point3>& nodes,
    const std::vector<WireBox>& cableBoxes,
    const std::vector<WireBox>& fiberBoxes,
    const std::vector<WireBox>& housingBoxes,
    const std::vector<WireBox>& carrierBoxes,
    const std::vector<WireBox>& boardBoxes,
    const std::vector<WireBox>& sensorBoxes,
    const std::vector<WireBox>& couplerBoxes,
    const std::vector<WireBox>& clampBoxes) {
  const int canvasWidth = std::min(width, 980);
  const int canvasHeight = std::min(std::max(height, 920), 1080);
  TCanvas canvas("paper_fastdpsu_string_detail", "paper_fastdpsu_string_detail", canvasWidth, canvasHeight);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  const auto allZs = UniqueStringZs(strings);
  std::vector<double> selectedZs;
  const double centerZ = 0.5 * (active.zmin + active.zmax);
  for (int pick = 0; pick < 4 && pick < static_cast<int>(allZs.size()); ++pick) {
    int bestIndex = -1;
    double bestDistance = std::numeric_limits<double>::max();
    for (int i = 0; i < static_cast<int>(allZs.size()); ++i) {
      if (NearAny(allZs[i], selectedZs, 1.0)) continue;
      const double distance = std::abs(allZs[i] - centerZ);
      if (distance < bestDistance) {
        bestDistance = distance;
        bestIndex = i;
      }
    }
    if (bestIndex >= 0) selectedZs.push_back(allZs[bestIndex]);
  }
  std::sort(selectedZs.begin(), selectedZs.end());

  Bounds3 view = active;
  if (!selectedZs.empty()) {
    const double pitch = selectedZs.size() > 1 ? std::abs(selectedZs[1] - selectedZs[0]) : 180.0;
    view.zmin = selectedZs.front() - 0.75 * pitch;
    view.zmax = selectedZs.back() + 0.75 * pitch;
  }
  Bounds2 b;
  Extend(b, {view.zmin, view.xmin});
  Extend(b, {view.zmax, view.xmax});
  RangeWithAspect(b, static_cast<double>(canvasWidth) / canvasHeight, 0.08);

  TBox fill(view.zmin, view.xmin, view.zmax, view.xmax);
  fill.SetLineColor(Color("#ffffff"));
  fill.SetFillColorAlpha(Color("#ffffff"), 0.01);
  fill.DrawClone("f");
  DrawSideEngineeringGrid(view);
  DrawSideCentralBand(view, 22.0, Color("#b8bcc2"), Color("#a855f7"));

  for (const double z : UniqueEdges(activeBoxes, 'z')) {
    if (z < view.zmin || z > view.zmax) continue;
    TLine line(z, active.xmin, z, active.xmax);
    line.SetLineColor(Color("#7b57ff"));
    line.SetLineWidth(1);
    line.DrawClone();
  }
  for (const double x : UniqueEdges(activeBoxes, 'x')) {
    TLine line(view.zmin, x, view.zmax, x);
    line.SetLineColor(Color("#eee7ff"));
    line.SetLineStyle(3);
    line.SetLineWidth(1);
    line.DrawClone();
  }

  const double zTolerance = 3.0;
  for (const double z : selectedZs) {
    TLine line(z, active.xmin, z, active.xmax);
    line.SetLineColor(Color("#9a5a16"));
    line.SetLineWidth(3);
    line.DrawClone();
  }

  DrawFastDPSUDetailBoxes(
      view, cableBoxes, fiberBoxes, housingBoxes, carrierBoxes, boardBoxes, sensorBoxes, couplerBoxes, clampBoxes);

  std::vector<double> zs;
  std::vector<double> xs;
  std::set<long long> keys;
  for (const auto& node : nodes) {
    if (!NearAny(node.z, selectedZs, zTolerance)) continue;
    const long long key = std::llround(node.z * 10.0) * 10000000LL + std::llround(node.x * 10.0);
    if (!keys.insert(key).second) continue;
    zs.push_back(node.z);
    xs.push_back(node.x);
  }
  if (!zs.empty()) {
    TGraph halo(static_cast<int>(zs.size()), zs.data(), xs.data());
    halo.SetMarkerStyle(20);
    halo.SetMarkerColor(Color("#d9edf7"));
    halo.SetMarkerSize(0.95);
    halo.DrawClone("P");

    TGraph graph(static_cast<int>(zs.size()), zs.data(), xs.data());
    graph.SetMarkerStyle(20);
    graph.SetMarkerColor(Color("#2b6f91"));
    graph.SetMarkerSize(0.55);
    graph.DrawClone("P");
  }

  DrawSideOutline(view, Color("#2f7d5b"), 4);
  canvas.SaveAs(outPath);
}

void DrawFastDPSUNodeDetail(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const std::vector<WireBox>& cableBoxes,
    const std::vector<WireBox>& fiberBoxes,
    const std::vector<WireBox>& housingBoxes,
    const std::vector<WireBox>& carrierBoxes,
    const std::vector<WireBox>& boardBoxes,
    const std::vector<WireBox>& sensorBoxes,
    const std::vector<WireBox>& couplerBoxes,
    const std::vector<WireBox>& clampBoxes) {
  const int canvasWidth = std::min(width, 1100);
  const int canvasHeight = std::min(std::max(height, 720), 820);
  TCanvas canvas("paper_fastdpsu_node_detail", "paper_fastdpsu_node_detail", canvasWidth, canvasHeight);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  Point3 target{0.5 * (active.xmin + active.xmax), 0.0, 0.5 * (active.zmin + active.zmax)};
  if (!housingBoxes.empty()) {
    double best = std::numeric_limits<double>::max();
    for (const auto& box : housingBoxes) {
      const Point3 c = CenterOf(box);
      const double d = std::hypot(c.x - target.x, c.z - target.z);
      if (d < best) {
        best = d;
        target = c;
      }
    }
  }

  Bounds3 view = active;
  view.xmin = target.x - 5.0;
  view.xmax = target.x + 5.0;
  view.zmin = target.z - 6.0;
  view.zmax = target.z + 6.0;

  Bounds2 b;
  Extend(b, {view.zmin, view.xmin});
  Extend(b, {view.zmax, view.xmax});
  RangeWithAspect(b, static_cast<double>(canvasWidth) / canvasHeight, 0.12);

  TBox fill(view.zmin, view.xmin, view.zmax, view.xmax);
  fill.SetLineColor(Color("#ffffff"));
  fill.SetFillColorAlpha(Color("#ffffff"), 0.01);
  fill.DrawClone("f");
  DrawLocalSideGrid(view);
  DrawFastDPSUDetailBoxes(
      view, cableBoxes, fiberBoxes, housingBoxes, carrierBoxes, boardBoxes, sensorBoxes, couplerBoxes, clampBoxes);
  DrawSideOutline(view, Color("#2f7d5b"), 4);

  canvas.SaveAs(outPath);
}

void DrawPitchVariant(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<Segment>& strings,
    const std::vector<Point3>& nodes,
    int stringStride,
    int nodeStride,
    bool densify) {
  const int canvasHeight = std::min(height, 420);
  TCanvas canvas("paper_fastdpsu_pitch_variant", "paper_fastdpsu_pitch_variant", width, canvasHeight);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  std::vector<double> zs = UniqueStringZs(strings);
  std::vector<double> xs = UniqueNodeXs(nodes);
  if (densify) {
    zs = DensifyValues(zs);
    xs = DensifyValues(xs);
  } else {
    zs = SelectEvery(zs, stringStride);
    xs = SelectEvery(xs, nodeStride);
  }

  RangeXZ(active, width, canvasHeight);
  TBox fill(active.zmin, active.xmin, active.zmax, active.xmax);
  fill.SetLineColor(Color("#ffffff"));
  fill.SetFillColorAlpha(Color("#ffffff"), 0.01);
  fill.DrawClone("f");
  DrawSideEngineeringGrid(active);
  DrawSideCentralBand(active, 22.0, Color("#b8bcc2"), Color("#a855f7"));

  for (const double z : UniqueEdges(activeBoxes, 'z')) {
    TLine line(z, active.xmin, z, active.xmax);
    line.SetLineColor(Color("#7b57ff"));
    line.SetLineWidth(1);
    line.DrawClone();
  }
  for (const double x : UniqueEdges(activeBoxes, 'x')) {
    TLine line(active.zmin, x, active.zmax, x);
    line.SetLineColor(Color("#efe8ff"));
    line.SetLineStyle(3);
    line.SetLineWidth(1);
    line.DrawClone();
  }

  for (const double z : zs) {
    TLine line(z, active.xmin, z, active.xmax);
    line.SetLineColor(Color("#9a5a16"));
    line.SetLineWidth(densify ? 1 : 2);
    line.DrawClone();
  }

  std::vector<double> pointZs;
  std::vector<double> pointXs;
  pointZs.reserve(zs.size() * xs.size());
  pointXs.reserve(zs.size() * xs.size());
  for (const double z : zs) {
    for (const double x : xs) {
      pointZs.push_back(z);
      pointXs.push_back(x);
    }
  }

  if (!pointZs.empty()) {
    TGraph halo(static_cast<int>(pointZs.size()), pointZs.data(), pointXs.data());
    halo.SetMarkerStyle(20);
    halo.SetMarkerColor(Color("#d9edf7"));
    halo.SetMarkerSize(densify ? 0.62 : 0.88);
    halo.DrawClone("P");

    TGraph graph(static_cast<int>(pointZs.size()), pointZs.data(), pointXs.data());
    graph.SetMarkerStyle(20);
    graph.SetMarkerColor(Color("#2b6f91"));
    graph.SetMarkerSize(densify ? 0.36 : 0.52);
    graph.DrawClone("P");
  }

  DrawSideOutline(active, Color("#2f7d5b"), 4);
  canvas.SaveAs(outPath);
}

void DrawTransparentCryostatActive(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const WireBox& cryostatBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes,
    const std::vector<Segment>& strings,
    const std::vector<Point3>& nodes) {
  TCanvas canvas("paper_fastdpsu_transparent", "paper_fastdpsu_transparent", width, height);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  const Bounds3 cryo = BoundsOf(cryostatBox.valid ? cryostatBox : activeBox);
  Bounds2 projected;
  ExtendProjected(projected, cryo);
  for (const auto& node : nodes) Extend(projected, Project(node));
  RangeWithAspect(projected, static_cast<double>(width) / height, 0.035);

  DrawPlaneX(active, active.xmin, Color("#d9fbe8"), 0.08, Color("#2f7d5b"), 1);
  DrawPlaneX(active, 0.5 * (active.xmin + active.xmax), Color("#b8bcc2"), 0.16, Color("#a855f7"), 1);
  DrawPlaneX(active, active.xmax, Color("#d9fbe8"), 0.08, Color("#2f7d5b"), 1);
  DrawObliqueSegmentationGrid(active, activeBoxes);
  DrawProjectedArapucas(arapucaBoxes, arapucaDoubleBoxes, 2);

  for (const auto& segment : strings) DrawLine2(Project(segment.a), Project(segment.b), Color("#9a5a16"), 1);
  DrawProjectedPoints(nodes, Color("#d9edf7"), 20, 0.70);
  DrawProjectedPoints(nodes, Color("#2b6f91"), 20, 0.34);
  DrawBoxEdges(cryo, Color("#b7c2cc"), 1, 2);
  DrawBoxEdges(active, Color("#2f7d5b"), 4);

  canvas.SaveAs(outPath);
}

void DrawGeometryValidationView(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<Segment>& strings,
    const std::vector<Point3>& nodes) {
  const int canvasHeight = std::min(height, 440);
  TCanvas canvas("paper_fastdpsu_validation", "paper_fastdpsu_validation", width, canvasHeight);
  PrepareCanvas(&canvas);

  const Bounds3 active = BoundsOf(activeBox);
  RangeXZ(active, width, canvasHeight);

  TBox fill(active.zmin, active.xmin, active.zmax, active.xmax);
  fill.SetLineColor(Color("#ffffff"));
  fill.SetFillColorAlpha(Color("#ffffff"), 0.01);
  fill.DrawClone("f");
  DrawSideEngineeringGrid(active);
  DrawSideCentralBand(active, 22.0, Color("#b8bcc2"), Color("#a855f7"));

  for (const double z : UniqueEdges(activeBoxes, 'z')) {
    TLine line(z, active.xmin, z, active.xmax);
    line.SetLineColor(Color("#e6ddff"));
    line.SetLineWidth(1);
    line.DrawClone();
  }

  std::set<int> drawnZ;
  for (const auto& segment : strings) {
    const int zKey = static_cast<int>(std::lround(0.5 * (segment.a.z + segment.b.z)));
    if (!drawnZ.insert(zKey).second) continue;
    TLine line(segment.a.z, segment.a.x, segment.b.z, segment.b.x);
    line.SetLineColor(Color("#d58b3a"));
    line.SetLineWidth(1);
    line.DrawClone();
  }

  std::vector<double> normalZs;
  std::vector<double> normalXs;
  std::vector<double> edgeZs;
  std::vector<double> edgeXs;
  std::set<long long> keys;
  const double clearanceThreshold = 180.0;
  for (const auto& node : nodes) {
    const long long key = std::llround(node.z * 10.0) * 10000000LL + std::llround(node.x * 10.0);
    if (!keys.insert(key).second) continue;
    const double clearance = std::min({
        node.x - active.xmin,
        active.xmax - node.x,
        node.z - active.zmin,
        active.zmax - node.z,
    });
    if (clearance < clearanceThreshold) {
      edgeZs.push_back(node.z);
      edgeXs.push_back(node.x);
    } else {
      normalZs.push_back(node.z);
      normalXs.push_back(node.x);
    }
  }

  if (!normalZs.empty()) {
    TGraph normal(static_cast<int>(normalZs.size()), normalZs.data(), normalXs.data());
    normal.SetMarkerStyle(20);
    normal.SetMarkerColor(Color("#ffd6df"));
    normal.SetMarkerSize(0.45);
    normal.DrawClone("P");
  }
  if (!edgeZs.empty()) {
    TGraph halo(static_cast<int>(edgeZs.size()), edgeZs.data(), edgeXs.data());
    halo.SetMarkerStyle(20);
    halo.SetMarkerColor(Color("#ffd1d1"));
    halo.SetMarkerSize(1.00);
    halo.DrawClone("P");

    TGraph edge(static_cast<int>(edgeZs.size()), edgeZs.data(), edgeXs.data());
    edge.SetMarkerStyle(20);
    edge.SetMarkerColor(Color("#e11d48"));
    edge.SetMarkerSize(0.62);
    edge.DrawClone("P");
  }

  DrawSideOutline(active, Color("#2f7d5b"), 4);
  canvas.SaveAs(outPath);
}

void PrepareSubPad(TPad& pad) {
  pad.SetFillColor(kWhite);
  pad.SetFrameFillColor(kWhite);
  pad.SetBorderMode(0);
  pad.SetMargin(0.0, 0.0, 0.0, 0.0);
}

void DrawOrthographicEngineeringSheet(
    const char* outPath,
    int width,
    int height,
    const WireBox& activeBox,
    const WireBox& cryostatBox,
    const std::vector<WireBox>& activeBoxes,
    const std::vector<WireBox>& arapucaBoxes,
    const std::vector<WireBox>& arapucaDoubleBoxes,
    const std::vector<Segment>& strings,
    const std::vector<Point3>& nodes) {
  const int sheetHeight = std::max(height, 1050);
  TCanvas canvas("paper_fastdpsu_orthographic_sheet", "paper_fastdpsu_orthographic_sheet", width, sheetHeight);
  PrepareCanvas(&canvas);

  TPad obliquePad("orthographic_oblique", "orthographic_oblique", 0.0, 0.56, 1.0, 1.0);
  TPad xzPad("orthographic_xz", "orthographic_xz", 0.0, 0.28, 1.0, 0.56);
  TPad yzPad("orthographic_yz", "orthographic_yz", 0.0, 0.0, 1.0, 0.28);
  PrepareSubPad(obliquePad);
  PrepareSubPad(xzPad);
  PrepareSubPad(yzPad);
  obliquePad.Draw();
  xzPad.Draw();
  yzPad.Draw();

  const Bounds3 active = BoundsOf(activeBox);
  const Bounds3 cryo = BoundsOf(cryostatBox.valid ? cryostatBox : activeBox);

  obliquePad.cd();
  {
    Bounds2 projected;
    ExtendProjected(projected, active);
    RangeWithAspect(projected, static_cast<double>(width) / (sheetHeight * 0.44), 0.04);
    DrawPlaneX(active, active.xmin, Color("#ffffff"), 0.02, Color("#789b58"), 1);
    DrawPlaneX(active, 0.5 * (active.xmin + active.xmax), Color("#b8bcc2"), 0.16, Color("#a855f7"), 1);
    DrawPlaneX(active, active.xmax, Color("#ffffff"), 0.02, Color("#789b58"), 1);
    DrawObliqueSegmentationGrid(active, activeBoxes);
    DrawProjectedArapucas(arapucaBoxes, arapucaDoubleBoxes, 2);
    for (const auto& segment : strings) DrawLine2(Project(segment.a), Project(segment.b), Color("#9a5a16"), 1);
    DrawProjectedPoints(nodes, Color("#2b6f91"), 20, 0.30);
    DrawBoxEdges(cryo, Color("#b7c2cc"), 1, 2);
    DrawBoxEdges(active, Color("#2f7d5b"), 3);
  }

  xzPad.cd();
  {
    RangeXZ(active, width, static_cast<int>(sheetHeight * 0.28));
    DrawSideEngineeringGrid(active);
    DrawSideCentralBand(active, 22.0, Color("#b8bcc2"), Color("#a855f7"));
    for (const double z : UniqueEdges(activeBoxes, 'z')) {
      TLine line(z, active.xmin, z, active.xmax);
      line.SetLineColor(Color("#7b57ff"));
      line.SetLineWidth(1);
      line.DrawClone();
    }
    std::set<int> drawnZ;
    for (const auto& segment : strings) {
      const int zKey = static_cast<int>(std::lround(0.5 * (segment.a.z + segment.b.z)));
      if (!drawnZ.insert(zKey).second) continue;
      TLine line(segment.a.z, segment.a.x, segment.b.z, segment.b.x);
      line.SetLineColor(Color("#9a5a16"));
      line.SetLineWidth(1);
      line.DrawClone();
    }
    std::vector<double> zs;
    std::vector<double> xs;
    std::set<long long> nodeKeys;
    for (const auto& node : nodes) {
      const long long key = std::llround(node.z * 10.0) * 10000000LL + std::llround(node.x * 10.0);
      if (!nodeKeys.insert(key).second) continue;
      zs.push_back(node.z);
      xs.push_back(node.x);
    }
    TGraph graph(static_cast<int>(zs.size()), zs.data(), xs.data());
    graph.SetMarkerStyle(20);
    graph.SetMarkerColor(Color("#2b6f91"));
    graph.SetMarkerSize(0.42);
    graph.DrawClone("P");
    DrawSideOutline(active, Color("#2f7d5b"), 4);
  }

  yzPad.cd();
  {
    Bounds2 b;
    Extend(b, {active.zmin, active.ymin});
    Extend(b, {active.zmax, active.ymax});
    RangeWithAspect(b, static_cast<double>(width) / (sheetHeight * 0.28), 0.035);
    DrawPlanEngineeringGrid(active);
    for (const auto& box : activeBoxes) DrawPlanOutline(BoundsOf(box), Color("#7b57ff"), 1);
    for (const auto& box : arapucaBoxes) {
      if (CenterInsideYZ(box, active)) DrawPlanRect(BoundsOf(box), Color("#ff5a2a"), Color("#ff9b55"), 0.06, 2);
    }
    for (const auto& box : arapucaDoubleBoxes) {
      if (CenterInsideYZ(box, active)) DrawPlanRect(BoundsOf(box), Color("#ff8c1a"), Color("#ffd166"), 0.07, 2);
    }
    DrawPlanMarkers(strings, Color("#d9edf7"), Color("#2b6f91"));
    DrawPlanOutline(active, Color("#2f7d5b"), 3);
  }

  canvas.cd();
  canvas.SaveAs(outPath);
}

}  // namespace

void render_dunevd_paper_fastdpsu(
    const char* gdmlPath = "gdml/dunevd10kt_3view_30deg_v7_refactored_2x8x40_nowires_fastdpsu_paper.gdml",
    const char* outPrefix = "data/dunevd_paper_fastdpsu_renders/dunevd_paper_fastdpsu",
    int width = 1900,
    int height = 720) {
  gROOT->SetBatch(kTRUE);
  gStyle->SetOptStat(0);
  gErrorIgnoreLevel = kWarning;

  TGeoManager* geom = TGeoManager::Import(gdmlPath);
  if (!geom) {
    Error("render_dunevd_paper_fastdpsu", "Could not import %s", gdmlPath);
    return;
  }

  const auto activeBox = CollectFirstBox(geom, "volEnclosureTPC");
  const auto cryostatBox = CollectFirstBox(geom, "volCryostat");
  const auto tpcBoxes = CollectBoxes(geom, "volTPC");
  const auto activeBoxes = CollectBoxes(geom, "volTPCActive");
  const auto anodeBoxes = CollectBoxes(geom, "volAnodePlate");
  const auto anodeBottomBoxes = CollectBoxes(geom, "volAnodePlateBottom");
  const auto fieldShaperBoxes = CollectBoxes(geom, "volFieldShaper");
  const auto fieldShaperSlimBoxes = CollectBoxes(geom, "volFieldShaperSlim");
  const auto arapucaBoxes = CollectBoxes(geom, "volArapuca");
  const auto arapucaDoubleBoxes = CollectBoxes(geom, "volArapucaDouble");
  const auto strings = CollectStrings(geom);
  const auto nodes = CollectCenters(geom, "volOpDetSensitive_FastDPSU");
  const auto fastdpsuHousingBoxes = CollectBoxes(geom, "volFastDPSUHousing");
  const auto fastdpsuCarrierBoxes = CollectBoxes(geom, "volFastDPSUCarrierPlate");
  const auto fastdpsuCableBoxes = CollectBoxes(geom, "volFastDPSUStringCoreCable");
  const auto fastdpsuFiberBoxes = CollectBoxes(geom, "volFastDPSUOpticalFiber");
  const auto fastdpsuBoardBoxes = CollectBoxes(geom, "volFastDPSUReadoutBoard");
  const auto fastdpsuSensorBoxes = CollectBoxes(geom, "volOpDetSensitive_FastDPSU");
  const auto fastdpsuCouplerBoxes = CollectBoxes(geom, "volFastDPSUFiberCoupler");
  const auto fastdpsuClampBoxes = CollectBoxes(geom, "volFastDPSUNodeClamp");

  if (!activeBox.valid || strings.empty() || nodes.empty()) {
    Error(
        "render_dunevd_paper_fastdpsu",
        "Missing required geometry: active=%d strings=%zu nodes=%zu",
        activeBox.valid,
        strings.size(),
        nodes.size());
    return;
  }

  gSystem->mkdir(gSystem->DirName(outPrefix), kTRUE);

  TString oblique = TString::Format("%s_oblique.png", outPrefix);
  TString xz = TString::Format("%s_xz_dangling.png", outPrefix);
  TString yz = TString::Format("%s_yz_string_map.png", outPrefix);
  TString singleModule = TString::Format("%s_single_module_cutaway.png", outPrefix);
  TString halfCutaway = TString::Format("%s_half_detector_cutaway.png", outPrefix);
  TString endOn = TString::Format("%s_end_on_dangling_slice.png", outPrefix);
  TString comparison = TString::Format("%s_comparison_overlay.png", outPrefix);
  TString baselineSameCamera = TString::Format("%s_baseline_same_camera.png", outPrefix);
  TString stringDetail = TString::Format("%s_string_detail.png", outPrefix);
  TString nodeDetail = TString::Format("%s_node_detail.png", outPrefix);
  TString arapucaLayer = TString::Format("%s_arapuca_layer.png", outPrefix);
  TString pitchSparse = TString::Format("%s_pitch_sparse.png", outPrefix);
  TString pitchNominal = TString::Format("%s_pitch_nominal.png", outPrefix);
  TString pitchDense = TString::Format("%s_pitch_dense.png", outPrefix);
  TString orthographic = TString::Format("%s_orthographic_engineering_sheet.png", outPrefix);
  TString transparent = TString::Format("%s_transparent_cryostat_active.png", outPrefix);
  TString validation = TString::Format("%s_geometry_validation.png", outPrefix);

  printf(
      "Rendering %zu TPCs, %zu active regions, %zu ARAPUCA boxes, %zu strings, %zu DPSU marker nodes, %zu detailed node boxes\n",
      tpcBoxes.size(),
      activeBoxes.size(),
      arapucaBoxes.size() + arapucaDoubleBoxes.size(),
      strings.size(),
      nodes.size(),
      fastdpsuHousingBoxes.size() + fastdpsuCarrierBoxes.size() + fastdpsuBoardBoxes.size() +
          fastdpsuSensorBoxes.size());
  DrawOblique(
      oblique.Data(),
      width,
      height,
      activeBox,
      cryostatBox,
      tpcBoxes,
      activeBoxes,
      anodeBoxes,
      anodeBottomBoxes,
      fieldShaperBoxes,
      fieldShaperSlimBoxes,
      arapucaBoxes,
      arapucaDoubleBoxes,
      strings,
      nodes);
  DrawXZ(xz.Data(), width, height, activeBox, activeBoxes, anodeBoxes, anodeBottomBoxes, strings, nodes);
  DrawYZ(yz.Data(), width, height, activeBox, activeBoxes, arapucaBoxes, arapucaDoubleBoxes, strings);
  DrawSingleModuleCutaway(
      singleModule.Data(),
      width,
      height,
      activeBox,
      cryostatBox,
      activeBoxes,
      arapucaBoxes,
      arapucaDoubleBoxes,
      strings,
      nodes);
  DrawHalfDetectorCutaway(
      halfCutaway.Data(),
      width,
      height,
      activeBox,
      cryostatBox,
      activeBoxes,
      arapucaBoxes,
      arapucaDoubleBoxes,
      strings,
      nodes);
  DrawEndOnDanglingSlice(endOn.Data(), width, height, activeBox, strings, nodes);
  DrawFastDPSUComparisonOverlay(
      comparison.Data(),
      width,
      height,
      activeBox,
      cryostatBox,
      activeBoxes,
      arapucaBoxes,
      arapucaDoubleBoxes,
      strings,
      nodes);
  DrawBaselineSameCamera(
      baselineSameCamera.Data(),
      width,
      height,
      activeBox,
      cryostatBox,
      activeBoxes,
      arapucaBoxes,
      arapucaDoubleBoxes);
  DrawFastDPSUStringDetail(
      stringDetail.Data(),
      width,
      height,
      activeBox,
      activeBoxes,
      strings,
      nodes,
      fastdpsuCableBoxes,
      fastdpsuFiberBoxes,
      fastdpsuHousingBoxes,
      fastdpsuCarrierBoxes,
      fastdpsuBoardBoxes,
      fastdpsuSensorBoxes,
      fastdpsuCouplerBoxes,
      fastdpsuClampBoxes);
  DrawFastDPSUNodeDetail(
      nodeDetail.Data(),
      width,
      height,
      activeBox,
      fastdpsuCableBoxes,
      fastdpsuFiberBoxes,
      fastdpsuHousingBoxes,
      fastdpsuCarrierBoxes,
      fastdpsuBoardBoxes,
      fastdpsuSensorBoxes,
      fastdpsuCouplerBoxes,
      fastdpsuClampBoxes);
  DrawArapucaLayerRendering(arapucaLayer.Data(), width, height, activeBox, activeBoxes, arapucaBoxes, arapucaDoubleBoxes);
  DrawPitchVariant(pitchSparse.Data(), width, height, activeBox, activeBoxes, strings, nodes, 3, 2, false);
  DrawPitchVariant(pitchNominal.Data(), width, height, activeBox, activeBoxes, strings, nodes, 1, 1, false);
  DrawPitchVariant(pitchDense.Data(), width, height, activeBox, activeBoxes, strings, nodes, 1, 1, true);
  DrawTransparentCryostatActive(
      transparent.Data(),
      width,
      height,
      activeBox,
      cryostatBox,
      activeBoxes,
      arapucaBoxes,
      arapucaDoubleBoxes,
      strings,
      nodes);
  DrawGeometryValidationView(validation.Data(), width, height, activeBox, activeBoxes, strings, nodes);
  DrawOrthographicEngineeringSheet(
      orthographic.Data(),
      width,
      height,
      activeBox,
      cryostatBox,
      activeBoxes,
      arapucaBoxes,
      arapucaDoubleBoxes,
      strings,
      nodes);
}

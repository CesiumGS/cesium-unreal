#include "VectorGpuLookupGen.h"

#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumUtility/JsonValue.h>
#include <CesiumUtility/Math.h>
#include <CesiumVectorData/GeoJsonObject.h>

#include "CesiumRuntime.h"
#include "Misc/Optional.h"

using namespace CesiumVectorData;
using namespace CesiumUtility;

// TODO:: FIX FOR DEGREES VS RADIANS!!

namespace {
glm::dvec3 deg2rad(const glm::dvec3& v) {
  return glm::dvec3(
      Math::degreesToRadians(v.x),
      Math::degreesToRadians(v.y),
      Math::degreesToRadians(v.z));
}

std::vector<glm::dvec3> deg2rad(const std::vector<glm::dvec3>& positions) {
  std::vector<glm::dvec3> result;
  result.reserve(positions.size());
  for (const glm::dvec3& v : positions) {
    result.emplace_back(deg2rad(v));
  }
  return result;
}

std::optional<std::vector<std::vector<bool>>>
getCutFlagsMember(const GeoJsonPolygon& polygon) {
  CesiumUtility::JsonValue::Object::const_iterator it =
      polygon.foreignMembers.find("cut_flags");
  if (it == polygon.foreignMembers.end()) {
    return std::nullopt;
  }

  if (!it->second.isArray()) {
    return std::nullopt;
  }

  std::vector<std::vector<bool>> values(it->second.getArray().size());
  for (const CesiumUtility::JsonValue& value : it->second.getArray()) {
    if (!value.isArray()) {
      values.push_back({});
    } else {
      std::vector<bool> childValues(value.getArray().size());
      for (const CesiumUtility::JsonValue& childValue : value.getArray()) {
        childValues.push_back(childValue.getBoolOrDefault(false));
      }
      values.push_back(childValues);
    }
  }

  return values;
}

size_t getNumofLineSegments(CesiumVectorData::GeoJsonObject::IteratorProvider<
                            CesiumVectorData::ConstGeoJsonObjectTypeIterator<
                                CesiumVectorData::GeoJsonFeature>> features) {
  size_t count = 0;
  for (const GeoJsonFeature& feature : features) {
    if (feature.geometry && feature.geometry->isType<GeoJsonLineString>()) {
      count +=
          feature.geometry->get<GeoJsonLineString>().coordinates.size() - 1;
    }

    if (feature.geometry && feature.geometry->isType<GeoJsonPolygon>()) {
      count +=
          feature.geometry->get<GeoJsonPolygon>().coordinates[0].size() - 1;
    }
  }
  return count;
}

// Helper function to check if a point is inside the boundary
bool isInside(
    const glm::dvec3& point,
    double boundary,
    bool isVertical,
    bool isMin) {
  if (isVertical) {
    return isMin ? point.x >= boundary : point.x <= boundary;
  }
  return isMin ? point.y >= boundary : point.y <= boundary;
}

// Helper function to compute the intersection of a line segment with a
// boundary
glm::dvec3 findIntersection(
    const glm::dvec3& p1,
    const glm::dvec3& p2,
    double boundary,
    bool isVertical) {
  if (isVertical) {
    // Vertical boundary: x = boundary
    const double t = (boundary - p1.x) / (p2.x - p1.x);
    const double y = p1.y + t * (p2.y - p1.y);
    return glm::dvec3(boundary, y, 0.0);
  }
  // Horizontal boundary: y = boundary
  const double t = (boundary - p1.y) / (p2.y - p1.y);
  const double x = p1.x + t * (p2.x - p1.x);
  return glm::dvec3(x, boundary, 0.0);
}

// Clip the polygon against a single boundary
std::tuple<std::vector<glm::dvec3>, std::vector<bool>> clipAgainstBoundary(
    const std::vector<glm::dvec3>& points,
    const std::vector<bool>& in_flags,
    double boundary,
    bool isVertical,
    bool isMin) {
  std::vector<glm::dvec3> newPoints;
  std::vector<bool> flags;
  for (size_t i = 0; i < points.size(); i++) {
    const glm::dvec3& curr = points[i];
    const glm::dvec3& prev = points[(i - 1 + points.size()) % points.size()];
    const bool currInside = isInside(curr, boundary, isVertical, isMin);
    const bool prevInside = isInside(prev, boundary, isVertical, isMin);

    if (currInside && prevInside) {
      // Both points are inside, add the current point
      newPoints.push_back(curr);
      flags.push_back(in_flags[i]);
    } else if (currInside && !prevInside) {
      // Current point is inside, previous point is outside
      // Add the intersection point and the current point
      newPoints.push_back(findIntersection(prev, curr, boundary, isVertical));
      flags.push_back(true);
      newPoints.push_back(curr);
      flags.push_back(in_flags[i]);
    } else if (!currInside && prevInside) {
      // Current point is outside, previous point is inside
      // Add the intersection point
      newPoints.push_back(findIntersection(prev, curr, boundary, isVertical));
      flags.push_back(true);
    }
    // If both points are outside, do nothing
  }
  return {newPoints, flags};
}

/**
 * Clips a polygon ring to a bounding box and returns a new ring enclosing the
 * areas within the bounding box.
 *
 * @param {Array} ring - The polygon ring as an array of points [[x1, y1], [x2,
 * y2], ...].
 * @param {number} x0 - The minimum x-coordinate of the bounding box.
 * @param {number} x1 - The maximum x-coordinate of the bounding box.
 * @param {number} y0 - The minimum y-coordinate of the bounding box.
 * @param {number} y1 - The maximum y-coordinate of the bounding box.
 * @returns {Array} The clipped polygon ring as an array of points.
 */
std::tuple<std::vector<glm::dvec3>, std::vector<bool>> clipPolygonToBoundingBox(
    std::vector<glm::dvec3> ring,
    std::vector<bool> flags,
    double x0,
    double x1,
    double y0,
    double y1) {

  // Start with the original ring
  std::vector<glm::dvec3> clippedRing = ring;
  std::vector<bool> clippingFlags = flags;

  // Clip against each boundary of the bounding box
  std::tie(clippedRing, clippingFlags) = clipAgainstBoundary(
      clippedRing,
      clippingFlags,
      x0,
      true,
      true); // Left boundary
  std::tie(clippedRing, clippingFlags) = clipAgainstBoundary(
      clippedRing,
      clippingFlags,
      x1,
      true,
      false); // Right boundary
  std::tie(clippedRing, clippingFlags) = clipAgainstBoundary(
      clippedRing,
      clippingFlags,
      y0,
      false,
      true); // Bottom boundary
  std::tie(clippedRing, clippingFlags) = clipAgainstBoundary(
      clippedRing,
      clippingFlags,
      y1,
      false,
      false); // Top boundary

  // Ensure the ring is closed (first and last points are the same)
  if (clippedRing.size() > 0 &&
      (clippedRing[0][0] != clippedRing[clippedRing.size() - 1][0] ||
       clippedRing[0][1] != clippedRing[clippedRing.size() - 1][1])) {
    clippedRing.push_back(clippedRing[0]);
    clippingFlags.push_back(clippingFlags[0]);
  }

  return {clippedRing, clippingFlags};
}

} // namespace

std::optional<FCesiumVectorLookup> FCesiumVectorLookup::Create(
    GeoJsonObject& rootObject,
    const CesiumGeospatial::GlobeRectangle& bbox,
    size_t targetLinePerCell,
    bool useTest,
    bool inGridClip) {
  const size_t lineSegmentsCount =
      getNumofLineSegments(rootObject.allOfType<GeoJsonFeature>());

  // decide grid size based on features length
  // aim for 1k lines per grid cell
  const size_t gridSizeX = (size_t)std::ceil(
      std::sqrt(lineSegmentsCount / (double)targetLinePerCell));
  const size_t gridSizeY = (size_t)std::ceil(
      std::sqrt(lineSegmentsCount / (double)targetLinePerCell));
  // console.log("Grid size (auto computed):", gridSizeX, gridSizeY);
  UE_LOG(LogCesium, Display, TEXT("Grid size: %d, %d"), gridSizeX, gridSizeY);

  // chop into grid cells
  const double cellWidth = bbox.computeWidth() / (double)gridSizeX;
  const double cellHeight = bbox.computeHeight() / (double)gridSizeY;
  UE_LOG(LogCesium, Display, TEXT("Cell size: %f, %f"), cellWidth, cellHeight);
  // console.log("Cell size:", cellWidth, cellHeight);

  // get a 2D array to hold line segments in each cell
  std::vector<std::vector<std::vector<glm::dvec4>>> grid;
  grid.resize(gridSizeX);
  std::vector<std::vector<std::vector<bool>>> gridCutFlags;
  gridCutFlags.resize(gridSizeX);
  for (size_t i = 0; i < gridSizeX; i++) {
    grid[i] = std::vector<std::vector<glm::dvec4>>();
    grid[i].resize(gridSizeY);
    gridCutFlags[i] = std::vector<std::vector<bool>>(gridSizeY);
    gridCutFlags[i].resize(gridSizeY);
  }

  // console.log("Original number of line segments:", lineSegmentsCount);
  size_t numOfLineSegmentsGeoJSON = 0;

  bool hasPolygons = false;

  // assign line segments to grid cells
  for (const GeoJsonFeature& feature : rootObject.allOfType<GeoJsonFeature>()) {
    if (!feature.geometry) {
      continue;
    }
    if (feature.geometry->isType<GeoJsonPolygon>()) {
      hasPolygons = true;
      const std::vector<std::vector<glm::dvec3>>& coords =
          feature.geometry->get<GeoJsonPolygon>().coordinates;
      const std::optional<std::vector<std::vector<bool>>> ringCutFlags =
          getCutFlagsMember(feature.geometry->get<GeoJsonPolygon>());
      for (size_t j = 0; j < coords.size(); j++) {
        const std::vector<glm::dvec3>& ring = deg2rad(coords[j]);

        double minX = ring[0].x;
        double maxX = ring[0].x;
        double minY = ring[0].y;
        double maxY = ring[0].y;
        for (size_t k = 1; k < ring.size(); k++) {
          const double x = ring[k].x;
          const double y = ring[k].y;
          if (x < minX) {
            minX = x;
          }
          if (x > maxX) {
            maxX = x;
          }
          if (y < minY) {
            minY = y;
          }
          if (y > maxY) {
            maxY = y;
          }
        }
        // determine which grid cells the polygon ring bbox overlaps
        const size_t startCellX = std::max(
            (size_t)0,
            (size_t)std::floor((minX - bbox.getWest()) / cellWidth));
        const size_t endCellX = std::min(
            gridSizeX - 1,
            (size_t)std::floor((maxX - bbox.getWest()) / cellWidth));
        const size_t startCellY = std::max(
            (size_t)0,
            (size_t)std::floor((minY - bbox.getSouth()) / cellHeight));
        const size_t endCellY = std::min(
            gridSizeY - 1,
            (size_t)std::floor((maxY - bbox.getSouth()) / cellHeight));

        // iterate through grid cells
        for (size_t c = startCellX; c <= endCellX; c++) {
          for (size_t r = startCellY; r <= endCellY; r++) {
            const double cellMinX = bbox.getWest() + c * cellWidth;
            const double cellMaxX = bbox.getWest() + (c + 1) * cellWidth;
            const double cellMinY = bbox.getSouth() + r * cellHeight;
            const double cellMaxY = bbox.getSouth() + (r + 1) * cellHeight;
            std::vector<glm::dvec3> p;
            std::vector<bool> flags;
            if (inGridClip) {
              auto [p, flags] = clipPolygonToBoundingBox(
                  deg2rad(coords[j]),
                  (*ringCutFlags)[j],
                  cellMinX,
                  cellMaxX,
                  cellMinY,
                  cellMaxY);
            } else {
              p = deg2rad(coords[j]);
              flags = ringCutFlags ? ringCutFlags->at(j)
                                   : std::vector<bool>(coords[j].size(), false);
            }
            if (p.size() > 0) {
              // convert to line segments
              for (size_t k = 0; k < p.size() - 1; k++) {
                const glm::dvec3& p1 = p[k];
                const glm::dvec3& p2 = p[k + 1];
                const bool flag0 = flags.size() > k ? flags[k] : false;
                const bool flag1 = flags.size() > k ? flags[k + 1] : false;
                const bool flagLine = flag0 && flag1 ? 1 : 0;
                check(c < grid.size() && r < grid[c].size());
                grid[c][r].push_back(glm::dvec4(p1.x, p1.y, p2.x, p2.y));
                check(c < gridCutFlags.size() && r < gridCutFlags[c].size());
                gridCutFlags[c][r].push_back(flagLine);
              }
              numOfLineSegmentsGeoJSON +=
                  p.size() - 1; // account for the duplicated line segment
            }
          }
        }
      }
    }

    if (feature.geometry->isType<GeoJsonLineString>()) {
      const std::vector<glm::dvec3>& coords =
          deg2rad(feature.geometry->get<GeoJsonLineString>().coordinates);
      // iterate through line segments
      // duplicate line segments that cross cell boundaries
      for (size_t j = 0; j < coords.size() - 1; j++) {
        const glm::dvec3& p1 = coords[j];
        // get next point
        const glm::dvec3& p2 = coords[j + 1];

        // determine which cells the endpoints are in
        const size_t cellX1 = std::max(
            std::min(
                (size_t)std::floor((p1.x - bbox.getWest()) / cellWidth),
                gridSizeX - 1),
            (size_t)0);
        const size_t cellY1 = std::max(
            std::min(
                (size_t)std::floor((p1.y - bbox.getSouth()) / cellHeight),
                gridSizeY - 1),
            (size_t)0);
        const size_t cellX2 = std::max(
            std::min(
                (size_t)std::floor((p2.x - bbox.getWest()) / cellWidth),
                gridSizeX - 1),
            (size_t)0);
        const size_t cellY2 = std::max(
            std::min(
                (size_t)std::floor((p2.y - bbox.getSouth()) / cellHeight),
                gridSizeY - 1),
            (size_t)0);

        // NOTE: this is assuming lines do not cross more than 2 cells
        // which is not always true. This is for convenience here.
        if (cellX1 == cellX2 && cellY1 == cellY2) {
          // endpoints in the same cell
          check(cellX1 < grid.size() && cellY1 < grid[cellX1].size());
          grid[cellX1][cellY1].push_back(glm::dvec4(p1.x, p1.y, p2.x, p2.y));
          numOfLineSegmentsGeoJSON += 1;
        } else {
          // endpoints in different cells
          check(cellX1 < grid.size() && cellY1 < grid[cellX1].size());
          grid[cellX1][cellY1].push_back(glm::dvec4(p1.x, p1.y, p2.x, p2.y));
          check(cellX2 < grid.size() && cellY1 < grid[cellX2].size());
          grid[cellX2][cellY2].push_back(glm::dvec4(p1.x, p1.y, p2.x, p2.y));
          numOfLineSegmentsGeoJSON +=
              2; // account for the duplicated line segment
        }
      }
    }
  }

  UE_LOG(
      LogCesium,
      Display,
      TEXT("Num line segments after grid assignment: %d"),
      numOfLineSegmentsGeoJSON);

  // console.log(
  //   "Number of line segments after grid assignment:",
  //   numOfLineSegmentsGeoJSON,
  //);

  UE_LOG(
      LogCesium,
      Display,
      TEXT("Number of line segments after grid split: %d"),
      numOfLineSegmentsGeoJSON);

  if (numOfLineSegmentsGeoJSON == 0) {
    return std::nullopt;
  }

  // get texture size to the next power of 2
  const size_t textureWidth = std::pow(
      2,
      std::ceil(std::log2(std::ceil(std::sqrt(numOfLineSegmentsGeoJSON)))));
  const size_t textureHeight = std::pow(
      2,
      std::ceil(std::log2(
          std::ceil(numOfLineSegmentsGeoJSON / (double)textureWidth))));
  const size_t numOfLineSegments = textureWidth * textureHeight;

  UE_LOG(
      LogCesium,
      Display,
      TEXT("Number of line segments in GPU texture: %d"),
      numOfLineSegments);
  std::vector<float> coords;
  coords.resize(numOfLineSegments * 4);
  std::vector<uint32_t> indices;
  indices.resize(
      gridSizeX * gridSizeY + 2); // squeeze in grid dimensions at the start
  std::vector<uint8_t> cutFlags;
  cutFlags.resize(numOfLineSegments);

  indices[0] = gridSizeX;
  indices[1] = gridSizeY;

  // loop through grids and fill in coords
  // record the index of each grid cell end position
  int index = 0;
  for (size_t j = 0; j < gridSizeY; j++) {
    for (size_t i = 0; i < gridSizeX; i++) {
      check(i < grid.size() && j < grid[i].size());
      std::vector<glm::dvec4>& cellLines = grid[i][j];
      for (size_t k = 0; k < cellLines.size(); k++) {
        double x1, y1, x2, y2;
        if (!useTest) {
          x1 = (cellLines[k].x - bbox.getWest()) / bbox.computeWidth();
          y1 = (cellLines[k].y - bbox.getSouth()) / bbox.computeHeight();
          x2 = (cellLines[k].z - bbox.getWest()) / bbox.computeWidth();
          y2 = (cellLines[k].w - bbox.getSouth()) / bbox.computeHeight();
        } else {
          // fill in all diagonal lines for testing
          x1 = 0.0;
          y1 = 0.0;
          x2 = 1.0;
          y2 = 1.0;
        }
        coords[index * 4] = x1;     // x1
        coords[index * 4 + 1] = y1; // y1
        coords[index * 4 + 2] = x2; // x2
        coords[index * 4 + 3] = y2; // y2

        check(index * 4 < coords.size() && index * 4 + 3 < coords.size());

        if (gridCutFlags[i][j].size() > k) {
          check(k < gridCutFlags[i][j].size());
          cutFlags[index] = gridCutFlags[i][j][k];
          check(index < cutFlags.size());
        }

        index++;
      }
      // record the end index of this cell
      indices[2 + j * gridSizeX + i] = index;
      check(2 + j * gridSizeX + i < indices.size());
    }
  }

  // fill in the rest with -1
  for (; index < numOfLineSegments; index++) {
    // everything here is invalid
    coords[index * 4] = -1.0;
    coords[index * 4 + 1] = -1.0;
    coords[index * 4 + 2] = -1.0;
    coords[index * 4 + 3] = -1.0;
    check(index * 4 < coords.size() && index * 4 + 3 < coords.size());
    cutFlags[index] = 2;
    check(index < cutFlags.size());
  }

  return FCesiumVectorLookup{
      coords,
      indices,
      cutFlags,
      glm::uvec2(textureWidth, textureHeight),
      glm::uvec2(gridSizeX, gridSizeY),
      hasPolygons};
}

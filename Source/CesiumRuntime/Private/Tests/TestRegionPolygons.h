#pragma once

#if WITH_EDITOR

#include "VectorTypes.h"
#include <initializer_list>
#include <vector>

namespace Cesium {
struct TestPolygon {
  TestPolygon(std::initializer_list<FVector2D> points) : points(points) {}

  /**
   * Returns a random point within this polygon to test with.
   */
  FVector GetRandomPoint() const;

private:
  std::vector<FVector2D> points;
};

// An array of test polygons delineating populated, high-interest areas to test
// with.
constexpr static int TEST_REGION_POLYGONS_COUNT = 32;

class TestRegionPolygons {
public:
  static const TestPolygon* polygons;
};

} // namespace Cesium

#endif

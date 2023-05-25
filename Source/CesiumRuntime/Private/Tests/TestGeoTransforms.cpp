#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumUtility/Math.h"
#include "GeoTransforms.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGeospatial;
using namespace CesiumUtility;

BEGIN_DEFINE_SPEC(
    FTestGeoTransforms,
    "Cesium.GeoTransforms",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FTestGeoTransforms)

void FTestGeoTransforms::Define() {
  Describe("TransformLongitudeLatitudeHeightToUnreal", [this]() {
    It("returns the origin when given the origin LLH", [this]() {
      GeoTransformsNew geotransforms{};
      glm::dvec3 center = geotransforms.TransformLongitudeLatitudeHeightToEcef(
          glm::dvec3(12.0, 23.0, 1000.0));
      geotransforms.setCenter(center);
      glm::dvec3 ue = geotransforms.TransformLongitudeLatitudeHeightToUnreal(
          glm::dvec3(0.0),
          glm::dvec3(12.0, 23.0, 1000.0));
      TestEqual("is at the origin", ue, glm::dvec3(0.0));
    });
  });
}

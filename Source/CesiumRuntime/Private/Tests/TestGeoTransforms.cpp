#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumUtility/Math.h"
#include "GeoTransforms.h"
#include "GeoTransformsNew.h"
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

  Describe("Test against old implementation", [this]() {
    GeoTransformsNew g(
        Ellipsoid::WGS84,
        glm::dvec3(4500000.0, 4500000.0, 0.0),
        1.0);
    GeoTransforms old(
        Ellipsoid::WGS84,
        glm::dvec3(4500000.0, 4500000.0, 0.0),
        1.0);

    It("TransformLongitudeLatitudeHeightToEcef", [=]() {
      glm::dvec3 ecef = g.TransformLongitudeLatitudeHeightToEcef(
          glm::dvec3(12.0, 23.0, 1000.0));
      glm::dvec3 ecefOld = old.TransformLongitudeLatitudeHeightToEcef(
          glm::dvec3(12.0, 23.0, 1000.0));
      TestTrue(
          "same ecef position",
          Math::equalsEpsilon(ecef, ecefOld, Math::Epsilon15));
    });

    It("TransformEcefToLongitudeLatitudeHeight", [=]() {
      glm::dvec3 llh = g.TransformEcefToLongitudeLatitudeHeight(
          glm::dvec3(4500000.0, 5000000.0, 1000.0));
      glm::dvec3 llhOld = old.TransformEcefToLongitudeLatitudeHeight(
          glm::dvec3(4500000.0, 5000000.0, 1000.0));
      TestTrue(
          "same llh position",
          Math::equalsEpsilon(llh, llhOld, Math::Epsilon15));
    });

    It("TransformLongitudeLatitudeHeightToUnreal", [=]() {
      glm::dvec3 ue = g.TransformLongitudeLatitudeHeightToUnreal(
          glm::dvec3(1000.0, 2000.0, 3000.0),
          glm::dvec3(12.0, 23.0, 1000.0));
      glm::dvec3 ueOld = old.TransformLongitudeLatitudeHeightToUnreal(
          glm::dvec3(1000.0, 2000.0, 3000.0),
          glm::dvec3(12.0, 23.0, 1000.0));
      TestTrue(
          "same UE position",
          Math::equalsEpsilon(ue, ueOld, Math::Epsilon15));
    });

    It("TransformUnrealToLongitudeLatitudeHeight", [=]() {
      glm::dvec3 llh = g.TransformUnrealToLongitudeLatitudeHeight(
          glm::dvec3(1000.0, 2000.0, 3000.0),
          glm::dvec3(1200.0, 23000.0, 10000.0));
      glm::dvec3 llhOld = old.TransformUnrealToLongitudeLatitudeHeight(
          glm::dvec3(1000.0, 2000.0, 3000.0),
          glm::dvec3(1200.0, 23000.0, 10000.0));
      TestTrue(
          "same llh position",
          Math::equalsEpsilon(llh, llhOld, Math::Epsilon15));
    });

    It("TransformEcefToUnreal", [=]() {
      glm::dvec3 ue = g.TransformEcefToUnreal(
          glm::dvec3(1000.0, 2000.0, 3000.0),
          glm::dvec3(4500000.0, 5000000.0, 1000.0));
      glm::dvec3 ueOld = old.TransformEcefToUnreal(
          glm::dvec3(1000.0, 2000.0, 3000.0),
          glm::dvec3(4500000.0, 5000000.0, 1000.0));
      TestTrue(
          "same UE position",
          Math::equalsEpsilon(ue, ueOld, Math::Epsilon15));
    });

    It("TransformUnrealToEcef", [=]() {
      glm::dvec3 ecef = g.TransformUnrealToEcef(
          glm::dvec3(1000.0, 2000.0, 3000.0),
          glm::dvec3(1200.0, 23000.0, 10000.0));
      glm::dvec3 ecefOld = old.TransformUnrealToEcef(
          glm::dvec3(1000.0, 2000.0, 3000.0),
          glm::dvec3(1200.0, 23000.0, 10000.0));
      TestTrue(
          "same ECEF position",
          Math::equalsEpsilon(ecef, ecefOld, Math::Epsilon15));
    });

    It("ComputeEastSouthUpToUnreal", [=]() {
      glm::dmat3 ecef = g.ComputeEastSouthUpToUnreal(
          glm::dvec3(1000.0, 2000.0, 3000.0),
          glm::dvec3(1200.0, 23000.0, 10000.0));
      glm::dmat3 ecefOld = old.ComputeEastSouthUpToUnreal(
          glm::dvec3(1000.0, 2000.0, 3000.0),
          glm::dvec3(1200.0, 23000.0, 10000.0));
      TestTrue(
          "same transformation column 0",
          Math::equalsEpsilon(ecef[0], ecefOld[0], Math::Epsilon15));
      TestTrue(
          "same transformation column 1",
          Math::equalsEpsilon(ecef[1], ecefOld[1], Math::Epsilon15));
      TestTrue(
          "same transformation column 2",
          Math::equalsEpsilon(ecef[2], ecefOld[2], Math::Epsilon15));
    });
  });
}

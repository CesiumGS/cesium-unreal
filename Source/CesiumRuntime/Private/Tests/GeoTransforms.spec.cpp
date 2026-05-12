// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "GeoTransforms.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumUtility/Math.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGeospatial;
using namespace CesiumUtility;

BEGIN_DEFINE_SPEC(
    FGeoTransformsSpec,
    "Cesium.Unit.GeoTransforms",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::ServerContext |
        EAutomationTestFlags::CommandletContext |
        EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FGeoTransformsSpec)

void FGeoTransformsSpec::Define() {
  Describe("TransformLongitudeLatitudeHeightToUnreal", [this]() {
    It("returns the origin when given the origin LLH", [this]() {
      GeoTransforms geotransforms{};
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

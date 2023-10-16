// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumGeoreference.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumTestHelpers.h"
#include "CesiumUtility/Math.h"
#include "GeoTransforms.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGeospatial;
using namespace CesiumUtility;

BEGIN_DEFINE_SPEC(
    FCesiumGeoreferenceSpec,
    "Cesium.Unit.Georeference",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)

TObjectPtr<ACesiumGeoreference> pGeoreferenceNullIsland;
TObjectPtr<ACesiumGeoreference> pGeoreference90Longitude;

END_DEFINE_SPEC(FCesiumGeoreferenceSpec)

void FCesiumGeoreferenceSpec::Define() {
  BeforeEach([this]() {
    UWorld* pWorld = CesiumTestHelpers::getGlobalWorldContext();
    pGeoreferenceNullIsland = pWorld->SpawnActor<ACesiumGeoreference>();
    pGeoreferenceNullIsland->SetOriginLongitudeLatitudeHeight(
        FVector(0.0, 0.0, 0.0));
    pGeoreference90Longitude = pWorld->SpawnActor<ACesiumGeoreference>();
    pGeoreference90Longitude->SetOriginLongitudeLatitudeHeight(
        FVector(90.0, 0.0, 0.0));
  });

  Describe("Position Transformation", [this]() {
    It("transforms longitude-latitude-height positions to Unreal", [this]() {
      FVector nullIslandUnreal =
          pGeoreferenceNullIsland
              ->TransformLongitudeLatitudeHeightPositionToUnreal(
                  FVector(0.0, 0.0, 0.0));
      TestEqual("nullIslandUnreal", nullIslandUnreal, FVector(0.0, 0.0, 0.0));

      FVector antiMeridianUnreal =
          pGeoreferenceNullIsland
              ->TransformLongitudeLatitudeHeightPositionToUnreal(
                  FVector(180.0, 0.0, 0.0));
      TestEqual(
          "antiMeridianUnreal",
          antiMeridianUnreal,
          FVector(
              0.0,
              0.0,
              -2.0 * Ellipsoid::WGS84.getMaximumRadius() * 100.0));
    });

    It("transforms Earth-Centered, Earth-Fixed positions to Unreal", [this]() {
      FVector nullIslandUnreal =
          pGeoreferenceNullIsland
              ->TransformEarthCenteredEarthFixedPositionToUnreal(
                  FVector(Ellipsoid::WGS84.getMaximumRadius(), 0.0, 0.0));
      TestEqual("nullIslandUnreal", nullIslandUnreal, FVector(0.0, 0.0, 0.0));

      FVector antiMeridianUnreal =
          pGeoreferenceNullIsland
              ->TransformEarthCenteredEarthFixedPositionToUnreal(
                  FVector(-Ellipsoid::WGS84.getMaximumRadius(), 0.0, 0.0));
      TestEqual(
          "antiMeridianUnreal",
          antiMeridianUnreal,
          FVector(
              0.0,
              0.0,
              -2.0 * Ellipsoid::WGS84.getMaximumRadius() * 100.0));
    });

    It("transforms Unreal positions to longitude-latitude-height", [this]() {
      FVector nullIslandLLH =
          pGeoreferenceNullIsland
              ->TransformUnrealPositionToLongitudeLatitudeHeight(
                  FVector(0.0, 0.0, 0.0));
      TestEqual("nullIslandLLH", nullIslandLLH, FVector(0.0, 0.0, 0.0));

      FVector antiMeridianLLH =
          pGeoreferenceNullIsland
              ->TransformUnrealPositionToLongitudeLatitudeHeight(FVector(
                  0.0,
                  0.0,
                  -2.0 * Ellipsoid::WGS84.getMaximumRadius() * 100.0));
      TestEqual("antiMeridianLLH", antiMeridianLLH, FVector(180.0, 0.0, 0.0));
    });

    It("transforms Unreal positions to Earth-Centered, Earth-Fixed", [this]() {
      FVector nullIslandEcef =
          pGeoreferenceNullIsland
              ->TransformUnrealPositionToEarthCenteredEarthFixed(
                  FVector(0.0, 0.0, 0.0));
      TestEqual(
          "nullIslandEcef",
          nullIslandEcef,
          FVector(Ellipsoid::WGS84.getMaximumRadius(), 0.0, 0.0));

      FVector antiMeridianEcef =
          pGeoreferenceNullIsland
              ->TransformUnrealPositionToEarthCenteredEarthFixed(FVector(
                  0.0,
                  0.0,
                  -2.0 * Ellipsoid::WGS84.getMaximumRadius() * 100.0));
      TestEqual(
          "antiMeridianEcef",
          antiMeridianEcef,
          FVector(-Ellipsoid::WGS84.getMaximumRadius(), 0.0, 0.0));
    });
  });

  Describe("Direction Transformation", [this]() {
    It("transforms Earth-Centered, Earth-Fixed directions to Unreal", [this]() {
      FVector northAtNullIslandUnreal =
          pGeoreferenceNullIsland
              ->TransformEarthCenteredEarthFixedDirectionToUnreal(
                  FVector(0.0, 0.0, 1.0));
      TestEqual(
          "northAtNullIslandUnreal",
          northAtNullIslandUnreal,
          FVector(0.0, -100.0, 0.0)); // meters -> centimeters

      FVector westAtAntiMeridianUnreal =
          pGeoreferenceNullIsland
              ->TransformEarthCenteredEarthFixedDirectionToUnreal(
                  FVector(0.0, 1.0, 0.0));
      TestEqual(
          "westAtAntiMeridianUnreal",
          westAtAntiMeridianUnreal,
          FVector(
              100.0, // West at anti-merdian is East at null island
              0.0,
              0.0));
    });

    It("transforms Unreal directions to Earth-Centered, Earth-Fixed", [this]() {
      FVector northAtNullIslandEcef =
          pGeoreferenceNullIsland
              ->TransformUnrealDirectionToEarthCenteredEarthFixed(
                  FVector(0.0, -1.0, 0.0));
      TestEqual(
          "northAtNullIslandEcef",
          northAtNullIslandEcef,
          FVector(0.0, 0.0, 1.0 / 100.0)); // centimeters -> meters

      // West at anti-merdian is East at null island
      FVector westAtAntiMeridianEcef =
          pGeoreferenceNullIsland
              ->TransformUnrealDirectionToEarthCenteredEarthFixed(
                  FVector(1.0, 0.0, 0.0));
      TestEqual(
          "westAtAntiMeridianEcef",
          westAtAntiMeridianEcef,
          FVector(0.0, 1.0 / 100.0, 0.0));
    });
  });

  Describe("Rotator Transformation", [this]() {
    It("treats Unreal and East-South-Up as identical at the georeference origin",
       [this]() {
         FRotator atOrigin1 =
             pGeoreferenceNullIsland->TransformEastSouthUpRotatorToUnreal(
                 FRotator(1.0, 2.0, 3.0),
                 FVector(0.0, 0.0, 0.0));
         TestEqual("atOrigin1", atOrigin1, FRotator(1.0, 2.0, 3.0));

         FRotator atOrigin2 =
             pGeoreferenceNullIsland->TransformUnrealRotatorToEastSouthUp(
                 FRotator(1.0, 2.0, 3.0),
                 FVector(0.0, 0.0, 0.0));
         TestEqual("atOrigin2", atOrigin2, FRotator(1.0, 2.0, 3.0));
       });

    It("transforms East-South-Up Rotators to Unreal", [this]() {
      FRotator rotationAt90DegreesLongitude = FRotator(1.0, 2.0, 3.0);
      FVector originOf90DegreesLongitudeInNullIslandCoordinates =
          pGeoreferenceNullIsland
              ->TransformLongitudeLatitudeHeightPositionToUnreal(
                  FVector(90.0, 0.0, 0.0));

      FRotator rotationAtNullIsland =
          pGeoreferenceNullIsland->TransformEastSouthUpRotatorToUnreal(
              rotationAt90DegreesLongitude,
              originOf90DegreesLongitudeInNullIslandCoordinates);

      CesiumTestHelpers::TestRotatorsAreEquivalent(
          this,
          pGeoreference90Longitude,
          rotationAt90DegreesLongitude,
          pGeoreferenceNullIsland,
          rotationAtNullIsland);
    });

    It("transforms Unreal Rotators to East-South-Up", [this]() {
      FRotator rotationAtNullIsland = FRotator(1.0, 2.0, 3.0);
      FVector originOf90DegreesLongitudeInNullIslandCoordinates =
          pGeoreferenceNullIsland
              ->TransformLongitudeLatitudeHeightPositionToUnreal(
                  FVector(90.0, 0.0, 0.0));

      FRotator rotationAt90DegreesLongitude =
          pGeoreferenceNullIsland->TransformUnrealRotatorToEastSouthUp(
              rotationAtNullIsland,
              originOf90DegreesLongitudeInNullIslandCoordinates);

      CesiumTestHelpers::TestRotatorsAreEquivalent(
          this,
          pGeoreferenceNullIsland,
          rotationAtNullIsland,
          pGeoreference90Longitude,
          rotationAt90DegreesLongitude);
    });
  });
}

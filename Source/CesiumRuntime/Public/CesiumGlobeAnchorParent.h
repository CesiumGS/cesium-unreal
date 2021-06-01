// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoreferenceComponent.h"
#include "GameFramework/Actor.h"

#include "CesiumGlobeAnchorParent.generated.h"

/**
 * DEPRECATED: Please create an empty actor, add a
 * CesiumGeoreferenceComponent, and attach child actors to mimic the
 * functionality of this class.
 *
 * This actor defines a local East-South-Up reference frame in centimeters
 * wherever on the globe it is placed. Actors can be attached as children of
 * this actor and operate in the local space. This local reference frame is
 * automatically georeferenced to the globe and its location will be precisely
 * maintained. Note: all children actors must be movable.
 */
UCLASS(Deprecated)
class CESIUMRUNTIME_API ADEPRECATED_CesiumGlobeAnchorParent : public AActor {
  GENERATED_BODY()

public:
  ADEPRECATED_CesiumGlobeAnchorParent();

  /**
   * The WGS84 longitude in degrees of this actor, in the range [-180, 180]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -180.0, ClampMax = 180.0))
  double Longitude = 0.0;

  /**
   * The WGS84 latitude in degrees of this actor, in the range [-90, 90]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -90.0, ClampMax = 90.0))
  double Latitude = 0.0;

  /**
   * The height in meters (above the WGS84 ellipsoid) of this actor.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double Height = 0.0;

  /**
   * The Earth-Centered Earth-Fixed X-coordinate of this actor.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double ECEF_X = 0.0;

  /**
   * The Earth-Centered Earth-Fixed Y-coordinate of this actor.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double ECEF_Y = 0.0;

  /**
   * The Earth-Centered Earth-Fixed Z-coordinate of this actor.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double ECEF_Z = 0.0;

  UPROPERTY()
  UCesiumGeoreferenceComponent* GeoreferenceComponent;

  // Called every frame
  virtual bool ShouldTickIfViewportsOnly() const override;
  virtual void Tick(float DeltaTime) override;

protected:
  virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

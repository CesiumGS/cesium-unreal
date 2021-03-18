// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "GameFramework/Actor.h"

#include "CesiumGlobeAnchorParent.generated.h"

UCLASS()
class CESIUMRUNTIME_API ACesiumGlobeAnchorParent : public AActor {
  GENERATED_BODY()

public:
  ACesiumGlobeAnchorParent();

  /**
   * The longitude of this actor.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double Longitude = 0.0;

  /**
   * The latitude of this actor.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double Latitude = 0.0;

  /**
   * The height in meters (above the WGS84 ellipsoid) of this actor.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double Altitude = 0.0;

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

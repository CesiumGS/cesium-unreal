// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "CesiumGeoreference.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumGeospatialBlueprintLibrary.generated.h"

/**
 *
 */
UCLASS()
class CESIUMRUNTIME_API UCesiumGeospatialBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  static ACesiumGeoreference* GetDefaultGeoref();

public:
  /**
   * Transforms the given WGS84 longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Unreal world coordinates
   * (relative to the floating origin).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium Geospatial")
  static FVector TransformLongLatHeightToUnreal(
      const FVector& LongLatHeight,
      const ACesiumGeoreference* Georef);

  UFUNCTION(BlueprintCallable, Category = "Cesium Geospatial")
  static FVector TransformLongLatHeightToUnrealUsingDefaultGeoref(
      const FVector& LongLatHeight);

  /**
   * Transforms Unreal world coordinates (relative to the floating origin) into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium Geospatial")
  static FVector TransformUnrealToLongLatHeight(
      const FVector& Ue,
      const ACesiumGeoreference* Georef);

  UFUNCTION(BlueprintCallable, Category = "Cesium Geospatial")
  static FVector
  TransformUnrealToLongLatHeightUsingDefaultGeoref(const FVector& Ue);

  /**
   * Transforms the given WGS84 longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Earth-Centered, Earth-Fixed
   * (ECEF) coordinates.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium Geospatial")
  static FVector TransformLongLatHeightToEcef(const FVector& LongLatHeight);

  /**
   * Transforms the given Earth-Centered, Earth-Fixed (ECEF) coordinates into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium Geospatial")
  static FVector TransformEcefToLongLatHeight(const FVector& Ecef);

  /**
   * Transforms a rotator from East-North-Up to Unreal world at the given
   * Unreal relative world location (relative to the floating origin).
   */
  UFUNCTION(BlueprintCallable, Category = "CesiumGeospatial")
  static FRotator TransformRotatorEastNorthUpToUnreal(
      const FRotator& EnuRotator,
      const FVector& UeLocation,
      ACesiumGeoreference* Georef);

  UFUNCTION(BlueprintCallable, Category = "CesiumGeospatial")
  static FRotator TransformRotatorEastNorthUpToUnrealUsingDefaultGeoref(
      const FRotator& EnuRotator,
      const FVector& UeLocation);

  /**
   * Transforms a rotator from Unreal world to East-North-Up at the given
   * Unreal relative world location (relative to the floating origin).
   */
  UFUNCTION(BlueprintCallable, Category = "CesiumGeospatial")
  static FRotator TransformRotatorUnrealToEastNorthUp(
      const FRotator& UeRotator,
      const FVector& UeLocation,
      ACesiumGeoreference* Georef);

  UFUNCTION(BlueprintCallable, Category = "CesiumGeospatial")
  static FRotator TransformRotatorUnrealToEastNorthUpUsingDefaultGeoref(
      const FRotator& UeRotator,
      const FVector& UeLocation);

  /**
   * Computes the rotation matrix from the local East-North-Up to Unreal at the
   * specified Unreal relative world location (relative to the floating
   * origin). The returned transformation works in Unreal's left-handed
   * coordinate system.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium Geospatial")
  static FMatrix
  ComputeEastNorthUpToUnreal(const FVector& Ue, ACesiumGeoreference* Georef);

  UFUNCTION(BlueprintCallable, Category = "Cesium Geospatial")
  static FMatrix
  ComputeEastNorthUpToUnrealUsingDefaultGeoref(const FVector& Ue);

  /**
   * Computes the rotation matrix from the local East-North-Up to
   * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium Geospatial")
  static FMatrix ComputeEastNorthUpToEcef(const FVector& Ecef);

private:
  static TWeakObjectPtr<ACesiumGeoreference>* _defaultGeoref;
};

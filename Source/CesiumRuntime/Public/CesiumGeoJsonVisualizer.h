// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoJsonObject.h"
#include "CesiumVectorData/GeoJsonDocument.h"

#include "CesiumGeoJsonVisualizer.generated.h"

class ACesiumGeoreference;

/**
 */
UCLASS()
class CESIUMRUNTIME_API ACesiumGeoJsonVisualizer : public AActor {
  GENERATED_BODY()

  /**
   */
  ACesiumGeoJsonVisualizer();
  virtual ~ACesiumGeoJsonVisualizer();

private:
  TSoftObjectPtr<ACesiumGeoreference> Georeference;

  /**
   * The resolved georeference used by this Tileset. This is not serialized
   * because it may point to a Georeference in the PersistentLevel while this
   * tileset is in a sublevel. If the Georeference property is specified,
   * however then this property will have the same value.
   *
   * This property will be null before ResolveGeoreference is called.
   */
  UPROPERTY(
      Transient,
      VisibleAnywhere,
      BlueprintReadOnly,
      Category = "Cesium",
      Meta = (AllowPrivateAccess))
  ACesiumGeoreference* ResolvedGeoreference = nullptr;

public:
  /**
   * Resolves the Cesium Georeference to use with this Actor. Returns
   * the value of the Georeference property if it is set. Otherwise, finds a
   * Georeference in the World and returns it, creating it if necessary. The
   * resolved Georeference is cached so subsequent calls to this function will
   * return the same instance.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  ACesiumGeoreference* ResolveGeoreference();

  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|Vector|Render",
      meta = (DisplayName = "Add Line String"))
  void AddLineString(const FCesiumGeoJsonLineString& LineString, bool bDebugMode);

  UPROPERTY(EditAnywhere, Category = "Cesium|Rendering")
  UMaterialInterface* Material = nullptr;

private:
  // This property mirrors RootComponent, and exists only so that the root
  // component's transform is editable in the Editor.
  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  USceneComponent* Root;
};

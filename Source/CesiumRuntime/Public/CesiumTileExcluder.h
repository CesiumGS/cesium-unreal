#pragma once

#include "Cesium3DTileset.h"
#include "CesiumTile.h"
#include "CoreMinimal.h"
#include "CesiumTileExcluder.generated.h"

class CesiumTileExcluderAdapter;

/**
 * An actor component for excluding Cesium Tiles.
 * This class provides an interface for excluding Cesium Tiles from a tileset.
 * You can create a blueprint that derives from this class and override the
 * `ShouldExclude` function to implement custom logic for determining whether a
 * tile should be excluded. This function can be implemented in either C++ or
 * Blueprints.
 */
UCLASS(
    ClassGroup = (Cesium),
    meta = (BlueprintSpawnableComponent),
    Blueprintable,
    Abstract)
class CESIUMRUNTIME_API UCesiumTileExcluder : public UActorComponent {
  GENERATED_BODY()
private:
  CesiumTileExcluderAdapter* pExcluderAdapter;

protected:
  UPROPERTY()
  UCesiumTile* CesiumTile;

  UPROPERTY()
  ACesium3DTileset* CesiumTileset;

public:
  UCesiumTileExcluder(const FObjectInitializer& ObjectInitializer);

  /**
   * Adds this tile excluder to its owning Cesium 3D Tileset Actor. If the
   * excluder is already added or if this component's Owner is not a Cesium 3D
   * Tileset, this method does nothing.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void AddToTileset();

  /**
   * Removes this tile excluder from its owning Cesium 3D Tileset Actor. If the
   * excluder is not yet added or if this component's Owner is not a Cesium 3D
   * Tileset, this method does nothing.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void RemoveFromTileset();

  /**
   * Determines whether a tile should be excluded.
   * This function is called to determine whether a tile should be excluded from
   * the tileset. You can override this function in a derived class or blueprint
   * to implement custom exclusion logic.
   */
  UFUNCTION(BlueprintNativeEvent)
  bool ShouldExclude(const UCesiumTile* TileObject);
};

// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once
#include "CesiumTile.h"
#include "Components/ActorComponent.h"
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

  UPROPERTY()
  UCesiumTile* CesiumTile;

public:
  UCesiumTileExcluder(const FObjectInitializer& ObjectInitializer);

  virtual void Activate(bool bReset) override;
  virtual void Deactivate() override;
  virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

#if WITH_EDITOR
  // Called when properties are changed in the editor
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

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
   * Refreshes this tile excluderby removing from its owning Cesium 3D Tileset
   * Actor and re-adding it. If this component's Owner is not a Cesium 3D
   * Tileset Actor, this method does nothing.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void Refresh();

  /**
   * Determines whether a tile should be excluded.
   * This function is called to determine whether a tile should be excluded from
   * the tileset. You can override this function in a derived class or blueprint
   * to implement custom exclusion logic.
   */
  UFUNCTION(BlueprintNativeEvent)
  bool ShouldExclude(const UCesiumTile* TileObject);
};

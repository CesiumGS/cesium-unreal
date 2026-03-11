// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/ActorComponent.h"

#include "CesiumMetadataComponent.generated.h"

class ACesium3DTileset;

namespace Cesium3DTilesSelection {
class TilesetMetadata;
}

/**
 * @brief A base component interface for interacting with metadata in a
 * Cesium3DTileset.
 */
UCLASS(ClassGroup = Cesium, Meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumMetadataComponent : public UActorComponent {
  GENERATED_BODY()

public:
  /**
   * Syncs this component's statistics description from its tileset owner,
   * retrieving values for the corresponding semantics.
   *
   * If there are described statistics that are not present on the tileset
   * owner, they will be left as null values.
   */
  void SyncStatistics();

  /**
   * Whether a sync is already in progress.
   */
  bool IsSyncing() const;

  /**
   * Interrupts any sync happening on this component. Usually called before
   * destroying or refreshing a tileset.
   */
  void InterruptSync();

protected:
  /**
   * Event that occurs once metadata is successfully synced from the tileset
   * during SyncStatistics. Derived classes must override this to make use of
   * the results.
   */
  virtual void OnFetchMetadata(
      ACesium3DTileset* pActor,
      const Cesium3DTilesSelection::TilesetMetadata* pMetadata);

  /**
   * Called during various parts of the sync process. Derived classes must
   * override this.
   */
  virtual void ClearStatistics();

  // Called when a component is registered. This seems to be the best way to
  // intercept when the component is pasted (to then update its statistics).
  virtual void OnRegister() override;

private:
  bool _syncInProgress;
};

// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTiles/ViewState.h"
#include "CesiumCreditSystem.h"
#include "CesiumExclusionZone.h"
#include "CesiumGeoreference.h"
#include "CesiumGeoreferenceable.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include <PhysicsEngine/BodyInstance.h>
#include <chrono>
#include <glm/mat4x4.hpp>
#include "Cesium3DTileset.generated.h"

class UMaterialInterface;

namespace Cesium3DTiles {
class Tileset;
class TilesetView;
} // namespace Cesium3DTiles

UCLASS()
class CESIUMRUNTIME_API ACesium3DTileset : public AActor,
                                           public ICesiumGeoreferenceable {
  GENERATED_BODY()

public:
  ACesium3DTileset();
  virtual ~ACesium3DTileset();

  /**
   * The URL of this tileset's "tileset.json" file.
   *
   * If this property is specified, the ion asset ID and token are ignored.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Url;

  /**
   * The ID of the Cesium ion asset to use.
   *
   * This property is ignored if the Url is specified.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  uint32 IonAssetID;

  /**
   * The access token to use to access the Cesium ion resource.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadOnly,
      Category = "Cesium",
      meta = (EditCondition = "IonAssetID"))
  FString IonAccessToken;

  /**
   * The actor controlling how this tileset's coordinate system relates to the
   * coordinate system in this Unreal Engine level.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ACesiumGeoreference* Georeference;

  /**
   * The actor managing this tileset's content attributions.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ACesiumCreditSystem* CreditSystem;

  /**
   * The maximum number of pixels of error when rendering this tileset.
   *
   * This is used to select an appropriate level-of-detail: A low value
   * will cause many tiles with a high level of detail to be loaded,
   * causing a finer visual representation of the tiles, but with a
   * higher performance cost for loading and rendering. A higher value will
   * cause a coarser visual representation, with lower performance
   * requirements.
   *
   * When a tileset uses the older layer.json / quantized-mesh format rather
   * than 3D Tiles, this value is effectively divided by 8.0. So the default
   * value of 16.0 corresponds to the standard value for quantized-mesh terrain
   * of 2.0.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Level of Detail")
  double MaximumScreenSpaceError = 16.0;

  /**
   * Whether to preload ancestor tiles.
   *
   * Setting this to true optimizes the zoom-out experience and provides more
   * detail in newly-exposed areas when panning. The down side is that it
   * requires loading more tiles.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Tile Loading")
  bool PreloadAncestors = true;

  /**
   * Whether to preload sibling tiles.
   *
   * Setting this to true causes tiles with the same parent as a rendered tile
   * to be loaded, even if they are culled. Setting this to true may provide a
   * better panning experience at the cost of loading more tiles.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Tile Loading")
  bool PreloadSiblings = true;

  /**
   * Whether to unrefine back to a parent tile when a child isn't done loading.
   *
   * When this is set to true, the tileset will guarantee that the tileset will
   * never be rendered with holes in place of tiles that are not yet loaded,
   * even though the tile that is rendered instead may have low resolution. When
   * false, overall loading will be faster, but newly-visible parts of the
   * tileset may initially be blank.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Tile Loading")
  bool ForbidHoles = false;

  /**
   * The maximum number of tiles that may be loaded at once.
   *
   * When new parts of the tileset become visible, the tasks to load the
   * corresponding tiles are put into a queue. This value determines how
   * many of these tasks are processed at the same time. A higher value
   * may cause the tiles to be loaded and rendered more quickly, at the
   * cost of a higher network- and processing load.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Tile Loading")
  int MaximumSimultaneousTileLoads = 20;

  /**
   * The number of loading descendents a tile should allow before deciding to
   * render itself instead of waiting.
   *
   * Setting this to 0 will cause each level of detail to be loaded
   * successively. This will increase the overall loading time, but cause
   * additional detail to appear more gradually. Setting this to a high value
   * like 1000 will decrease the overall time until the desired level of detail
   * is achieved, but this high-detail representation will appear at once, as
   * soon as it is loaded completely.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Tile Loading")
  int LoadingDescendantLimit = 20;

  /**
   * Whether to cull tiles that are outside the frustum.
   *
   * By default this is true, meaning that tiles that are not visible with the
   * current camera configuration will be ignored. It can be set to false, so
   * that these tiles are still considered for loading, refinement and
   * rendering.
   *
   * This will cause more tiles to be loaded, but helps to avoid holes and
   * provides a more consistent mesh, which may be helpful for physics.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Tile Culling")
  bool EnableFrustumCulling = true;

  /**
   * Whether to cull tiles that are occluded by fog.
   *
   * This does not refer to the atmospheric fog of the Unreal Engine,
   * but to an internal representation of fog: Depending on the height
   * of the camera above the ground, tiles that are far away (close to
   * the horizon) will be culled when this flag is enabled.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Tile Culling")
  bool EnableFogCulling = true;

  /**
   * Whether a specified screen-space error should be enforced for tiles that
   * are outside the frustum or hidden in fog.
   *
   * When "Enable Frustum Culling" and "Enable Fog Culling" are both true, tiles
   * outside the view frustum or hidden in fog are effectively ignored, and so
   * their level-of-detail doesn't matter. And in this scenario, this property
   * is ignored.
   *
   * However, when either of those flags are false, these "would-be-culled"
   * tiles continue to be processed, and the question arises of how to handle
   * their level-of-detail. When this property is false, refinement terminates
   * at these tiles, no matter what their current screen-space error. The tiles
   * are available for physics, shadows, etc., but their level-of-detail may
   * be very low.
   *
   * When set to true, these tiles are refined until they achieve the specified
   * "Culled Screen Space Error". This allows control over the minimum quality
   * of these would-be-culled tiles.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Tile Culling")
  bool EnforceCulledScreenSpaceError = false;

  /**
   * A list of rectangles that are excluded from this tileset. Any tiles that
   * overlap any of these rectangles are not shown. This is a crude method to
   * avoid overlapping geometry coming from different tilesets. For example, to
   * exclude Cesium OSM Buildings where there are photogrammetry assets.
   *
   * Note that in the current version, excluded tiles are still loaded, they're
   * just not displayed. Also, because the tiles shown when zoomed out cover a
   * large area, using an exclusion zone often means the tileset won't be shown
   * at all when zoomed out.
   *
   * This property is currently only supported for 3D Tiles that use "region"
   * for their bounding volumes. For other tilesets it is silently ignored.
   *
   * This is an experimental feature and may change in future versions.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Experimental")
  TArray<FCesiumExclusionZone> ExclusionZones;

  /**
   * The screen-space error to be enforced for tiles that are outside the view
   * frustum or hidden in fog.
   *
   * When "Enable Frustum Culling" and "Enable Fog Culling" are both true, tiles
   * outside the view frustum or hidden in fog are effectively ignored, and so
   * their level-of-detail doesn't matter. And in this scenario, this property
   * is ignored.
   *
   * However, when either of those flags are false, these "would-be-culled"
   * tiles continue to be processed, and the question arises of how to handle
   * their level-of-detail. When "Enforce Culled Screen Space Error" is false,
   * this property is ignored and refinement terminates at these tiles, no
   * matter what their current screen-space error. The tiles are available for
   * physics, shadows, etc., but their level-of-detail may be very low.
   *
   * When set to true, these tiles are refined until they achieve the
   * screen-space error specified by this property.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Tile Culling",
      meta = (EditCondition = "EnforceCulledScreenSpaceError"))
  double CulledScreenSpaceError = 64.0;

  /**
   * A custom Material to use to render this tileset, in order to implement
   * custom visual effects.
   *
   * The custom material should generally be created by copying the
   * "GltfMaterialWithOverlays" material and customizing it as desired.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Rendering")
  UMaterialInterface* Material = nullptr;

  /**
   * Whether to request and render the water mask.
   *
   * Currently only applicable for quantized-mesh tilesets that support the
   * water mask extension.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Rendering")
  bool EnableWaterMask = true;

  /**
   * Pauses level-of-detail and culling updates of this tileset.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Debug")
  bool SuspendUpdate;

  /**
   * If true, this tileset is ticked/updated in the editor. If false, is only
   * ticked while playing (including Play-in-Editor).
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Debug")
  bool UpdateInEditor = true;

  /**
   * Define the collision profile for all the 3D tiles created inside this
   * actor.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadOnly,
      Category = "Collision",
      meta = (ShowOnlyInnerProperties, SkipUCSModifiedProperties))
  FBodyInstance BodyInstance;

  UFUNCTION(BlueprintCallable, Category = "Cesium|Rendering")
  void PlayMovieSequencer();

  UFUNCTION(BlueprintCallable, Category = "Cesium|Rendering")
  void StopMovieSequencer();

  UFUNCTION(BlueprintCallable, Category = "Cesium|Rendering")
  void PauseMovieSequencer();

  const glm::dmat4& GetCesiumTilesetToUnrealRelativeWorldTransform() const;

  Cesium3DTiles::Tileset* GetTileset() { return this->_pTileset; }
  const Cesium3DTiles::Tileset* GetTileset() const { return this->_pTileset; }

  void UpdateTransformFromCesium(const glm::dmat4& CesiumToUnreal);

  // ICesiumGeoreferenceable implementation
  virtual bool IsBoundingVolumeReady() const override;
  virtual std::optional<Cesium3DTiles::BoundingVolume>
  GetBoundingVolume() const override;
  virtual void NotifyGeoreferenceUpdated();

  // AActor overrides
  virtual bool ShouldTickIfViewportsOnly() const override;
  virtual void Tick(float DeltaTime) override;
  virtual void BeginDestroy() override;
  virtual void Destroyed() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void PostLoad() override;

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;
  virtual void OnConstruction(const FTransform& Transform) override;

  virtual void NotifyHit(
      class UPrimitiveComponent* MyComp,
      AActor* Other,
      class UPrimitiveComponent* OtherComp,
      bool bSelfMoved,
      FVector HitLocation,
      FVector HitNormal,
      FVector NormalImpulse,
      const FHitResult& Hit) override;

private:
  void LoadTileset();
  void DestroyTileset();
  Cesium3DTiles::ViewState CreateViewStateFromViewParameters(
      const FVector2D& viewportSize,
      const FVector& location,
      const FRotator& rotation,
      double fieldOfViewDegrees) const;

  struct UnrealCameraParameters {
    FVector2D viewportSize;
    FVector location;
    FRotator rotation;
    double fieldOfViewDegrees;
  };

  std::optional<UnrealCameraParameters> GetCamera() const;
  std::optional<UnrealCameraParameters> GetPlayerCamera() const;

#if WITH_EDITOR
  std::optional<UnrealCameraParameters> GetEditorCamera() const;
  void OnFocusEditorViewportOnActors(const AActor* actor);
#endif

private:
  Cesium3DTiles::Tileset* _pTileset;

  UMaterialInterface* _lastMaterial = nullptr;

  uint32_t _lastTilesRendered;
  uint32_t _lastTilesLoadingLowPriority;
  uint32_t _lastTilesLoadingMediumPriority;
  uint32_t _lastTilesLoadingHighPriority;

  uint32_t _lastTilesVisited;
  uint32_t _lastCulledTilesVisited;
  uint32_t _lastTilesCulled;
  uint32_t _lastMaxDepthVisited;

  bool _updateGeoreferenceOnBoundingVolumeReady;
  std::chrono::high_resolution_clock::time_point _startTime;

  bool _captureMovieMode;
  bool _beforeMoviePreloadAncestors;
  bool _beforeMoviePreloadSiblings;
  int32_t _beforeMovieLoadingDescendantLimit;
  bool _beforeMovieKeepWorldOriginNearCamera;
};

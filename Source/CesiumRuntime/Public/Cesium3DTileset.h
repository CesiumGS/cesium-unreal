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

UENUM(BlueprintType)
enum class ETilesetSource : uint8 {
  /**
   * The tileset will be loaded from Cesium Ion using the provided IonAssetID
   * and IonAccessToken.
   */
  FromCesiumIon UMETA(DisplayName = "From Cesium Ion"),

  /**
   * The tileset will be loaded from the specified Url.
   */
  FromUrl UMETA(DisplayName = "From Url")
};

UCLASS()
class CESIUMRUNTIME_API ACesium3DTileset : public AActor,
                                           public ICesiumGeoreferenceable {
  GENERATED_BODY()

public:
  ACesium3DTileset();
  virtual ~ACesium3DTileset();

  /**
   * The actor controlling how this tileset's coordinate system relates to the
   * coordinate system in this Unreal Engine level.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
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
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Level of Detail",
      meta = (ClampMin = 0.0))
  float MaximumScreenSpaceError = 16.0;

  /**
   * Whether to preload ancestor tiles.
   *
   * Setting this to true optimizes the zoom-out experience and provides more
   * detail in newly-exposed areas when panning. The down side is that it
   * requires loading more tiles.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Tile Loading")
  bool PreloadAncestors = true;

  /**
   * Whether to preload sibling tiles.
   *
   * Setting this to true causes tiles with the same parent as a rendered tile
   * to be loaded, even if they are culled. Setting this to true may provide a
   * better panning experience at the cost of loading more tiles.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Tile Loading")
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
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Tile Loading")
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
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Tile Loading",
      meta = (ClampMin = 0))
  int32 MaximumSimultaneousTileLoads = 20;

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
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium|Tile Loading",
      meta = (ClampMin = 0))
  int32 LoadingDescendantLimit = 20;

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
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Tile Culling")
  bool EnableFrustumCulling = true;

  /**
   * Whether to cull tiles that are occluded by fog.
   *
   * This does not refer to the atmospheric fog of the Unreal Engine,
   * but to an internal representation of fog: Depending on the height
   * of the camera above the ground, tiles that are far away (close to
   * the horizon) will be culled when this flag is enabled.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Tile Culling")
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
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Tile Culling")
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
      BlueprintReadWrite,
      Category = "Cesium|Tile Culling",
      meta = (EditCondition = "EnforceCulledScreenSpaceError", ClampMin = 0.0))
  float CulledScreenSpaceError = 64.0;

  /**
   * Pauses level-of-detail and culling updates of this tileset.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium|Debug")
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

private:
  /**
   * The type of source from which to load this tileset.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintGetter = GetTilesetSource,
      BlueprintSetter = SetTilesetSource,
      Category = "Cesium",
      meta = (DisplayName = "Source"))
  ETilesetSource TilesetSource = ETilesetSource::FromCesiumIon;

  /**
   * The URL of this tileset's "tileset.json" file.
   *
   * If this property is specified, the ion asset ID and token are ignored.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintGetter = GetUrl,
      BlueprintSetter = SetUrl,
      Category = "Cesium",
      meta = (EditCondition = "TilesetSource==ETilesetSource::FromUrl"))
  FString Url;

  /**
   * The ID of the Cesium ion asset to use.
   *
   * This property is ignored if the Url is specified.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintGetter = GetIonAssetID,
      BlueprintSetter = SetIonAssetID,
      Category = "Cesium",
      meta =
          (EditCondition = "TilesetSource==ETilesetSource::FromCesiumIon",
           ClampMin = 0))
  int32 IonAssetID;

  /**
   * The access token to use to access the Cesium ion resource.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintGetter = GetIonAccessToken,
      BlueprintSetter = SetIonAccessToken,
      Category = "Cesium",
      meta = (EditCondition = "TilesetSource==ETilesetSource::FromCesiumIon"))
  FString IonAccessToken;

  /**
   * Whether to request and render the water mask.
   *
   * Currently only applicable for quantized-mesh tilesets that support the
   * water mask extension.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintGetter = GetEnableWaterMask,
      BlueprintSetter = SetEnableWaterMask,
      Category = "Cesium|Rendering",
      meta = (EditCondition = "PlatformName != TEXT(\"Mac\")"))
  bool EnableWaterMask = false;

  /**
   * A custom Material to use to render this tileset, in order to implement
   * custom visual effects.
   *
   * The custom material should generally be created by copying the
   * "M_CesiumOverlay" material and customizing it as desired.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintGetter = GetMaterial,
      BlueprintSetter = SetMaterial,
      Category = "Cesium|Rendering")
  UMaterialInterface* Material = nullptr;

  /**
   * A custom Material to use to render this tileset in areas where the
   * watermask is, in order to implement custom visual effects.
   * Currently only applicable for quantized-mesh tilesets that support the
   * water mask extension.
   *
   * The custom material should generally be created by copying the
   * "M_CesiumOverlayWater" material and customizing it as desired. For best
   * results, any changes to the above material should also be duplicated in the
   * Water Material.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintGetter = GetWaterMaterial,
      BlueprintSetter = SetWaterMaterial,
      Category = "Cesium|Rendering")
  UMaterialInterface* WaterMaterial = nullptr;

  /**
   * A custom Material to use to render this tileset in areas using opacity
   * masks, in order to implement custom visual effects.
   *
   * The custom material should generally be created by copying the
   * "M_CesiumDefaultMasked" material and customizing it as desired.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintGetter = GetOpacityMaskMaterial,
      BlueprintSetter = SetOpacityMaskMaterial,
      Category = "Cesium|Rendering")
  UMaterialInterface* OpacityMaskMaterial = nullptr;

protected:
  UPROPERTY()
  FString PlatformName;

public:
  UFUNCTION(BlueprintGetter, Category = "Cesium")
  ETilesetSource GetTilesetSource() const { return TilesetSource; }

  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetTilesetSource(ETilesetSource InSource);

  UFUNCTION(BlueprintGetter, Category = "Cesium")
  FString GetUrl() const { return Url; }

  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetUrl(FString InUrl);

  UFUNCTION(BlueprintGetter, Category = "Cesium")
  int32 GetIonAssetID() const { return IonAssetID; }

  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetIonAssetID(int32 InAssetID);

  UFUNCTION(BlueprintGetter, Category = "Cesium")
  FString GetIonAccessToken() const { return IonAccessToken; }

  UFUNCTION(BlueprintSetter, Category = "Cesium")
  void SetIonAccessToken(FString InAccessToken);

  UFUNCTION(BlueprintGetter, Category = "Cesium|Rendering")
  bool GetEnableWaterMask() const { return EnableWaterMask; }

  UFUNCTION(BlueprintSetter, Category = "Cesium|Rendering")
  void SetEnableWaterMask(bool bEnableMask);

  UFUNCTION(BlueprintGetter, Category = "Cesium|Rendering")
  UMaterialInterface* GetMaterial() const { return Material; }

  UFUNCTION(BlueprintSetter, Category = "Cesium|Rendering")
  void SetMaterial(UMaterialInterface* InMaterial);

  UFUNCTION(BlueprintGetter, Category = "Cesium|Rendering")
  UMaterialInterface* GetWaterMaterial() const { return WaterMaterial; }

  UFUNCTION(BlueprintSetter, Category = "Cesium|Rendering")
  void SetWaterMaterial(UMaterialInterface* InMaterial);

  UFUNCTION(BlueprintGetter, Category = "Cesium|Rendering")
  UMaterialInterface* GetOpacityMaskMaterial() const {
    return OpacityMaskMaterial;
  }

  UFUNCTION(BlueprintSetter, Category = "Cesium|Rendering")
  void SetOpacityMaskMaterial(UMaterialInterface* InMaterial);

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
  virtual void Serialize(FArchive& Ar) override;

  // UObject overrides
#if WITH_EDITOR
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
  // Called when the game starts or when spawned
  virtual void BeginPlay() override;
  virtual void OnConstruction(const FTransform& Transform) override;

  /**
   * Called after the C++ constructor and after the properties have
   * been initialized, including those loaded from config.
   */
  virtual void PostInitProperties() override;

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
  void MarkTilesetDirty();
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

  /**
   * Will be called after the tileset is loaded or spawned, to register
   * a delegate that calls OnFocusEditorViewportOnThis when this
   * tileset is double-clicked
   */
  void AddFocusViewportDelegate();

#if WITH_EDITOR
  std::optional<UnrealCameraParameters> GetEditorCamera() const;

  /**
   * Will focus all viewports on this tileset.
   *
   * This is called when double-clicking the tileset in the World Outliner.
   * It will move the tileset into the center of the view, *even if* the
   * tileset was not visible before, and no geometry has been created yet
   * for the tileset: It solely operates on the tile bounding volume that
   * was given in the root tile.
   */
  void OnFocusEditorViewportOnThis();
#endif

private:
  Cesium3DTiles::Tileset* _pTileset;

  /**
   * Marks whether tileset should be updated in LoadTileset.
   * Default to true so that tileset is created on construction.
   */
  bool _tilesetIsDirty = true;

  // For debug output
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

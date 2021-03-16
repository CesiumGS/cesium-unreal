// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTiles/ViewState.h"
#include "CesiumCreditSystem.h"
#include "CesiumGeoreference.h"
#include "CesiumGeoreferenceable.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include <chrono>
#include <glm/mat4x4.hpp>
#include "ACesium3DTileset.generated.h"

class UMaterial;

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
   * Whether culled screen-space error should be enforced for culled tiles.
   *
   * When "Enable Fog Culling" is selected, this flag will indicate that the
   * screen space error for tiles that have been culled by fog
   * should still be enforced.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Tile Culling")
  bool EnforceCulledScreenSpaceError = false;

  /**
   * The screen-space error to be used for culled tiles.
   *
   * In contrast to the global maximum screen space error, this value
   * refers to the screen space error that should be used for tiles that
   * have been culled by fog. It will only be taken into account when
   * "Enforce Culled Screen Space Error" is enabled.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Tile Culling",
      meta = (EditCondition = "EnforceCulledScreenSpaceError"))
  double CulledScreenSpaceError = 64.0;

  UPROPERTY(EditAnywhere, Category = "Cesium|Rendering")
  UMaterial* Material = nullptr;

  /**
   * Pauses level-of-detail and culling updates of this tileset.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Debug")
  bool SuspendUpdate;

  /**
   * If true, this tileset is loaded and shown in the editor. If false, is only
   * shown while playing (including Play-in-Editor).
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|Debug")
  bool ShowInEditor = true;

  UFUNCTION(BlueprintCallable, Category = "Cesium|Rendering")
  void PlayMovieSequencer();

  UFUNCTION(BlueprintCallable, Category = "Cesium|Rendering")
  void StopMovieSequencer();

  UFUNCTION(BlueprintCallable, Category = "Cesium|Rendering")
  void PauseMovieSequencer();

  const glm::dmat4& GetCesiumTilesetToUnrealRelativeWorldTransform() const;

  Cesium3DTiles::Tileset* GetTileset() { return this->_pTileset; }
  const Cesium3DTiles::Tileset* GetTileset() const { return this->_pTileset; }

  virtual bool IsBoundingVolumeReady() const override;
  virtual std::optional<Cesium3DTiles::BoundingVolume>
  GetBoundingVolume() const override;
  void UpdateTransformFromCesium(const glm::dmat4& cesiumToUnreal);
  virtual void UpdateGeoreferenceTransform(
      const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform);

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

public:
  // Called every frame
  virtual bool ShouldTickIfViewportsOnly() const override;
  virtual void Tick(float DeltaTime) override;
  virtual void BeginDestroy() override;
  virtual void Destroyed() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
  Cesium3DTiles::Tileset* _pTileset;

  UMaterial* _lastMaterial = nullptr;

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
  int _beforeMovieLoadingDescendantLimit;
  bool _beforeMovieKeepWorldOriginNearCamera;
};

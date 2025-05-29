// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "Cesium3DTileset.h"
#include "Async/Async.h"
#include "Camera/CameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Cesium3DTilesSelection/EllipsoidTilesetLoader.h"
#include "Cesium3DTilesSelection/Tile.h"
#include "Cesium3DTilesSelection/TilesetLoadFailureDetails.h"
#include "Cesium3DTilesSelection/TilesetOptions.h"
#include "Cesium3DTilesSelection/TilesetSharedAssetSystem.h"
#include "Cesium3DTilesetLoadFailureDetails.h"
#include "Cesium3DTilesetRoot.h"
#include "CesiumActors.h"
#include "CesiumAsync/SharedAssetDepot.h"
#include "CesiumBoundingVolumeComponent.h"
#include "CesiumCamera.h"
#include "CesiumCameraManager.h"
#include "CesiumCommon.h"
#include "CesiumCustomVersion.h"
#include "CesiumGeospatial/GlobeTransforms.h"
#include "CesiumGltf/ImageAsset.h"
#include "CesiumGltf/Ktx2TranscodeTargets.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPointsSceneProxyUpdater.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumIonClient/Connection.h"
#include "CesiumRasterOverlay.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"
#include "CesiumTileExcluder.h"
#include "CesiumViewExtension.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "ICesiumObjectAtRelativeHeight.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Math/UnrealMathUtility.h"
#include "PixelFormat.h"
#include "StereoRendering.h"
#include "UnrealPrepareRendererResources.h"
#include "VecMath.h"
#include <Cesium3DTilesSelection/ITileSelectionEventReceiver.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <memory>
#include <spdlog/spdlog.h>

#ifdef CESIUM_DEBUG_TILE_STATES
#include "HAL/PlatformFileManager.h"
#include <Cesium3DTilesSelection/DebugTileStateDatabase.h>
#endif

FCesium3DTilesetLoadFailure OnCesium3DTilesetLoadFailure{};

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#include "FileHelpers.h"
#include "LevelEditorViewport.h"
#endif

// Avoid complaining about the deprecated metadata struct
PRAGMA_DISABLE_DEPRECATION_WARNINGS

// Sets default values
ACesium3DTileset::ACesium3DTileset()
    : AActor(),
      Georeference(nullptr),
      ResolvedGeoreference(nullptr),
      CreditSystem(nullptr),

      _pTileset(nullptr),

#ifdef CESIUM_DEBUG_TILE_STATES
      _pStateDebug(nullptr),
#endif

      _lastTilesRendered(0),
      _lastWorkerThreadTileLoadQueueLength(0),
      _lastMainThreadTileLoadQueueLength(0),

      _lastTilesVisited(0),
      _lastTilesCulled(0),
      _lastTilesOccluded(0),
      _lastTilesWaitingForOcclusionResults(0),
      _lastMaxDepthVisited(0),

      _captureMovieMode{false},
      _beforeMoviePreloadAncestors{PreloadAncestors},
      _beforeMoviePreloadSiblings{PreloadSiblings},
      _beforeMovieLoadingDescendantLimit{LoadingDescendantLimit},
      _beforeMovieUseLodTransitions{true},

      _tilesetsBeingDestroyed(0) {
  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickGroup = ETickingGroup::TG_PostUpdateWork;

#if WITH_EDITOR
  this->SetIsSpatiallyLoaded(false);
#endif

  this->SetActorEnableCollision(true);

  this->RootComponent =
      CreateDefaultSubobject<UCesium3DTilesetRoot>(TEXT("Tileset"));
  this->Root = this->RootComponent;

  PlatformName = UGameplayStatics::GetPlatformName();
}

ACesium3DTileset::~ACesium3DTileset() { this->DestroyTileset(); }
PRAGMA_ENABLE_DEPRECATION_WARNINGS

TSoftObjectPtr<ACesiumGeoreference> ACesium3DTileset::GetGeoreference() const {
  return this->Georeference;
}

void ACesium3DTileset::SetMobility(EComponentMobility::Type NewMobility) {
  if (NewMobility != this->RootComponent->Mobility) {
    this->RootComponent->SetMobility(NewMobility);
    DestroyTileset();
  }
}

void ACesium3DTileset::SampleHeightMostDetailed(
    const TArray<FVector>& LongitudeLatitudeHeightArray,
    FCesiumSampleHeightMostDetailedCallback OnHeightsSampled) {
  // It's possible to call this function before a Tick happens, so make sure
  // that the necessary variables are resolved.
  this->ResolveGeoreference();
  this->ResolveCameraManager();
  this->ResolveCreditSystem();

  if (this->_pTileset == nullptr) {
    this->LoadTileset();
  }

  std::vector<CesiumGeospatial::Cartographic> positions;
  positions.reserve(LongitudeLatitudeHeightArray.Num());

  for (const FVector& position : LongitudeLatitudeHeightArray) {
    positions.emplace_back(CesiumGeospatial::Cartographic::fromDegrees(
        position.X,
        position.Y,
        position.Z));
  }

  auto sampleHeights = [this, &positions]() mutable {
    if (this->_pTileset) {
      return this->_pTileset->sampleHeightMostDetailed(positions)
          .catchImmediately([positions = std::move(positions)](
                                std::exception&& exception) mutable {
            std::vector<bool> sampleSuccess(positions.size(), false);
            return Cesium3DTilesSelection::SampleHeightResult{
                std::move(positions),
                std::move(sampleSuccess),
                {exception.what()}};
          });
    } else {
      std::vector<bool> sampleSuccess(positions.size(), false);
      return getAsyncSystem().createResolvedFuture(
          Cesium3DTilesSelection::SampleHeightResult{
              std::move(positions),
              std::move(sampleSuccess),
              {"Could not sample heights from tileset because it has not "
               "been created."}});
    }
  };

  sampleHeights().thenImmediately(
      [this, OnHeightsSampled = std::move(OnHeightsSampled)](
          Cesium3DTilesSelection::SampleHeightResult&& result) {
        if (!IsValid(this))
          return;

        check(result.positions.size() == result.sampleSuccess.size());

        // This should do nothing, but will prevent undefined behavior if
        // the array sizes are unexpectedly different.
        result.sampleSuccess.resize(result.positions.size(), false);

        TArray<FCesiumSampleHeightResult> sampleHeightResults;
        sampleHeightResults.Reserve(result.positions.size());

        for (size_t i = 0; i < result.positions.size(); ++i) {
          const CesiumGeospatial::Cartographic& position = result.positions[i];

          FCesiumSampleHeightResult unrealResult;
          unrealResult.LongitudeLatitudeHeight = FVector(
              CesiumUtility::Math::radiansToDegrees(position.longitude),
              CesiumUtility::Math::radiansToDegrees(position.latitude),
              position.height);
          unrealResult.SampleSuccess = result.sampleSuccess[i];

          sampleHeightResults.Emplace(std::move(unrealResult));
        }

        TArray<FString> warnings;
        warnings.Reserve(result.warnings.size());

        for (const std::string& warning : result.warnings) {
          warnings.Emplace(UTF8_TO_TCHAR(warning.c_str()));
        }

        OnHeightsSampled.ExecuteIfBound(this, sampleHeightResults, warnings);
      });
}

namespace {

void AddToTile(
    ICesiumObjectAtRelativeHeight* Object,
    const Cesium3DTilesSelection::Tile* pTile) {
  if (!pTile)
    return;

  if (pTile->getState() != Cesium3DTilesSelection::TileLoadState::Done)
    return;

  const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
      pTile->getContent().getRenderContent();
  UCesiumGltfComponent* pGltf =
      static_cast<UCesiumGltfComponent*>(pRenderContent->getRenderResources());
  pGltf->AddObjectOnTile(Object);
}

void RemoveFromTile(
    ICesiumObjectAtRelativeHeight* Object,
    const Cesium3DTilesSelection::Tile* pTile) {
  if (!pTile)
    return;

  if (pTile->getState() != Cesium3DTilesSelection::TileLoadState::Done)
    return;

  const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
      pTile->getContent().getRenderContent();
  UCesiumGltfComponent* pGltf =
      static_cast<UCesiumGltfComponent*>(pRenderContent->getRenderResources());
  pGltf->RemoveObjectOnTile(Object);
}

} // namespace

std::optional<double> ACesium3DTileset::SampleCurrentHeight(
    ICesiumObjectAtRelativeHeight* Object,
    ACesium3DTileset::ObjectAtRelativeHeight& Details,
    double Longitude,
    double Latitude) {
  Details.longitude = Longitude;
  Details.latitude = Latitude;

  ACesiumGeoreference* Georeference = this->ResolveGeoreference();
  if (!Georeference) {
    RemoveFromTile(Object, Details.pCurrentTile);
    Details.pCurrentTile = nullptr;
    return std::nullopt;
  }

  Details.verticalLineHigh =
      Georeference->TransformLongitudeLatitudeHeightPositionToUnreal(FVector(
          Longitude,
          Latitude,
          Georeference->GetEllipsoid()->GetMaximumRadius() * 0.01));
  Details.verticalLineLow =
      Georeference->TransformLongitudeLatitudeHeightPositionToUnreal(FVector(
          Longitude,
          Latitude,
          -Georeference->GetEllipsoid()->GetMaximumRadius() * 0.01));

  FHitResult Hit{};
  FCollisionQueryParams Params{};
  if (this->ActorLineTraceSingle(
          Hit,
          Details.verticalLineHigh,
          Details.verticalLineLow,
          ECollisionChannel::ECC_Visibility,
          Params) &&
      Hit.Component.IsValid()) {
    UCesiumGltfComponent* Gltf =
        Cast<UCesiumGltfComponent>(Hit.Component->GetAttachParent());
    if (Gltf && Gltf->GetTile()) {
      double HitHeight =
          Georeference
              ->TransformUnrealPositionToLongitudeLatitudeHeight(Hit.Location)
              .Z;

      RemoveFromTile(Object, Details.pCurrentTile);
      Details.pCurrentTile = Gltf->GetTile();
      AddToTile(Object, Details.pCurrentTile);

      return std::make_optional(HitHeight);
    }
  }

  RemoveFromTile(Object, Details.pCurrentTile);
  Details.pCurrentTile = nullptr;
  return std::nullopt;
}

namespace {
struct Visitor {
  ICesiumObjectAtRelativeHeight* pObject;
  double longitude;
  double latitude;
  const FVector& Start;
  const FVector& End;

  bool operator()(const CesiumGeometry::BoundingSphere& bounds) {
    // TODO
    return false;
  }
  bool operator()(const CesiumGeometry::OrientedBoundingBox& bounds) {
    // TODO
    return false;
  }
  bool operator()(const CesiumGeospatial::S2CellBoundingVolume& bounds) {
    // TODO
    return false;
  }
  bool operator()(const CesiumGeospatial::BoundingRegion& bounds) {
    return bounds.getRectangle().contains(
        CesiumGeospatial::Cartographic::fromDegrees(longitude, latitude, 0.0));
  }
  bool operator()(
      const CesiumGeospatial::BoundingRegionWithLooseFittingHeights& bounds) {
    (*this)(bounds.getBoundingRegion());
  }
  bool operator()(const CesiumGeometry::BoundingCylinderRegion& bounds) {
    // TODO
    return false;
  }
};
} // namespace

void ACesium3DTileset::UpdateObjectsForNewRenderedTile(
    const Cesium3DTilesSelection::Tile& tile) {
  // check(tile.getState() == Cesium3DTilesSelection::TileLoadState::Done);
  if (tile.getState() != Cesium3DTilesSelection::TileLoadState::Done)
    return;

  const Cesium3DTilesSelection::BoundingVolume& boundingVolume =
      tile.getContentBoundingVolume().value_or(tile.getBoundingVolume());

  for (TPair<ICesiumObjectAtRelativeHeight*, ObjectAtRelativeHeight>& pair :
       this->_objectsAtRelativeHeight) {
    if (!std::visit(
            Visitor{
                pair.Key,
                pair.Value.longitude,
                pair.Value.latitude,
                pair.Value.verticalLineHigh,
                pair.Value.verticalLineLow},
            boundingVolume)) {
      // The tile's bounding volume indicates this object can't be in it.
      continue;
    }

    // Check for actual intersection.
    const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
        tile.getContent().getRenderContent();
    if (!pRenderContent)
      continue;

    UCesiumGltfComponent* pGltf = static_cast<UCesiumGltfComponent*>(
        pRenderContent->getRenderResources());

    bool hit = false;

    for (const TObjectPtr<USceneComponent>& pChild :
         pGltf->GetAttachChildren()) {
      UPrimitiveComponent* pPrimitive = Cast<UPrimitiveComponent>(pChild);
      FHitResult Hit{};
      FCollisionQueryParams Params{};
      if (pPrimitive->LineTraceComponent(
              Hit,
              pair.Value.verticalLineHigh,
              pair.Value.verticalLineLow,
              Params)) {
        hit = true;
        // break;
      }
      pPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    if (hit) {
      // If there's a hit, do a trace against the entire Tileset. It's possible
      // the object lies in this tile, but it also lies in another tile that is
      // above it.
      std::optional<double> maybeHeight = this->SampleCurrentHeight(
          pair.Key,
          pair.Value,
          pair.Value.longitude,
          pair.Value.latitude);

      // We've already established this object lies in a component, so if it
      // doesn't lie somewhere in the tileset, something has gone wrong.
      check(maybeHeight);

      if (maybeHeight) {
        pair.Key->OnSurfaceHeightChanged(*maybeHeight);
      }
    }
  }
}

std::optional<double> ACesium3DTileset::AddOrUpdateObjectAtRelativeHeight(
    ICesiumObjectAtRelativeHeight* Object,
    double Longitude,
    double Latitude) {
  ObjectAtRelativeHeight& Details =
      this->_objectsAtRelativeHeight.FindOrAdd(Object);
  return this->SampleCurrentHeight(Object, Details, Longitude, Latitude);
}

void ACesium3DTileset::RemoveObjectAtRelativeHeight(
    ICesiumObjectAtRelativeHeight* Object) {
  ObjectAtRelativeHeight* pDetails =
      this->_objectsAtRelativeHeight.Find(Object);
  if (pDetails) {
    RemoveFromTile(Object, pDetails->pCurrentTile);
    this->_objectsAtRelativeHeight.Remove(Object);
  }
}

void ACesium3DTileset::SetGeoreference(
    TSoftObjectPtr<ACesiumGeoreference> NewGeoreference) {
  this->Georeference = NewGeoreference;
  this->InvalidateResolvedGeoreference();
  this->ResolveGeoreference();
}

ACesiumGeoreference* ACesium3DTileset::ResolveGeoreference() {
  if (IsValid(this->ResolvedGeoreference)) {
    return this->ResolvedGeoreference;
  }

  if (IsValid(this->Georeference.Get())) {
    this->ResolvedGeoreference = this->Georeference.Get();
  } else {
    this->ResolvedGeoreference =
        ACesiumGeoreference::GetDefaultGeoreferenceForActor(this);
  }

  if (this->ResolvedGeoreference) {
    UCesium3DTilesetRoot* pRoot =
        Cast<UCesium3DTilesetRoot>(this->RootComponent);
    if (pRoot) {
      this->ResolvedGeoreference->OnGeoreferenceUpdated.AddUniqueDynamic(
          pRoot,
          &UCesium3DTilesetRoot::HandleGeoreferenceUpdated);
      this->ResolvedGeoreference->OnEllipsoidChanged.AddUniqueDynamic(
          this,
          &ACesium3DTileset::HandleOnGeoreferenceEllipsoidChanged);

      // Update existing tile positions, if any.
      pRoot->HandleGeoreferenceUpdated();
    }
  }

  return this->ResolvedGeoreference;
}

void ACesium3DTileset::InvalidateResolvedGeoreference() {
  if (IsValid(this->ResolvedGeoreference)) {
    this->ResolvedGeoreference->OnGeoreferenceUpdated.RemoveAll(
        this->RootComponent);
  }
  this->ResolvedGeoreference = nullptr;
}

TSoftObjectPtr<ACesiumCreditSystem> ACesium3DTileset::GetCreditSystem() const {
  return this->CreditSystem;
}

void ACesium3DTileset::SetCreditSystem(
    TSoftObjectPtr<ACesiumCreditSystem> NewCreditSystem) {
  this->CreditSystem = NewCreditSystem;
  this->InvalidateResolvedCreditSystem();
  this->ResolveCreditSystem();
}

ACesiumCreditSystem* ACesium3DTileset::ResolveCreditSystem() {
  if (IsValid(this->ResolvedCreditSystem)) {
    return this->ResolvedCreditSystem;
  }

  if (IsValid(this->CreditSystem.Get())) {
    this->ResolvedCreditSystem = this->CreditSystem.Get();
  } else {
    this->ResolvedCreditSystem =
        ACesiumCreditSystem::GetDefaultCreditSystem(this);
  }

  // Refresh the tileset so it uses the new credit system.
  this->RefreshTileset();

  return this->ResolvedCreditSystem;
}

void ACesium3DTileset::InvalidateResolvedCreditSystem() {
  this->ResolvedCreditSystem = nullptr;
  this->RefreshTileset();
}

TSoftObjectPtr<ACesiumCameraManager>
ACesium3DTileset::GetCameraManager() const {
  return this->CameraManager;
}

void ACesium3DTileset::SetCameraManager(
    TSoftObjectPtr<ACesiumCameraManager> NewCameraManager) {
  this->CameraManager = NewCameraManager;
  this->InvalidateResolvedCameraManager();
  this->ResolveCameraManager();
}

ACesiumCameraManager* ACesium3DTileset::ResolveCameraManager() {
  if (IsValid(this->ResolvedCameraManager)) {
    return this->ResolvedCameraManager;
  }

  if (IsValid(this->CameraManager.Get())) {
    this->ResolvedCameraManager = this->CameraManager.Get();
  } else {
    this->ResolvedCameraManager =
        ACesiumCameraManager::GetDefaultCameraManager(this);
  }

  return this->ResolvedCameraManager;
}

void ACesium3DTileset::InvalidateResolvedCameraManager() {
  this->ResolvedCameraManager = nullptr;
  this->RefreshTileset();
}

void ACesium3DTileset::RefreshTileset() { this->DestroyTileset(); }

void ACesium3DTileset::TroubleshootToken() {
  OnCesium3DTilesetIonTroubleshooting.Broadcast(this);
}

void ACesium3DTileset::AddFocusViewportDelegate() {
#if WITH_EDITOR
  FEditorDelegates::OnFocusViewportOnActors.AddLambda(
      [this](const TArray<AActor*>& actors) {
        if (actors.Num() == 1 && actors[0] == this) {
          this->OnFocusEditorViewportOnThis();
        }
      });
#endif // WITH_EDITOR
}

void ACesium3DTileset::PostInitProperties() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called PostInitProperties on actor %s"),
      *this->GetName());

  Super::PostInitProperties();

  AddFocusViewportDelegate();

  UCesiumRuntimeSettings* pSettings =
      GetMutableDefault<UCesiumRuntimeSettings>();
  if (pSettings) {
    CanEnableOcclusionCulling =
        pSettings->EnableExperimentalOcclusionCullingFeature;
#if WITH_EDITOR
    pSettings->OnSettingChanged().AddUObject(
        this,
        &ACesium3DTileset::RuntimeSettingsChanged);
#endif
  }
}

void ACesium3DTileset::SetUseLodTransitions(bool InUseLodTransitions) {
  if (InUseLodTransitions != this->UseLodTransitions) {
    this->UseLodTransitions = InUseLodTransitions;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetTilesetSource(ETilesetSource InSource) {
  if (InSource != this->TilesetSource) {
    this->DestroyTileset();
    this->TilesetSource = InSource;
  }
}

namespace {

bool MapsAreEqual(
    const TMap<FString, FString>& Lhs,
    const TMap<FString, FString>& Rhs) {
  if (Lhs.Num() != Rhs.Num()) {
    return false;
  }

  for (const auto& [Key, Value] : Lhs) {
    const FString* RhsVal = Rhs.Find(Key);
    if (!RhsVal || *RhsVal != Value) {
      return false;
    }
  }

  return true;
}

} // namespace

void ACesium3DTileset::SetRequestHeaders(
    const TMap<FString, FString>& InRequestHeaders) {
  if (!MapsAreEqual(InRequestHeaders, this->RequestHeaders)) {
    this->DestroyTileset();
    this->RequestHeaders = InRequestHeaders;
  }
}

void ACesium3DTileset::SetUrl(const FString& InUrl) {
  if (InUrl != this->Url) {
    if (this->TilesetSource == ETilesetSource::FromUrl) {
      this->DestroyTileset();
    }
    this->Url = InUrl;
  }
}

void ACesium3DTileset::SetIonAssetID(int64 InAssetID) {
  if (InAssetID >= 0 && InAssetID != this->IonAssetID) {
    if (this->TilesetSource == ETilesetSource::FromCesiumIon) {
      this->DestroyTileset();
    }
    this->IonAssetID = InAssetID;
  }
}

void ACesium3DTileset::SetIonAccessToken(const FString& InAccessToken) {
  if (this->IonAccessToken != InAccessToken) {
    if (this->TilesetSource == ETilesetSource::FromCesiumIon) {
      this->DestroyTileset();
    }
    this->IonAccessToken = InAccessToken;
  }
}

void ACesium3DTileset::SetCesiumIonServer(UCesiumIonServer* Server) {
  if (this->CesiumIonServer != Server) {
    if (this->TilesetSource == ETilesetSource::FromCesiumIon) {
      this->DestroyTileset();
    }
    this->CesiumIonServer = Server;
  }
}

void ACesium3DTileset::SetMaximumScreenSpaceError(
    double InMaximumScreenSpaceError) {
  if (MaximumScreenSpaceError != InMaximumScreenSpaceError) {
    MaximumScreenSpaceError = InMaximumScreenSpaceError;
    FCesiumGltfPointsSceneProxyUpdater::UpdateSettingsInProxies(this);
  }
}

bool ACesium3DTileset::GetEnableOcclusionCulling() const {
  return GetDefault<UCesiumRuntimeSettings>()
             ->EnableExperimentalOcclusionCullingFeature &&
         EnableOcclusionCulling;
}

void ACesium3DTileset::SetEnableOcclusionCulling(bool bEnableOcclusionCulling) {
  if (this->EnableOcclusionCulling != bEnableOcclusionCulling) {
    this->EnableOcclusionCulling = bEnableOcclusionCulling;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetOcclusionPoolSize(int32 newOcclusionPoolSize) {
  if (this->OcclusionPoolSize != newOcclusionPoolSize) {
    this->OcclusionPoolSize = newOcclusionPoolSize;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetDelayRefinementForOcclusion(
    bool bDelayRefinementForOcclusion) {
  if (this->DelayRefinementForOcclusion != bDelayRefinementForOcclusion) {
    this->DelayRefinementForOcclusion = bDelayRefinementForOcclusion;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetCreatePhysicsMeshes(bool bCreatePhysicsMeshes) {
  if (this->CreatePhysicsMeshes != bCreatePhysicsMeshes) {
    this->CreatePhysicsMeshes = bCreatePhysicsMeshes;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetCreateNavCollision(bool bCreateNavCollision) {
  if (this->CreateNavCollision != bCreateNavCollision) {
    this->CreateNavCollision = bCreateNavCollision;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetAlwaysIncludeTangents(bool bAlwaysIncludeTangents) {
  if (this->AlwaysIncludeTangents != bAlwaysIncludeTangents) {
    this->AlwaysIncludeTangents = bAlwaysIncludeTangents;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetGenerateSmoothNormals(bool bGenerateSmoothNormals) {
  if (this->GenerateSmoothNormals != bGenerateSmoothNormals) {
    this->GenerateSmoothNormals = bGenerateSmoothNormals;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetEnableWaterMask(bool bEnableMask) {
  if (this->EnableWaterMask != bEnableMask) {
    this->EnableWaterMask = bEnableMask;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetIgnoreKhrMaterialsUnlit(
    bool bIgnoreKhrMaterialsUnlit) {
  if (this->IgnoreKhrMaterialsUnlit != bIgnoreKhrMaterialsUnlit) {
    this->IgnoreKhrMaterialsUnlit = bIgnoreKhrMaterialsUnlit;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetMaterial(UMaterialInterface* InMaterial) {
  if (this->Material != InMaterial) {
    this->Material = InMaterial;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetTranslucentMaterial(UMaterialInterface* InMaterial) {
  if (this->TranslucentMaterial != InMaterial) {
    this->TranslucentMaterial = InMaterial;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetWaterMaterial(UMaterialInterface* InMaterial) {
  if (this->WaterMaterial != InMaterial) {
    this->WaterMaterial = InMaterial;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetCustomDepthParameters(
    FCustomDepthParameters InCustomDepthParameters) {
  if (this->CustomDepthParameters != InCustomDepthParameters) {
    this->CustomDepthParameters = InCustomDepthParameters;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetPointCloudShading(
    FCesiumPointCloudShading InPointCloudShading) {
  if (PointCloudShading != InPointCloudShading) {
    PointCloudShading = InPointCloudShading;
    FCesiumGltfPointsSceneProxyUpdater::UpdateSettingsInProxies(this);
  }
}

void ACesium3DTileset::SetRuntimeVirtualTextures(
    TArray<URuntimeVirtualTexture*> InRuntimeVirtualTextures) {
  if (this->RuntimeVirtualTextures != InRuntimeVirtualTextures) {
    this->RuntimeVirtualTextures = InRuntimeVirtualTextures;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetTranslucencySortPriority(
    int32 InTranslucencySortPriority) {
  if (this->TranslucencySortPriority != InTranslucencySortPriority) {
    this->TranslucencySortPriority = InTranslucencySortPriority;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::PlayMovieSequencer() {
  this->_beforeMoviePreloadAncestors = this->PreloadAncestors;
  this->_beforeMoviePreloadSiblings = this->PreloadSiblings;
  this->_beforeMovieLoadingDescendantLimit = this->LoadingDescendantLimit;
  this->_beforeMovieUseLodTransitions = this->UseLodTransitions;

  this->_captureMovieMode = true;
  this->PreloadAncestors = false;
  this->PreloadSiblings = false;
  this->LoadingDescendantLimit = 10000;
  this->UseLodTransitions = false;
}

void ACesium3DTileset::StopMovieSequencer() {
  this->_captureMovieMode = false;
  this->PreloadAncestors = this->_beforeMoviePreloadAncestors;
  this->PreloadSiblings = this->_beforeMoviePreloadSiblings;
  this->LoadingDescendantLimit = this->_beforeMovieLoadingDescendantLimit;
  this->UseLodTransitions = this->_beforeMovieUseLodTransitions;
}

void ACesium3DTileset::PauseMovieSequencer() { this->StopMovieSequencer(); }

#if WITH_EDITOR
void ACesium3DTileset::OnFocusEditorViewportOnThis() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called OnFocusEditorViewportOnThis on actor %s"),
      *this->GetName());

  struct CalculateECEFCameraPosition {
    const CesiumGeospatial::Ellipsoid& ellipsoid;

    glm::dvec3 operator()(const CesiumGeometry::BoundingSphere& sphere) {
      const glm::dvec3& center = sphere.getCenter();
      glm::dmat4 ENU =
          CesiumGeospatial::GlobeTransforms::eastNorthUpToFixedFrame(
              center,
              ellipsoid);
      glm::dvec3 offset =
          sphere.getRadius() *
          glm::normalize(
              glm::dvec3(ENU[0]) + glm::dvec3(ENU[1]) + glm::dvec3(ENU[2]));
      glm::dvec3 position = center + offset;
      return position;
    }

    glm::dvec3
    operator()(const CesiumGeometry::OrientedBoundingBox& orientedBoundingBox) {
      const glm::dvec3& center = orientedBoundingBox.getCenter();
      glm::dmat4 ENU =
          CesiumGeospatial::GlobeTransforms::eastNorthUpToFixedFrame(
              center,
              ellipsoid);
      const glm::dmat3& halfAxes = orientedBoundingBox.getHalfAxes();
      glm::dvec3 offset =
          glm::length(halfAxes[0] + halfAxes[1] + halfAxes[2]) *
          glm::normalize(
              glm::dvec3(ENU[0]) + glm::dvec3(ENU[1]) + glm::dvec3(ENU[2]));
      glm::dvec3 position = center + offset;
      return position;
    }

    glm::dvec3
    operator()(const CesiumGeospatial::BoundingRegion& boundingRegion) {
      return (*this)(boundingRegion.getBoundingBox());
    }

    glm::dvec3
    operator()(const CesiumGeospatial::BoundingRegionWithLooseFittingHeights&
                   boundingRegionWithLooseFittingHeights) {
      return (*this)(boundingRegionWithLooseFittingHeights.getBoundingRegion()
                         .getBoundingBox());
    }

    glm::dvec3 operator()(const CesiumGeospatial::S2CellBoundingVolume& s2) {
      return (*this)(s2.computeBoundingRegion());
    }

    glm::dvec3
    operator()(const CesiumGeometry::BoundingCylinderRegion& cylinder) {
      return (*this)(cylinder.toOrientedBoundingBox());
    }
  };

  const Cesium3DTilesSelection::Tile* pRootTile =
      this->_pTileset->getRootTile();
  if (!pRootTile) {
    return;
  }

  const Cesium3DTilesSelection::BoundingVolume& boundingVolume =
      pRootTile->getBoundingVolume();

  ACesiumGeoreference* pGeoreference = this->ResolveGeoreference();

  const CesiumGeospatial::Ellipsoid& ellipsoid =
      pGeoreference->GetEllipsoid()->GetNativeEllipsoid();

  // calculate unreal camera position
  glm::dvec3 ecefCameraPosition =
      std::visit(CalculateECEFCameraPosition{ellipsoid}, boundingVolume);
  FVector unrealCameraPosition =
      pGeoreference->TransformEarthCenteredEarthFixedPositionToUnreal(
          VecMath::createVector(ecefCameraPosition));

  // calculate unreal camera orientation
  glm::dvec3 ecefCenter =
      Cesium3DTilesSelection::getBoundingVolumeCenter(boundingVolume);
  FVector unrealCenter =
      pGeoreference->TransformEarthCenteredEarthFixedPositionToUnreal(
          VecMath::createVector(ecefCenter));
  FVector unrealCameraFront =
      (unrealCenter - unrealCameraPosition).GetSafeNormal();
  FVector unrealCameraRight =
      FVector::CrossProduct(FVector::ZAxisVector, unrealCameraFront)
          .GetSafeNormal();
  FVector unrealCameraUp =
      FVector::CrossProduct(unrealCameraFront, unrealCameraRight)
          .GetSafeNormal();
  FRotator cameraRotator = FMatrix(
                               unrealCameraFront,
                               unrealCameraRight,
                               unrealCameraUp,
                               FVector::ZeroVector)
                               .Rotator();

  // Update all viewports.
  for (FLevelEditorViewportClient* LinkedViewportClient :
       GEditor->GetLevelViewportClients()) {
    // Dont move camera attach to an actor
    if (!LinkedViewportClient->IsAnyActorLocked()) {
      FViewportCameraTransform& ViewTransform =
          LinkedViewportClient->GetViewTransform();
      LinkedViewportClient->SetViewRotation(cameraRotator);
      LinkedViewportClient->SetViewLocation(unrealCameraPosition);
      LinkedViewportClient->Invalidate();
    }
  }
}
#endif

const glm::dmat4&
ACesium3DTileset::GetCesiumTilesetToUnrealRelativeWorldTransform() const {
  return Cast<UCesium3DTilesetRoot>(this->RootComponent)
      ->GetCesiumTilesetToUnrealRelativeWorldTransform();
}

void ACesium3DTileset::UpdateTransformFromCesium() {
  const glm::dmat4& CesiumToUnreal =
      this->GetCesiumTilesetToUnrealRelativeWorldTransform();
  TArray<UCesiumGltfComponent*> gltfComponents;
  this->GetComponents<UCesiumGltfComponent>(gltfComponents);

  for (UCesiumGltfComponent* pGltf : gltfComponents) {
    pGltf->UpdateTransformFromCesium(CesiumToUnreal);
  }

  if (this->BoundingVolumePoolComponent) {
    this->BoundingVolumePoolComponent->UpdateTransformFromCesium(
        CesiumToUnreal);
  }
}

void ACesium3DTileset::HandleOnGeoreferenceEllipsoidChanged(
    UCesiumEllipsoid* OldEllipsoid,
    UCesiumEllipsoid* NewEllpisoid) {
  UE_LOG(LogCesium, Warning, TEXT("Ellipsoid changed"));
  this->RefreshTileset();
}

// Called when the game starts or when spawned
void ACesium3DTileset::BeginPlay() {
  Super::BeginPlay();

  this->ResolveGeoreference();
  this->ResolveCameraManager();
  this->ResolveCreditSystem();

  this->LoadTileset();

  // Search for level sequence.
  for (auto sequenceActorIt = TActorIterator<ALevelSequenceActor>(GetWorld());
       sequenceActorIt;
       ++sequenceActorIt) {
    ALevelSequenceActor* sequenceActor = *sequenceActorIt;

    if (!IsValid(sequenceActor->GetSequencePlayer())) {
      continue;
    }

    FScriptDelegate playMovieSequencerDelegate;
    playMovieSequencerDelegate.BindUFunction(this, FName("PlayMovieSequencer"));
    sequenceActor->GetSequencePlayer()->OnPlay.Add(playMovieSequencerDelegate);

    FScriptDelegate stopMovieSequencerDelegate;
    stopMovieSequencerDelegate.BindUFunction(this, FName("StopMovieSequencer"));
    sequenceActor->GetSequencePlayer()->OnStop.Add(stopMovieSequencerDelegate);

    FScriptDelegate pauseMovieSequencerDelegate;
    pauseMovieSequencerDelegate.BindUFunction(
        this,
        FName("PauseMovieSequencer"));
    sequenceActor->GetSequencePlayer()->OnPause.Add(
        pauseMovieSequencerDelegate);
  }
}

void ACesium3DTileset::OnConstruction(const FTransform& Transform) {
  this->ResolveGeoreference();
  this->ResolveCameraManager();
  this->ResolveCreditSystem();

  this->LoadTileset();

  // Hide all existing tiles. The still-visible ones will be shown next time we
  // tick. But if update is suspended, leave the components in their current
  // state.
  if (!this->SuspendUpdate) {
    TArray<UCesiumGltfComponent*> gltfComponents;
    this->GetComponents<UCesiumGltfComponent>(gltfComponents);

    for (UCesiumGltfComponent* pGltf : gltfComponents) {
      if (pGltf && IsValid(pGltf) && pGltf->IsVisible()) {
        pGltf->SetVisibility(false, true);
        pGltf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
      }
    }
  }
}

void ACesium3DTileset::NotifyHit(
    UPrimitiveComponent* MyComp,
    AActor* Other,
    UPrimitiveComponent* OtherComp,
    bool bSelfMoved,
    FVector HitLocation,
    FVector HitNormal,
    FVector NormalImpulse,
    const FHitResult& Hit) {
  // std::cout << "Hit face index: " << Hit.FaceIndex << std::endl;

  // FHitResult detailedHit;
  // FCollisionQueryParams params;
  // params.bReturnFaceIndex = true;
  // params.bTraceComplex = true;
  // MyComp->LineTraceComponent(detailedHit, Hit.TraceStart, Hit.TraceEnd,
  // params);

  // std::cout << "Hit face index 2: " << detailedHit.FaceIndex << std::endl;
}

void ACesium3DTileset::UpdateLoadStatus() {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::UpdateLoadStatus)

  float nativeLoadProgress = this->_pTileset->computeLoadProgress();

  // If native tileset still loading, just copy its progress
  if (nativeLoadProgress < 100) {
    this->LoadProgress = nativeLoadProgress;
    return;
  }

  // Native tileset is 100% loaded, but there might be a few frames where
  // nothing needs to be loaded as we are waiting for occlusion results to come
  // back, which means we are not done with loading all the tiles in the tileset
  // yet. Interpret this as 99% (almost) done
  if (this->_lastTilesWaitingForOcclusionResults > 0) {
    this->LoadProgress = 99;
    return;
  }

  // If we have tiles to hide next frame, we haven't completely finished loading
  // yet. We need to tick once more. We're really close to done.
  if (!this->_tilesToHideNextFrame.empty()) {
    this->LoadProgress = glm::min(this->LoadProgress, 99.9999f);
    return;
  }

  // We can now report 100 percent loaded
  float lastLoadProgress = this->LoadProgress;
  this->LoadProgress = 100;

  // Only broadcast the update when we first hit 100%, not everytime
  if (lastLoadProgress != LoadProgress) {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::BroadcastOnTilesetLoaded)

    // Tileset just finished loading, we broadcast the update
    UE_LOG(LogCesium, Verbose, TEXT("Broadcasting OnTileLoaded"));
    OnTilesetLoaded.Broadcast();
  }
}

namespace {
const TSharedRef<CesiumViewExtension, ESPMode::ThreadSafe>&
getCesiumViewExtension() {
  static TSharedRef<CesiumViewExtension, ESPMode::ThreadSafe>
      cesiumViewExtension =
          GEngine->ViewExtensions->NewExtension<CesiumViewExtension>();
  return cesiumViewExtension;
}
} // namespace

void ACesium3DTileset::LoadTileset() {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::LoadTileset)

  if (this->_pTileset) {
    // Tileset already loaded, do nothing.
    return;
  }

  UWorld* pWorld = this->GetWorld();
  if (!pWorld) {
    return;
  }

  AWorldSettings* pWorldSettings = pWorld->GetWorldSettings();
  if (pWorldSettings && pWorldSettings->bEnableWorldBoundsChecks) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "\"Enable World Bounds Checks\" in the world settings is currently enabled. Please consider disabling it to avoid potential issues."),
        *this->Url);
  }

  // Make sure we have a valid Cesium ion server if we need one.
  if (this->TilesetSource == ETilesetSource::FromCesiumIon &&
      !IsValid(this->CesiumIonServer)) {
    this->Modify();
    this->CesiumIonServer = UCesiumIonServer::GetServerForNewObjects();
  }

  const TSharedRef<CesiumViewExtension, ESPMode::ThreadSafe>&
      cesiumViewExtension = getCesiumViewExtension();
  const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor =
      getAssetAccessor();
  const CesiumAsync::AsyncSystem& asyncSystem = getAsyncSystem();

  // Both the feature flag and the CesiumViewExtension are global, not owned by
  // the Tileset. We're just applying one to the other here out of convenience.
  cesiumViewExtension->SetEnabled(
      GetDefault<UCesiumRuntimeSettings>()
          ->EnableExperimentalOcclusionCullingFeature);

  TArray<UCesiumRasterOverlay*> rasterOverlays;
  this->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

  TArray<UCesiumTileExcluder*> tileExcluders;
  this->GetComponents<UCesiumTileExcluder>(tileExcluders);

  const UCesiumFeaturesMetadataComponent* pFeaturesMetadataComponent =
      this->FindComponentByClass<UCesiumFeaturesMetadataComponent>();

  // Check if this component exists for backwards compatibility.
  PRAGMA_DISABLE_DEPRECATION_WARNINGS

  const UDEPRECATED_CesiumEncodedMetadataComponent* pEncodedMetadataComponent =
      this->FindComponentByClass<UDEPRECATED_CesiumEncodedMetadataComponent>();

  this->_featuresMetadataDescription = std::nullopt;
  this->_metadataDescription_DEPRECATED = std::nullopt;

  if (pFeaturesMetadataComponent) {
    this->_featuresMetadataDescription =
        pFeaturesMetadataComponent->Description;
  } else if (pEncodedMetadataComponent) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "CesiumEncodedMetadataComponent is deprecated. Use CesiumFeaturesMetadataComponent instead."));
    this->_metadataDescription_DEPRECATED = {
        pEncodedMetadataComponent->FeatureTables,
        pEncodedMetadataComponent->FeatureTextures};
  }

  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  this->_cesiumViewExtension = cesiumViewExtension;

  if (GetDefault<UCesiumRuntimeSettings>()
          ->EnableExperimentalOcclusionCullingFeature &&
      this->EnableOcclusionCulling && !this->BoundingVolumePoolComponent) {
    const glm::dmat4& cesiumToUnreal =
        GetCesiumTilesetToUnrealRelativeWorldTransform();
    this->BoundingVolumePoolComponent =
        NewObject<UCesiumBoundingVolumePoolComponent>(this);
    this->BoundingVolumePoolComponent->SetFlags(
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
    this->BoundingVolumePoolComponent->RegisterComponent();
    this->BoundingVolumePoolComponent->UpdateTransformFromCesium(
        cesiumToUnreal);
  }

  if (this->BoundingVolumePoolComponent) {
    this->BoundingVolumePoolComponent->initPool(this->OcclusionPoolSize);
  }

  CesiumGeospatial::Ellipsoid pNativeEllipsoid =
      this->ResolveGeoreference()->GetEllipsoid()->GetNativeEllipsoid();

  ACesiumCreditSystem* pCreditSystem = this->ResolvedCreditSystem;

  Cesium3DTilesSelection::TilesetExternals externals{
      pAssetAccessor,
      std::make_shared<UnrealPrepareRendererResources>(this),
      asyncSystem,
      pCreditSystem ? pCreditSystem->GetExternalCreditSystem() : nullptr,
      spdlog::default_logger(),
      (GetDefault<UCesiumRuntimeSettings>()
           ->EnableExperimentalOcclusionCullingFeature &&
       this->EnableOcclusionCulling && this->BoundingVolumePoolComponent)
          ? this->BoundingVolumePoolComponent->getPool()
          : nullptr};

  this->_startTime = std::chrono::high_resolution_clock::now();

  this->LoadProgress = 0;

  Cesium3DTilesSelection::TilesetOptions options;

  options.ellipsoid = pNativeEllipsoid;

  options.enableOcclusionCulling =
      GetDefault<UCesiumRuntimeSettings>()
          ->EnableExperimentalOcclusionCullingFeature &&
      this->EnableOcclusionCulling;
  options.delayRefinementForOcclusion = this->DelayRefinementForOcclusion;

  options.showCreditsOnScreen = ShowCreditsOnScreen;

  options.loadErrorCallback =
      [this](const Cesium3DTilesSelection::TilesetLoadFailureDetails& details) {
        static_assert(
            uint8_t(ECesium3DTilesetLoadType::CesiumIon) ==
            uint8_t(Cesium3DTilesSelection::TilesetLoadType::CesiumIon));
        static_assert(
            uint8_t(ECesium3DTilesetLoadType::TilesetJson) ==
            uint8_t(Cesium3DTilesSelection::TilesetLoadType::TilesetJson));
        static_assert(
            uint8_t(ECesium3DTilesetLoadType::Unknown) ==
            uint8_t(Cesium3DTilesSelection::TilesetLoadType::Unknown));

        uint8_t typeValue = uint8_t(details.type);
        assert(
            uint8_t(details.type) <=
            uint8_t(Cesium3DTilesSelection::TilesetLoadType::TilesetJson));
        assert(this->_pTileset == details.pTileset);

        FCesium3DTilesetLoadFailureDetails ueDetails{};
        ueDetails.Tileset = this;
        ueDetails.Type = ECesium3DTilesetLoadType(typeValue);
        ueDetails.HttpStatusCode = details.statusCode;
        ueDetails.Message = UTF8_TO_TCHAR(details.message.c_str());

        // Broadcast the event from the game thread.
        // Even if we're already in the game thread, let the stack unwind.
        // Otherwise actions that destroy the Tileset will cause a deadlock.
        AsyncTask(
            ENamedThreads::GameThread,
            [ueDetails = std::move(ueDetails)]() {
              OnCesium3DTilesetLoadFailure.Broadcast(ueDetails);
            });
      };

  // Generous per-frame time limits for loading / unloading on main thread.
  options.mainThreadLoadingTimeLimit = 5.0;
  options.tileCacheUnloadTimeLimit = 5.0;

  options.contentOptions.generateMissingNormalsSmooth =
      this->GenerateSmoothNormals;

  // TODO: figure out why water material crashes mac
#if PLATFORM_MAC
#else
  options.contentOptions.enableWaterMask = this->EnableWaterMask;
#endif

  CesiumGltf::SupportedGpuCompressedPixelFormats supportedFormats;
  supportedFormats.ETC1_RGB = GPixelFormats[EPixelFormat::PF_ETC1].Supported;
  supportedFormats.ETC2_RGBA =
      GPixelFormats[EPixelFormat::PF_ETC2_RGBA].Supported;
  supportedFormats.BC1_RGB = GPixelFormats[EPixelFormat::PF_DXT1].Supported;
  supportedFormats.BC3_RGBA = GPixelFormats[EPixelFormat::PF_DXT5].Supported;
  supportedFormats.BC4_R = GPixelFormats[EPixelFormat::PF_BC4].Supported;
  supportedFormats.BC5_RG = GPixelFormats[EPixelFormat::PF_BC5].Supported;
  supportedFormats.BC7_RGBA = GPixelFormats[EPixelFormat::PF_BC7].Supported;
  supportedFormats.ASTC_4x4_RGBA =
      GPixelFormats[EPixelFormat::PF_ASTC_4x4].Supported;
  supportedFormats.PVRTC2_4_RGBA =
      GPixelFormats[EPixelFormat::PF_PVRTC2].Supported;
  supportedFormats.ETC2_EAC_R11 =
      GPixelFormats[EPixelFormat::PF_ETC2_R11_EAC].Supported;
  supportedFormats.ETC2_EAC_RG11 =
      GPixelFormats[EPixelFormat::PF_ETC2_RG11_EAC].Supported;

  options.contentOptions.ktx2TranscodeTargets =
      CesiumGltf::Ktx2TranscodeTargets(supportedFormats, false);

  options.contentOptions.applyTextureTransform = false;

  options.requestHeaders.reserve(this->RequestHeaders.Num());

  for (const auto& [Key, Value] : this->RequestHeaders) {
    options.requestHeaders.emplace_back(CesiumAsync::IAssetAccessor::THeader{
        TCHAR_TO_UTF8(*Key),
        TCHAR_TO_UTF8(*Value)});
  }

  switch (this->TilesetSource) {
  case ETilesetSource::FromEllipsoid:
    UE_LOG(LogCesium, Log, TEXT("Loading tileset from ellipsoid"));
    this->_pTileset = TUniquePtr<Cesium3DTilesSelection::Tileset>(
        Cesium3DTilesSelection::EllipsoidTilesetLoader::createTileset(
            externals,
            options)
            .release());
    break;
  case ETilesetSource::FromUrl:
    UE_LOG(LogCesium, Log, TEXT("Loading tileset from URL %s"), *this->Url);
    this->_pTileset = MakeUnique<Cesium3DTilesSelection::Tileset>(
        externals,
        TCHAR_TO_UTF8(*this->Url),
        options);
    break;
  case ETilesetSource::FromCesiumIon:
    UE_LOG(
        LogCesium,
        Log,
        TEXT("Loading tileset for asset ID %d"),
        this->IonAssetID);
    FString token = this->IonAccessToken.IsEmpty()
                        ? this->CesiumIonServer->DefaultIonAccessToken
                        : this->IonAccessToken;

#if WITH_EDITOR
    this->CesiumIonServer->ResolveApiUrl();
#endif

    std::string ionAssetEndpointUrl =
        TCHAR_TO_UTF8(*this->CesiumIonServer->ApiUrl);

    if (!ionAssetEndpointUrl.empty()) {
      // Make sure the URL ends with a slash
      if (!ionAssetEndpointUrl.empty() && *ionAssetEndpointUrl.rbegin() != '/')
        ionAssetEndpointUrl += '/';

      this->_pTileset = MakeUnique<Cesium3DTilesSelection::Tileset>(
          externals,
          static_cast<uint32_t>(this->IonAssetID),
          TCHAR_TO_UTF8(*token),
          options,
          ionAssetEndpointUrl);
    }
    break;
  }

  class TestEventReceiver
      : public Cesium3DTilesSelection::ITileSelectionEventReceiver {
  public:
    TestEventReceiver(ACesium3DTileset* pTileset) : _pTileset(pTileset) {}

    virtual void tileVisible(
        const Cesium3DTilesSelection::Tile& tile,
        const Cesium3DTilesSelection::TileSelectionState& previousState,
        const Cesium3DTilesSelection::TileSelectionState& currentState)
        override {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("Tile Visible (%s -> %s): %s"),
          lookup(previousState.getResult()),
          lookup(currentState.getResult()),
          UTF8_TO_TCHAR(
              Cesium3DTilesSelection::TileIdUtilities::createTileIdString(
                  tile.getTileID())
                  .c_str()));

      if (currentState.getResult() ==
          Cesium3DTilesSelection::TileSelectionState::Result::Rendered) {
        this->_pTileset->UpdateObjectsForNewRenderedTile(tile);
      }
    }

    virtual void tileCulled(
        const Cesium3DTilesSelection::Tile& tile,
        const Cesium3DTilesSelection::TileSelectionState& previousState,
        const Cesium3DTilesSelection::TileSelectionState& currentState)
        override {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("Tile Culled (%s -> %s): %s"),
          lookup(previousState.getResult()),
          lookup(currentState.getResult()),
          UTF8_TO_TCHAR(
              Cesium3DTilesSelection::TileIdUtilities::createTileIdString(
                  tile.getTileID())
                  .c_str()));
    }

    virtual void tileRefined(
        const Cesium3DTilesSelection::Tile& tile,
        const Cesium3DTilesSelection::TileSelectionState& previousState,
        const Cesium3DTilesSelection::TileSelectionState& currentState,
        const std::span<Cesium3DTilesSelection::Tile*>& newRenderedTiles)
        override {
      FString newRenderedTileIds;
      for (size_t i = 0; i < newRenderedTiles.size(); ++i) {
        newRenderedTileIds += TEXT("\n - ");
        newRenderedTileIds += UTF8_TO_TCHAR(
            Cesium3DTilesSelection::TileIdUtilities::createTileIdString(
                newRenderedTiles[i]->getTileID())
                .c_str());
      }

      UE_LOG(
          LogCesium,
          Warning,
          TEXT("Tile Refined (%s -> %s): %s refined to:%s"),
          lookup(previousState.getResult()),
          lookup(currentState.getResult()),
          UTF8_TO_TCHAR(
              Cesium3DTilesSelection::TileIdUtilities::createTileIdString(
                  tile.getTileID())
                  .c_str()),
          *newRenderedTileIds);

      for (size_t i = 0; i < newRenderedTiles.size(); ++i) {
        this->_pTileset->UpdateObjectsForNewRenderedTile(*newRenderedTiles[i]);
      }
    }

    virtual void tileCoarsened(
        const Cesium3DTilesSelection::Tile& tile,
        const Cesium3DTilesSelection::TileSelectionState& previousState,
        const Cesium3DTilesSelection::TileSelectionState& currentState,
        const Cesium3DTilesSelection::Tile& newRenderedTile) override {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("Tile Coarsened (%s -> %s): %s coarsened to %s"),
          lookup(previousState.getResult()),
          lookup(currentState.getResult()),
          UTF8_TO_TCHAR(
              Cesium3DTilesSelection::TileIdUtilities::createTileIdString(
                  tile.getTileID())
                  .c_str()),
          UTF8_TO_TCHAR(
              Cesium3DTilesSelection::TileIdUtilities::createTileIdString(
                  newRenderedTile.getTileID())
                  .c_str()));

      this->_pTileset->UpdateObjectsForNewRenderedTile(newRenderedTile);
    }

  private:
    const TCHAR*
    lookup(Cesium3DTilesSelection::TileSelectionState::Result state) {
      switch (state) {
      case Cesium3DTilesSelection::TileSelectionState::Result::None:
        return TEXT("None");
      case Cesium3DTilesSelection::TileSelectionState::Result::Culled:
        return TEXT("Culled");
      case Cesium3DTilesSelection::TileSelectionState::Result::Rendered:
        return TEXT("Rendered");
      case Cesium3DTilesSelection::TileSelectionState::Result::Refined:
        return TEXT("Refined");
      case Cesium3DTilesSelection::TileSelectionState::Result::
          RenderedAndKicked:
        return TEXT("RenderedAndKicked");
      case Cesium3DTilesSelection::TileSelectionState::Result::RefinedAndKicked:
        return TEXT("RefinedAndKicked");
      default:
        return TEXT("Unknown");
      }
    }

    ACesium3DTileset* _pTileset;
  };

  this->_pTileset->getDefaultViewGroup().setEventReceiver(
      std::make_shared<TestEventReceiver>(this));

#ifdef CESIUM_DEBUG_TILE_STATES
  FString dbDirectory = FPaths::Combine(
      FPaths::ProjectSavedDir(),
      TEXT("CesiumDebugTileStateDatabase"));

  IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
  if (!PlatformFile.DirectoryExists(*dbDirectory)) {
    PlatformFile.CreateDirectory(*dbDirectory);
  }

  FString dbFile =
      FPaths::Combine(dbDirectory, this->GetName() + TEXT(".sqlite"));
  this->_pStateDebug =
      MakeUnique<Cesium3DTilesSelection::DebugTileStateDatabase>(
          TCHAR_TO_UTF8(*dbFile));
#endif

  for (UCesiumRasterOverlay* pOverlay : rasterOverlays) {
    if (pOverlay->IsActive()) {
      pOverlay->AddToTileset();
    }
  }

  for (UCesiumTileExcluder* pTileExcluder : tileExcluders) {
    if (pTileExcluder->IsActive()) {
      pTileExcluder->AddToTileset();
    }
  }

  switch (this->TilesetSource) {
  case ETilesetSource::FromEllipsoid:
    UE_LOG(LogCesium, Log, TEXT("Loading tileset from ellipsoid done"));
    break;
  case ETilesetSource::FromUrl:
    UE_LOG(
        LogCesium,
        Log,
        TEXT("Loading tileset from URL %s done"),
        *this->Url);
    break;
  case ETilesetSource::FromCesiumIon:
    UE_LOG(
        LogCesium,
        Log,
        TEXT("Loading tileset for asset ID %d done"),
        this->IonAssetID);
    break;
  }

  switch (ApplyDpiScaling) {
  case (EApplyDpiScaling::UseProjectDefault):
    _scaleUsingDPI =
        GetDefault<UCesiumRuntimeSettings>()->ScaleLevelOfDetailByDPI;
    break;
  case (EApplyDpiScaling::Yes):
    _scaleUsingDPI = true;
    break;
  case (EApplyDpiScaling::No):
    _scaleUsingDPI = false;
    break;
  default:
    _scaleUsingDPI = true;
  }
}

void ACesium3DTileset::DestroyTileset() {
  if (this->_cesiumViewExtension) {
    this->_cesiumViewExtension = nullptr;
  }

  switch (this->TilesetSource) {
  case ETilesetSource::FromEllipsoid:
    UE_LOG(LogCesium, Verbose, TEXT("Destroying tileset from ellipsoid"));
    break;
  case ETilesetSource::FromUrl:
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Destroying tileset from URL %s"),
        *this->Url);
    break;
  case ETilesetSource::FromCesiumIon:
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Destroying tileset for asset ID %d"),
        this->IonAssetID);
    break;
  }

  // The way CesiumRasterOverlay::add is currently implemented, destroying the
  // tileset without removing overlays will make it impossible to add it again
  // once a new tileset is created (e.g. when switching between terrain
  // assets)
  TArray<UCesiumRasterOverlay*> rasterOverlays;
  this->GetComponents<UCesiumRasterOverlay>(rasterOverlays);
  for (UCesiumRasterOverlay* pOverlay : rasterOverlays) {
    if (pOverlay->IsActive()) {
      pOverlay->RemoveFromTileset();
    }
  }

  TArray<UCesiumTileExcluder*> tileExcluders;
  this->GetComponents<UCesiumTileExcluder>(tileExcluders);
  for (UCesiumTileExcluder* pTileExcluder : tileExcluders) {
    if (pTileExcluder->IsActive()) {
      pTileExcluder->RemoveFromTileset();
    }
  }

  // Tiles are about to be deleted, so we should not keep raw pointers on them.
  // It did crash in Tick() when we trigger refresh events at a high frequency,
  // typically if the user clicks a button "frantically"...)
  this->_tilesToHideNextFrame.clear();

  if (!this->_pTileset) {
    return;
  }

  // Don't allow this Cesium3DTileset to be fully destroyed until
  // any cesium-native Tilesets it created have wrapped up any async
  // operations in progress and have been fully destroyed.
  // See IsReadyForFinishDestroy.
  ++this->_tilesetsBeingDestroyed;
  this->_pTileset->getAsyncDestructionCompleteEvent().thenInMainThread(
      [this]() { --this->_tilesetsBeingDestroyed; });
  this->_pTileset.Reset();

  switch (this->TilesetSource) {
  case ETilesetSource::FromEllipsoid:
    UE_LOG(LogCesium, Verbose, TEXT("Destroying tileset from ellipsoid done"));
    break;
  case ETilesetSource::FromUrl:
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Destroying tileset from URL %s done"),
        *this->Url);
    break;
  case ETilesetSource::FromCesiumIon:
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Destroying tileset for asset ID %d done"),
        this->IonAssetID);
    break;
  }
}

std::vector<FCesiumCamera> ACesium3DTileset::GetCameras() const {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CollectCameras)
  std::vector<FCesiumCamera> cameras = this->GetPlayerCameras();

  std::vector<FCesiumCamera> sceneCaptures = this->GetSceneCaptures();
  cameras.insert(
      cameras.end(),
      std::make_move_iterator(sceneCaptures.begin()),
      std::make_move_iterator(sceneCaptures.end()));

#if WITH_EDITOR
  std::vector<FCesiumCamera> editorCameras = this->GetEditorCameras();
  cameras.insert(
      cameras.end(),
      std::make_move_iterator(editorCameras.begin()),
      std::make_move_iterator(editorCameras.end()));
#endif

  ACesiumCameraManager* pCameraManager = this->ResolvedCameraManager;
  if (pCameraManager) {
    const TMap<int32, FCesiumCamera>& extraCameras =
        pCameraManager->GetCameras();
    cameras.reserve(cameras.size() + extraCameras.Num());
    for (auto cameraIt : extraCameras) {
      cameras.push_back(cameraIt.Value);
    }
  }

  return cameras;
}

std::vector<FCesiumCamera> ACesium3DTileset::GetPlayerCameras() const {
  UWorld* pWorld = this->GetWorld();
  if (!pWorld) {
    return {};
  }

  double worldToMeters = 100.0;
  AWorldSettings* pWorldSettings = pWorld->GetWorldSettings();
  if (pWorldSettings) {
    worldToMeters = pWorldSettings->WorldToMeters;
  }

  TSharedPtr<IStereoRendering, ESPMode::ThreadSafe> pStereoRendering = nullptr;
  if (GEngine) {
    pStereoRendering = GEngine->StereoRenderingDevice;
  }

  bool useStereoRendering = false;
  if (pStereoRendering && pStereoRendering->IsStereoEnabled()) {
    useStereoRendering = true;
  }

  std::vector<FCesiumCamera> cameras;
  cameras.reserve(pWorld->GetNumPlayerControllers());

  for (auto playerControllerIt = pWorld->GetPlayerControllerIterator();
       playerControllerIt;
       playerControllerIt++) {
    const TWeakObjectPtr<APlayerController> pPlayerController =
        *playerControllerIt;
    if (pPlayerController == nullptr) {
      continue;
    }

    const APlayerCameraManager* pPlayerCameraManager =
        pPlayerController->PlayerCameraManager;

    if (!pPlayerCameraManager) {
      continue;
    }

    double fov = pPlayerCameraManager->GetFOVAngle();

    FVector location;
    FRotator rotation;
    pPlayerController->GetPlayerViewPoint(location, rotation);

    int32 sizeX, sizeY;
    pPlayerController->GetViewportSize(sizeX, sizeY);
    if (sizeX < 1 || sizeY < 1) {
      continue;
    }

    float dpiScalingFactor = 1.0f;
    if (this->_scaleUsingDPI) {
      ULocalPlayer* LocPlayer = Cast<ULocalPlayer>(pPlayerController->Player);
      if (LocPlayer && LocPlayer->ViewportClient) {
        dpiScalingFactor = LocPlayer->ViewportClient->GetDPIScale();
      }
    }

    if (useStereoRendering) {
      const auto leftEye = EStereoscopicEye::eSSE_LEFT_EYE;
      const auto rightEye = EStereoscopicEye::eSSE_RIGHT_EYE;

      uint32 stereoLeftSizeX = static_cast<uint32>(sizeX);
      uint32 stereoLeftSizeY = static_cast<uint32>(sizeY);
      uint32 stereoRightSizeX = static_cast<uint32>(sizeX);
      uint32 stereoRightSizeY = static_cast<uint32>(sizeY);
      if (useStereoRendering) {
        int32 _x;
        int32 _y;

        pStereoRendering
            ->AdjustViewRect(leftEye, _x, _y, stereoLeftSizeX, stereoLeftSizeY);

        pStereoRendering->AdjustViewRect(
            rightEye,
            _x,
            _y,
            stereoRightSizeX,
            stereoRightSizeY);
      }

      FVector2D stereoLeftSize(stereoLeftSizeX, stereoLeftSizeY);
      FVector2D stereoRightSize(stereoRightSizeX, stereoRightSizeY);

      if (stereoLeftSize.X >= 1.0 && stereoLeftSize.Y >= 1.0) {
        FVector leftEyeLocation = location;
        FRotator leftEyeRotation = rotation;
        pStereoRendering->CalculateStereoViewOffset(
            leftEye,
            leftEyeRotation,
            worldToMeters,
            leftEyeLocation);

        FMatrix projection =
            pStereoRendering->GetStereoProjectionMatrix(leftEye);

        // TODO: consider assymetric frustums using 4 fovs
        double one_over_tan_half_hfov = projection.M[0][0];

        double hfov =
            glm::degrees(2.0 * glm::atan(1.0 / one_over_tan_half_hfov));

        cameras.emplace_back(
            stereoLeftSize,
            leftEyeLocation,
            leftEyeRotation,
            hfov);
      }

      if (stereoRightSize.X >= 1.0 && stereoRightSize.Y >= 1.0) {
        FVector rightEyeLocation = location;
        FRotator rightEyeRotation = rotation;
        pStereoRendering->CalculateStereoViewOffset(
            rightEye,
            rightEyeRotation,
            worldToMeters,
            rightEyeLocation);

        FMatrix projection =
            pStereoRendering->GetStereoProjectionMatrix(rightEye);

        double one_over_tan_half_hfov = projection.M[0][0];

        double hfov =
            glm::degrees(2.0f * glm::atan(1.0f / one_over_tan_half_hfov));

        cameras.emplace_back(
            stereoRightSize,
            rightEyeLocation,
            rightEyeRotation,
            hfov);
      }
    } else {
      cameras.emplace_back(
          FVector2D(sizeX / dpiScalingFactor, sizeY / dpiScalingFactor),
          location,
          rotation,
          fov);
    }
  }

  return cameras;
}

std::vector<FCesiumCamera> ACesium3DTileset::GetSceneCaptures() const {
  // TODO: really USceneCaptureComponent2D can be attached to any actor, is it
  // worth searching every actor? Might it be better to provide an interface
  // where users can volunteer cameras to be used with the tile selection as
  // needed?
  TArray<AActor*> sceneCaptures;
  static TSubclassOf<ASceneCapture2D> SceneCapture2D =
      ASceneCapture2D::StaticClass();
  UGameplayStatics::GetAllActorsOfClass(this, SceneCapture2D, sceneCaptures);

  std::vector<FCesiumCamera> cameras;
  cameras.reserve(sceneCaptures.Num());

  for (AActor* pActor : sceneCaptures) {
    ASceneCapture2D* pSceneCapture = static_cast<ASceneCapture2D*>(pActor);
    if (!pSceneCapture) {
      continue;
    }

    USceneCaptureComponent2D* pSceneCaptureComponent =
        pSceneCapture->GetCaptureComponent2D();
    if (!pSceneCaptureComponent) {
      continue;
    }

    if (pSceneCaptureComponent->ProjectionType !=
        ECameraProjectionMode::Type::Perspective) {
      continue;
    }

    UTextureRenderTarget2D* pRenderTarget =
        pSceneCaptureComponent->TextureTarget;
    if (!pRenderTarget) {
      continue;
    }

    FVector2D renderTargetSize(pRenderTarget->SizeX, pRenderTarget->SizeY);
    if (renderTargetSize.X < 1.0 || renderTargetSize.Y < 1.0) {
      continue;
    }

    FVector captureLocation = pSceneCaptureComponent->GetComponentLocation();
    FRotator captureRotation = pSceneCaptureComponent->GetComponentRotation();
    double captureFov = pSceneCaptureComponent->FOVAngle;

    cameras.emplace_back(
        renderTargetSize,
        captureLocation,
        captureRotation,
        captureFov);
  }

  return cameras;
}

/*static*/ Cesium3DTilesSelection::ViewState
ACesium3DTileset::CreateViewStateFromViewParameters(
    const FCesiumCamera& camera,
    const glm::dmat4& unrealWorldToTileset,
    UCesiumEllipsoid* ellipsoid) {
  double horizontalFieldOfView =
      FMath::DegreesToRadians(camera.FieldOfViewDegrees);

  double actualAspectRatio;
  glm::dvec2 size(camera.ViewportSize.X, camera.ViewportSize.Y);

  if (camera.OverrideAspectRatio != 0.0f) {
    // Use aspect ratio and recompute effective viewport size after black bars
    // are added.
    actualAspectRatio = camera.OverrideAspectRatio;
    double computedX = actualAspectRatio * camera.ViewportSize.Y;
    double computedY = camera.ViewportSize.Y / actualAspectRatio;

    double barWidth = camera.ViewportSize.X - computedX;
    double barHeight = camera.ViewportSize.Y - computedY;

    if (barWidth > 0.0 && barWidth > barHeight) {
      // Black bars on the sides
      size.x = computedX;
    } else if (barHeight > 0.0 && barHeight > barWidth) {
      // Black bars on the top and bottom
      size.y = computedY;
    }
  } else {
    actualAspectRatio = camera.ViewportSize.X / camera.ViewportSize.Y;
  }

  double verticalFieldOfView =
      atan(tan(horizontalFieldOfView * 0.5) / actualAspectRatio) * 2.0;

  FVector direction = camera.Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
  FVector up = camera.Rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));

  glm::dvec3 tilesetCameraLocation = glm::dvec3(
      unrealWorldToTileset *
      glm::dvec4(camera.Location.X, camera.Location.Y, camera.Location.Z, 1.0));
  glm::dvec3 tilesetCameraFront = glm::normalize(glm::dvec3(
      unrealWorldToTileset *
      glm::dvec4(direction.X, direction.Y, direction.Z, 0.0)));
  glm::dvec3 tilesetCameraUp = glm::normalize(
      glm::dvec3(unrealWorldToTileset * glm::dvec4(up.X, up.Y, up.Z, 0.0)));

  return Cesium3DTilesSelection::ViewState(
      tilesetCameraLocation,
      tilesetCameraFront,
      tilesetCameraUp,
      size,
      horizontalFieldOfView,
      verticalFieldOfView,
      ellipsoid->GetNativeEllipsoid());
}

#if WITH_EDITOR
std::vector<FCesiumCamera> ACesium3DTileset::GetEditorCameras() const {
  if (!GEditor) {
    return {};
  }

  UWorld* pWorld = this->GetWorld();
  if (!IsValid(pWorld)) {
    return {};
  }

  // Do not include editor cameras when running in a game world (which includes
  // Play-in-Editor)
  if (pWorld->IsGameWorld()) {
    return {};
  }

  const TArray<FEditorViewportClient*>& viewportClients =
      GEditor->GetAllViewportClients();

  std::vector<FCesiumCamera> cameras;
  cameras.reserve(viewportClients.Num());

  for (FEditorViewportClient* pEditorViewportClient : viewportClients) {
    if (!pEditorViewportClient) {
      continue;
    }

    if (!pEditorViewportClient->IsVisible() ||
        !pEditorViewportClient->IsRealtime() ||
        !pEditorViewportClient->IsPerspective()) {
      continue;
    }

    FRotator rotation;
    if (pEditorViewportClient->bUsingOrbitCamera) {
      rotation = (pEditorViewportClient->GetLookAtLocation() -
                  pEditorViewportClient->GetViewLocation())
                     .Rotation();
    } else {
      rotation = pEditorViewportClient->GetViewRotation();
    }

    const FVector& location = pEditorViewportClient->GetViewLocation();
    double fov = pEditorViewportClient->ViewFOV;
    FIntPoint offset;
    FIntPoint size;
    pEditorViewportClient->GetViewportDimensions(offset, size);

    if (size.X < 1 || size.Y < 1) {
      continue;
    }

    if (this->_scaleUsingDPI) {
      float dpiScalingFactor = pEditorViewportClient->GetDPIScale();
      size.X = static_cast<float>(size.X) / dpiScalingFactor;
      size.Y = static_cast<float>(size.Y) / dpiScalingFactor;
    }

    if (pEditorViewportClient->IsAspectRatioConstrained()) {
      cameras.emplace_back(
          size,
          location,
          rotation,
          fov,
          pEditorViewportClient->AspectRatio);
    } else {
      cameras.emplace_back(size, location, rotation, fov);
    }
  }

  return cameras;
}
#endif

bool ACesium3DTileset::ShouldTickIfViewportsOnly() const {
  return this->UpdateInEditor;
}

namespace {
template <typename Func>
void forEachRenderableTile(const auto& tiles, Func&& f) {
  for (const CesiumUtility::IntrusivePointer<Cesium3DTilesSelection::Tile>&
           pTile : tiles) {
    if (!pTile ||
        pTile->getState() != Cesium3DTilesSelection::TileLoadState::Done) {
      continue;
    }

    const Cesium3DTilesSelection::TileContent& content = pTile->getContent();
    const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
        content.getRenderContent();
    if (!pRenderContent) {
      continue;
    }

    UCesiumGltfComponent* Gltf = static_cast<UCesiumGltfComponent*>(
        pRenderContent->getRenderResources());
    if (!Gltf) {
      // When a tile does not have render resources (i.e. a glTF), then
      // the resources either have not yet been loaded or prepared,
      // or the tile is from an external tileset and does not directly
      // own renderable content. In both cases, the tile is ignored here.
      continue;
    }

    f(pTile, Gltf);
  }
}

void removeVisibleTilesFromList(
    std::vector<CesiumUtility::IntrusivePointer<Cesium3DTilesSelection::Tile>>&
        list,
    const std::vector<
        CesiumUtility::IntrusivePointer<Cesium3DTilesSelection::Tile>>&
        visibleTiles) {
  if (list.empty()) {
    return;
  }

  for (const CesiumUtility::IntrusivePointer<Cesium3DTilesSelection::Tile>&
           pTile : visibleTiles) {
    auto it = std::find(list.begin(), list.end(), pTile);
    if (it != list.end()) {
      list.erase(it);
    }
  }
}

/**
 * @brief Hides the visual representations of the given tiles.
 *
 * The visual representations (i.e. the `getRendererResources` of the
 * tiles) are assumed to be `UCesiumGltfComponent` instances that
 * are made invisible by this call.
 *
 * @param tiles The tiles to hide
 */
void hideTiles(
    const std::vector<
        CesiumUtility::IntrusivePointer<Cesium3DTilesSelection::Tile>>& tiles) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::HideTiles)
  forEachRenderableTile(
      tiles,
      [](const CesiumUtility::IntrusivePointer<
             Cesium3DTilesSelection::Tile>& /*pTile*/,
         UCesiumGltfComponent* pGltf) {
        if (pGltf->IsVisible()) {
          TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetVisibilityFalse)
          pGltf->SetVisibility(false, true);
        } else {
          // TODO: why is this happening?
          UE_LOG(
              LogCesium,
              Verbose,
              TEXT("Tile to no longer render does not have a visible Gltf"));
        }
      });
}

/**
 * @brief Removes collision for tiles that have been removed from the render
 * list. This includes tiles that are fading out.
 */
void removeCollisionForTiles(
    const std::unordered_set<
        CesiumUtility::IntrusivePointer<Cesium3DTilesSelection::Tile>>& tiles) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::RemoveCollisionForTiles)
  forEachRenderableTile(
      tiles,
      [](const CesiumUtility::IntrusivePointer<
             Cesium3DTilesSelection::Tile>& /*pTile*/,
         UCesiumGltfComponent* pGltf) {
        TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetCollisionDisabled)
        pGltf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
      });
}

/**
 * @brief Applies the actor collision settings for a newly created glTF
 * component
 *
 * TODO Add details here what that means
 * @param BodyInstance ...
 * @param Gltf ...
 */
void applyActorCollisionSettings(
    const FBodyInstance& BodyInstance,
    UCesiumGltfComponent* Gltf) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::ApplyActorCollisionSettings)

  const TArray<USceneComponent*>& ChildrenComponents =
      Gltf->GetAttachChildren();

  for (USceneComponent* ChildComponent : ChildrenComponents) {
    UCesiumGltfPrimitiveComponent* PrimitiveComponent =
        Cast<UCesiumGltfPrimitiveComponent>(ChildComponent);
    if (PrimitiveComponent != nullptr) {
      if (PrimitiveComponent->GetCollisionObjectType() !=
          BodyInstance.GetObjectType()) {
        PrimitiveComponent->SetCollisionObjectType(
            BodyInstance.GetObjectType());
      }
      const UEnum* ChannelEnum = StaticEnum<ECollisionChannel>();
      if (ChannelEnum) {
        FCollisionResponseContainer responseContainer =
            BodyInstance.GetResponseToChannels();
        PrimitiveComponent->SetCollisionResponseToChannels(responseContainer);
      }
    }
  }
}
} // namespace

void ACesium3DTileset::updateTilesetOptionsFromProperties() {
  Cesium3DTilesSelection::TilesetOptions& options =
      this->_pTileset->getOptions();
  options.maximumScreenSpaceError =
      static_cast<double>(this->MaximumScreenSpaceError);
  options.maximumCachedBytes = this->MaximumCachedBytes;
  options.preloadAncestors = this->PreloadAncestors;
  options.preloadSiblings = this->PreloadSiblings;
  options.forbidHoles = this->ForbidHoles;
  options.maximumSimultaneousTileLoads = this->MaximumSimultaneousTileLoads;
  options.loadingDescendantLimit = this->LoadingDescendantLimit;
  options.enableFrustumCulling = this->EnableFrustumCulling;
  options.enableOcclusionCulling =
      GetDefault<UCesiumRuntimeSettings>()
          ->EnableExperimentalOcclusionCullingFeature &&
      this->EnableOcclusionCulling;
  options.showCreditsOnScreen = this->ShowCreditsOnScreen;

  options.delayRefinementForOcclusion = this->DelayRefinementForOcclusion;
  options.enableFogCulling = this->EnableFogCulling;
  options.enforceCulledScreenSpaceError = this->EnforceCulledScreenSpaceError;
  options.culledScreenSpaceError =
      static_cast<double>(this->CulledScreenSpaceError);
  options.enableLodTransitionPeriod = this->UseLodTransitions;
  options.lodTransitionLength = this->LodTransitionLength;
  // options.kickDescendantsWhileFadingIn = false;
}

void ACesium3DTileset::updateLastViewUpdateResultState(
    const Cesium3DTilesSelection::ViewUpdateResult& result) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::updateLastViewUpdateResultState)

  if (this->DrawTileInfo) {
    const UWorld* World = GetWorld();
    check(World);

    const TSoftObjectPtr<ACesiumGeoreference> Georeference =
        ResolveGeoreference();
    check(Georeference);

    for (const CesiumUtility::IntrusivePointer<Cesium3DTilesSelection::Tile>&
             pTile : result.tilesToRenderThisFrame) {
      CesiumGeometry::OrientedBoundingBox obb =
          Cesium3DTilesSelection::getOrientedBoundingBoxFromBoundingVolume(
              pTile->getBoundingVolume(),
              Georeference->GetEllipsoid()->GetNativeEllipsoid());

      FVector unrealCenter =
          Georeference->TransformEarthCenteredEarthFixedPositionToUnreal(
              VecMath::createVector(obb.getCenter()));

      FString text = FString::Printf(
          TEXT("ID %s (%p)"),
          UTF8_TO_TCHAR(
              Cesium3DTilesSelection::TileIdUtilities::createTileIdString(
                  pTile->getTileID())
                  .c_str()),
          pTile.get());

      DrawDebugString(World, unrealCenter, text, nullptr, FColor::Red, 0, true);
    }
  }

#ifdef CESIUM_DEBUG_TILE_STATES
  if (this->_pStateDebug && GetWorld()->IsPlayInEditor()) {
    this->_pStateDebug->recordAllTileStates(
        result.frameNumber,
        *this->_pTileset);
  }
#endif

  if (!this->LogSelectionStats && !this->LogSharedAssetStats) {
    return;
  }

  if (result.tilesToRenderThisFrame.size() != this->_lastTilesRendered ||
      result.workerThreadTileLoadQueueLength !=
          this->_lastWorkerThreadTileLoadQueueLength ||
      result.mainThreadTileLoadQueueLength !=
          this->_lastMainThreadTileLoadQueueLength ||
      result.tilesVisited != this->_lastTilesVisited ||
      result.culledTilesVisited != this->_lastCulledTilesVisited ||
      result.tilesCulled != this->_lastTilesCulled ||
      result.tilesOccluded != this->_lastTilesOccluded ||
      result.tilesWaitingForOcclusionResults !=
          this->_lastTilesWaitingForOcclusionResults ||
      result.maxDepthVisited != this->_lastMaxDepthVisited) {
    this->_lastTilesRendered = result.tilesToRenderThisFrame.size();
    this->_lastWorkerThreadTileLoadQueueLength =
        result.workerThreadTileLoadQueueLength;
    this->_lastMainThreadTileLoadQueueLength =
        result.mainThreadTileLoadQueueLength;

    this->_lastTilesVisited = result.tilesVisited;
    this->_lastCulledTilesVisited = result.culledTilesVisited;
    this->_lastTilesCulled = result.tilesCulled;
    this->_lastTilesOccluded = result.tilesOccluded;
    this->_lastTilesWaitingForOcclusionResults =
        result.tilesWaitingForOcclusionResults;
    this->_lastMaxDepthVisited = result.maxDepthVisited;

    if (this->LogSelectionStats) {
      UE_LOG(
          LogCesium,
          Display,
          TEXT(
              "%s: %d ms, Unreal Frame #%d, Tileset Frame: #%d, Visited %d, Culled Visited %d, Rendered %d, Culled %d, Occluded %d, Waiting For Occlusion Results %d, Max Depth Visited: %d, Loading-Worker %d, Loading-Main %d, Loaded tiles %g%%"),
          *this->GetName(),
          (std::chrono::high_resolution_clock::now() - this->_startTime)
                  .count() /
              1000000,
          GFrameCounter,
          result.frameNumber,
          result.tilesVisited,
          result.culledTilesVisited,
          result.tilesToRenderThisFrame.size(),
          result.tilesCulled,
          result.tilesOccluded,
          result.tilesWaitingForOcclusionResults,
          result.maxDepthVisited,
          result.workerThreadTileLoadQueueLength,
          result.mainThreadTileLoadQueueLength,
          this->LoadProgress);
    }

    if (this->LogSharedAssetStats && this->_pTileset) {
      const Cesium3DTilesSelection::TilesetSharedAssetSystem::ImageDepot&
          imageDepot = *this->_pTileset->getSharedAssetSystem().pImage;
      UE_LOG(
          LogCesium,
          Display,
          TEXT(
              "Images shared asset depot: %d distinct assets, %d inactive assets pending deletion (%d bytes)"),
          imageDepot.getAssetCount(),
          imageDepot.getInactiveAssetCount(),
          imageDepot.getInactiveAssetTotalSizeBytes());
    }
  }
}

void ACesium3DTileset::showTilesToRender(
    const std::vector<
        CesiumUtility::IntrusivePointer<Cesium3DTilesSelection::Tile>>& tiles) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::ShowTilesToRender)
  forEachRenderableTile(
      tiles,
      [&RootComponent = this->RootComponent,
       &BodyInstance = this->BodyInstance](
          const CesiumUtility::IntrusivePointer<Cesium3DTilesSelection::Tile>&
              pTile,
          UCesiumGltfComponent* pGltf) {
        applyActorCollisionSettings(BodyInstance, pGltf);

        if (pGltf->GetAttachParent() == nullptr) {
          // The AttachToComponent method is ridiculously complex,
          // so print a warning if attaching fails for some reason
          bool attached = pGltf->AttachToComponent(
              RootComponent,
              FAttachmentTransformRules::KeepRelativeTransform);
          if (!attached) {
            FString tileIdString(
                Cesium3DTilesSelection::TileIdUtilities::createTileIdString(
                    pTile->getTileID())
                    .c_str());
            UE_LOG(
                LogCesium,
                Warning,
                TEXT("Tile %s could not be attached to root"),
                *tileIdString);
          }
        }

        if (!pGltf->IsVisible()) {
          TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetVisibilityTrue)
          pGltf->SetVisibility(true, true);
        }

        {
          TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::SetCollisionEnabled)
          pGltf->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        }
      });
}

static void updateTileFades(const auto& tiles, bool fadingIn) {
  forEachRenderableTile(
      tiles,
      [fadingIn](
          const CesiumUtility::IntrusivePointer<Cesium3DTilesSelection::Tile>&
              pTile,
          UCesiumGltfComponent* pGltf) {
        float percentage = pTile->getContent()
                               .getRenderContent()
                               ->getLodTransitionFadePercentage();
        pGltf->UpdateFade(percentage, fadingIn);
      });
}

// Called every frame
void ACesium3DTileset::Tick(float DeltaTime) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::TilesetTick)

  Super::Tick(DeltaTime);

  this->ResolveGeoreference();
  this->ResolveCameraManager();
  this->ResolveCreditSystem();

  UCesium3DTilesetRoot* pRoot = Cast<UCesium3DTilesetRoot>(this->RootComponent);
  if (!pRoot) {
    return;
  }

  if (this->SuspendUpdate) {
    return;
  }

  if (!this->_pTileset) {
    LoadTileset();

    // In the unlikely event that we _still_ don't have a tileset, stop here so
    // we don't crash below. This shouldn't happen.
    if (!this->_pTileset) {
      assert(false);
      return;
    }
  }

  if (this->BoundingVolumePoolComponent && this->_cesiumViewExtension) {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::UpdateOcclusion)
    const TArray<USceneComponent*>& children =
        this->BoundingVolumePoolComponent->GetAttachChildren();
    for (USceneComponent* pChild : children) {
      UCesiumBoundingVolumeComponent* pBoundingVolume =
          Cast<UCesiumBoundingVolumeComponent>(pChild);

      if (!pBoundingVolume) {
        continue;
      }

      pBoundingVolume->UpdateOcclusion(*this->_cesiumViewExtension.Get());
    }
  }

  updateTilesetOptionsFromProperties();

  std::vector<FCesiumCamera> cameras = this->GetCameras();

  glm::dmat4 ueTilesetToUeWorld =
      VecMath::createMatrix4D(this->GetActorTransform().ToMatrixWithScale());

  const glm::dmat4& cesiumTilesetToUeTileset =
      this->GetCesiumTilesetToUnrealRelativeWorldTransform();
  glm::dmat4 unrealWorldToCesiumTileset =
      glm::affineInverse(ueTilesetToUeWorld * cesiumTilesetToUeTileset);

  if (glm::isnan(unrealWorldToCesiumTileset[3].x) ||
      glm::isnan(unrealWorldToCesiumTileset[3].y) ||
      glm::isnan(unrealWorldToCesiumTileset[3].z)) {
    // Probably caused by a zero scale.
    return;
  }

  UCesiumEllipsoid* ellipsoid = this->ResolveGeoreference()->GetEllipsoid();

  std::vector<Cesium3DTilesSelection::ViewState> frustums;
  for (const FCesiumCamera& camera : cameras) {
    frustums.push_back(CreateViewStateFromViewParameters(
        camera,
        unrealWorldToCesiumTileset,
        ellipsoid));
  }

  const Cesium3DTilesSelection::ViewUpdateResult* pResult;
  if (this->_captureMovieMode) {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::updateViewOffline)
    pResult = &this->_pTileset->updateViewGroupOffline(
        this->_pTileset->getDefaultViewGroup(),
        frustums);
  } else {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::updateView)
    pResult = &this->_pTileset->updateViewGroup(
        this->_pTileset->getDefaultViewGroup(),
        frustums,
        DeltaTime);
  }

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::loadTiles)
    this->_pTileset->loadTiles();
  }

  updateLastViewUpdateResultState(*pResult);

  removeCollisionForTiles(pResult->tilesFadingOut);

  removeVisibleTilesFromList(
      this->_tilesToHideNextFrame,
      pResult->tilesToRenderThisFrame);
  hideTiles(this->_tilesToHideNextFrame);

  _tilesToHideNextFrame.clear();
  for (const CesiumUtility::IntrusivePointer<Cesium3DTilesSelection::Tile>&
           pTile : pResult->tilesFadingOut) {
    Cesium3DTilesSelection::TileRenderContent* pRenderContent =
        pTile->getContent().getRenderContent();
    if (!this->UseLodTransitions ||
        (pRenderContent &&
         pRenderContent->getLodTransitionFadePercentage() >= 1.0f)) {
      _tilesToHideNextFrame.push_back(pTile);
    }
  }

  showTilesToRender(pResult->tilesToRenderThisFrame);

  if (this->UseLodTransitions) {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::UpdateTileFades)
    updateTileFades(pResult->tilesToRenderThisFrame, true);
    updateTileFades(pResult->tilesFadingOut, false);
  }

  this->UpdateLoadStatus();
}

void ACesium3DTileset::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  this->DestroyTileset();
  AActor::EndPlay(EndPlayReason);
}

void ACesium3DTileset::PostLoad() {
  BodyInstance.FixupData(this); // We need to call this one after Loading the
                                // actor to have correct BodyInstance values.

  Super::PostLoad();

  if (CesiumActors::shouldValidateFlags(this))
    CesiumActors::validateActorFlags(this);

#if WITH_EDITOR
  const int32 CesiumVersion =
      this->GetLinkerCustomVersion(FCesiumCustomVersion::GUID);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  if (CesiumVersion < FCesiumCustomVersion::CesiumIonServer) {
    this->CesiumIonServer = UCesiumIonServer::GetBackwardCompatibleServer(
        this->IonAssetEndpointUrl_DEPRECATED);
  }
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif
}

void ACesium3DTileset::Serialize(FArchive& Ar) {
  Super::Serialize(Ar);

  Ar.UsingCustomVersion(FCesiumCustomVersion::GUID);

  const int32 CesiumVersion = Ar.CustomVer(FCesiumCustomVersion::GUID);

  if (CesiumVersion < FCesiumCustomVersion::TilesetExplicitSource) {
    // In previous versions, the tileset source was inferred from the presence
    // of a non-empty URL property, rather than being explicitly specified.
    if (this->Url.Len() > 0) {
      this->TilesetSource = ETilesetSource::FromUrl;
    } else {
      this->TilesetSource = ETilesetSource::FromCesiumIon;
    }
  }

  if (CesiumVersion < FCesiumCustomVersion::TilesetMobilityRemoved) {
    this->RootComponent->SetMobility(this->Mobility_DEPRECATED);
  }
}

#if WITH_EDITOR
void ACesium3DTileset::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {
  Super::PostEditChangeProperty(PropertyChangedEvent);

  if (!PropertyChangedEvent.Property) {
    return;
  }

  FName PropName = PropertyChangedEvent.Property->GetFName();
  FString PropNameAsString = PropertyChangedEvent.Property->GetName();

  if (PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, TilesetSource) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, Url) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, IonAssetID) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, IonAccessToken) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, CreatePhysicsMeshes) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, CreateNavCollision) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, AlwaysIncludeTangents) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, GenerateSmoothNormals) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, EnableWaterMask) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, IgnoreKhrMaterialsUnlit) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, Material) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, TranslucentMaterial) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, WaterMaterial) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, ApplyDpiScaling) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, EnableOcclusionCulling) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, UseLodTransitions) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, ShowCreditsOnScreen) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, Root) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, CesiumIonServer) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, RequestHeaders) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, RuntimeVirtualTextures) ||
      PropName == GET_MEMBER_NAME_CHECKED(
                      ACesium3DTileset,
                      VirtualTextureRenderPassType) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, TranslucencySortPriority) ||
      // For properties nested in structs, GET_MEMBER_NAME_CHECKED will prefix
      // with the struct name, so just do a manual string comparison.
      PropNameAsString == TEXT("RenderCustomDepth") ||
      PropNameAsString == TEXT("CustomDepthStencilValue") ||
      PropNameAsString == TEXT("CustomDepthStencilWriteMask")) {
    this->DestroyTileset();
  } else if (
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, Georeference)) {
    this->InvalidateResolvedGeoreference();
  } else if (
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, CreditSystem)) {
    this->InvalidateResolvedCreditSystem();
  } else if (
      PropName ==
      GET_MEMBER_NAME_CHECKED(ACesium3DTileset, MaximumScreenSpaceError)) {
    TArray<UCesiumRasterOverlay*> rasterOverlays;
    this->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

    for (UCesiumRasterOverlay* pOverlay : rasterOverlays) {
      pOverlay->Refresh();
    }
    TArray<UCesiumTileExcluder*> tileExcluders;
    this->GetComponents<UCesiumTileExcluder>(tileExcluders);

    for (UCesiumTileExcluder* pTileExcluder : tileExcluders) {
      pTileExcluder->Refresh();
    }

    // Maximum Screen Space Error can affect how attenuated points are rendered,
    // so propagate the new value to the render proxies for this tileset.
    FCesiumGltfPointsSceneProxyUpdater::UpdateSettingsInProxies(this);
  }
}

void ACesium3DTileset::PostEditChangeChainProperty(
    FPropertyChangedChainEvent& PropertyChangedChainEvent) {
  Super::PostEditChangeChainProperty(PropertyChangedChainEvent);

  if (!PropertyChangedChainEvent.Property ||
      PropertyChangedChainEvent.PropertyChain.IsEmpty()) {
    return;
  }

  FName PropName =
      PropertyChangedChainEvent.PropertyChain.GetHead()->GetValue()->GetFName();
  if (PropName ==
      GET_MEMBER_NAME_CHECKED(ACesium3DTileset, PointCloudShading)) {
    FCesiumGltfPointsSceneProxyUpdater::UpdateSettingsInProxies(this);
  }
}

void ACesium3DTileset::PostEditUndo() {
  Super::PostEditUndo();

  // It doesn't appear to be possible to get detailed information about what
  // changed in the undo/redo operation, so we have to assume the worst and
  // recreate the tileset.
  this->DestroyTileset();
}

void ACesium3DTileset::PostEditImport() {
  Super::PostEditImport();

  // Recreate the tileset on Paste.
  this->DestroyTileset();
}

bool ACesium3DTileset::CanEditChange(const FProperty* InProperty) const {
  if (InProperty->GetFName() ==
      GET_MEMBER_NAME_CHECKED(ACesium3DTileset, EnableWaterMask)) {
    // Disable this option on Mac
    return PlatformName != TEXT("Mac");
  }
  return true;
}
#endif

void ACesium3DTileset::BeginDestroy() {
  this->InvalidateResolvedGeoreference();
  this->DestroyTileset();

  AActor::BeginDestroy();
}

bool ACesium3DTileset::IsReadyForFinishDestroy() {
  bool ready = AActor::IsReadyForFinishDestroy();
  ready &= this->_tilesetsBeingDestroyed == 0;

  if (!ready) {
    getAssetAccessor()->tick();
    getAsyncSystem().dispatchMainThreadTasks();
  }

  return ready;
}

void ACesium3DTileset::Destroyed() {
  this->DestroyTileset();

  AActor::Destroyed();
}

#if WITH_EDITOR
void ACesium3DTileset::RuntimeSettingsChanged(
    UObject* pObject,
    struct FPropertyChangedEvent& changed) {
  bool occlusionCullingAvailable =
      GetDefault<UCesiumRuntimeSettings>()
          ->EnableExperimentalOcclusionCullingFeature;
  if (occlusionCullingAvailable != this->CanEnableOcclusionCulling) {
    this->CanEnableOcclusionCulling = occlusionCullingAvailable;
    this->RefreshTileset();
  }
}
#endif

// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "Cesium3DTileset.h"
#include "Async/Async.h"
#include "Camera/CameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Cesium3DTilesSelection/BingMapsRasterOverlay.h"
#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/IPrepareRendererResources.h"
#include "Cesium3DTilesSelection/TilesetLoadFailureDetails.h"
#include "Cesium3DTilesSelection/TilesetOptions.h"
#include "Cesium3DTilesetLoadFailureDetails.h"
#include "Cesium3DTilesetRoot.h"
#include "CesiumAsync/CachingAssetAccessor.h"
#include "CesiumAsync/IAssetResponse.h"
#include "CesiumAsync/SqliteCache.h"
#include "CesiumBoundingVolumeComponent.h"
#include "CesiumCamera.h"
#include "CesiumCameraManager.h"
#include "CesiumCommon.h"
#include "CesiumCustomVersion.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumGltf/ImageCesium.h"
#include "CesiumGltf/Ktx2TranscodeTargets.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumLifetime.h"
#include "CesiumRasterOverlay.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"
#include "CesiumTextureUtility.h"
#include "CesiumTransforms.h"
#include "CesiumViewExtension.h"
#include "Components/SceneCaptureComponent2D.h"
#include "CreateGltfOptions.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "IPhysXCookingModule.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/EnumRange.h"
#include "PhysicsPublicCore.h"
#include "PixelFormat.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"
#include "SceneTypes.h"
#include "StereoRendering.h"
#include "UnrealAssetAccessor.h"
#include "UnrealTaskProcessor.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <spdlog/spdlog.h>

FCesium3DTilesetLoadFailure OnCesium3DTilesetLoadFailure{};

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#endif

// Sets default values
ACesium3DTileset::ACesium3DTileset()
    : Georeference(nullptr),
      ResolvedGeoreference(nullptr),
      CreditSystem(nullptr),

      _pTileset(nullptr),

      _lastTilesRendered(0),
      _lastTilesLoadingLowPriority(0),
      _lastTilesLoadingMediumPriority(0),
      _lastTilesLoadingHighPriority(0),

      _lastTilesVisited(0),
      _lastTilesCulled(0),
      _lastTilesOccluded(0),
      _lastTilesWaitingForOcclusionResults(0),
      _lastMaxDepthVisited(0),

      _captureMovieMode{false},
      _beforeMoviePreloadAncestors{PreloadAncestors},
      _beforeMoviePreloadSiblings{PreloadSiblings},
      _beforeMovieLoadingDescendantLimit{LoadingDescendantLimit},
      _beforeMovieUseLodTransitions{true} {

  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickGroup = ETickingGroup::TG_PostUpdateWork;

  this->SetActorEnableCollision(true);

  this->RootComponent =
      CreateDefaultSubobject<UCesium3DTilesetRoot>(TEXT("Tileset"));

  PlatformName = UGameplayStatics::GetPlatformName();
}

ACesium3DTileset::~ACesium3DTileset() { this->DestroyTileset(); }

ACesiumGeoreference* ACesium3DTileset::GetGeoreference() const {
  return this->Georeference;
}

void ACesium3DTileset::SetMobility(EComponentMobility::Type NewMobility) {
  if (NewMobility != this->Mobility) {
    this->Mobility = NewMobility;
    DestroyTileset();
  }
}

void ACesium3DTileset::SetGeoreference(ACesiumGeoreference* NewGeoreference) {
  this->Georeference = NewGeoreference;
  this->InvalidateResolvedGeoreference();
  this->ResolveGeoreference();
}

ACesiumGeoreference* ACesium3DTileset::ResolveGeoreference() {
  if (IsValid(this->ResolvedGeoreference)) {
    return this->ResolvedGeoreference;
  }

  if (IsValid(this->Georeference)) {
    this->ResolvedGeoreference = this->Georeference;
  } else {
    this->ResolvedGeoreference =
        ACesiumGeoreference::GetDefaultGeoreference(this);
  }

  UCesium3DTilesetRoot* pRoot = Cast<UCesium3DTilesetRoot>(this->RootComponent);
  if (pRoot) {
    this->ResolvedGeoreference->OnGeoreferenceUpdated.AddUniqueDynamic(
        pRoot,
        &UCesium3DTilesetRoot::HandleGeoreferenceUpdated);

    // Update existing tile positions, if any.
    pRoot->HandleGeoreferenceUpdated();
  }

  return this->ResolvedGeoreference;
}

void ACesium3DTileset::InvalidateResolvedGeoreference() {
  if (IsValid(this->ResolvedGeoreference)) {
    this->ResolvedGeoreference->OnGeoreferenceUpdated.RemoveAll(this);
  }
  this->ResolvedGeoreference = nullptr;
}

ACesiumCreditSystem* ACesium3DTileset::GetCreditSystem() const {
  return this->CreditSystem;
}

void ACesium3DTileset::SetCreditSystem(ACesiumCreditSystem* NewCreditSystem) {
  this->CreditSystem = NewCreditSystem;
  this->InvalidateResolvedCreditSystem();
  this->ResolveCreditSystem();
}

ACesiumCreditSystem* ACesium3DTileset::ResolveCreditSystem() {
  if (IsValid(this->ResolvedCreditSystem)) {
    return this->ResolvedCreditSystem;
  }

  if (IsValid(this->CreditSystem)) {
    this->ResolvedCreditSystem = this->CreditSystem;
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

void ACesium3DTileset::SetTilesetSource(ETilesetSource InSource) {
  if (InSource != this->TilesetSource) {
    this->DestroyTileset();
    this->TilesetSource = InSource;
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

void ACesium3DTileset::SetIonAssetEndpointUrl(
    const FString& InIonAssetEndpointUrl) {
  if (this->IonAssetEndpointUrl != InIonAssetEndpointUrl) {
    if (this->TilesetSource == ETilesetSource::FromCesiumIon) {
      this->DestroyTileset();
    }
    this->IonAssetEndpointUrl = InIonAssetEndpointUrl;
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

    const GeoTransforms& localGeoTransforms;

    glm::dvec3 operator()(const CesiumGeometry::BoundingSphere& sphere) {
      const glm::dvec3& center = sphere.getCenter();
      glm::dmat4 ENU =
          glm::dmat4(localGeoTransforms.ComputeEastNorthUpToEcef(center));
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
          glm::dmat4(localGeoTransforms.ComputeEastNorthUpToEcef(center));
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
  };

  const Cesium3DTilesSelection::Tile* pRootTile =
      this->_pTileset->getRootTile();
  if (!pRootTile) {
    return;
  }

  const Cesium3DTilesSelection::BoundingVolume& boundingVolume =
      pRootTile->getBoundingVolume();

  // calculate unreal camera position
  const glm::dmat4& transform =
      this->GetCesiumTilesetToUnrealRelativeWorldTransform();
  glm::dvec3 ecefCameraPosition = std::visit(
      CalculateECEFCameraPosition{
          this->ResolveGeoreference()->GetGeoTransforms()},
      boundingVolume);
  glm::dvec3 unrealCameraPosition =
      glm::dvec3(transform * glm::dvec4(ecefCameraPosition, 1.0));

  // calculate unreal camera orientation
  glm::dvec3 ecefCenter =
      Cesium3DTilesSelection::getBoundingVolumeCenter(boundingVolume);
  glm::dvec3 unrealCenter = glm::dvec3(transform * glm::dvec4(ecefCenter, 1.0));
  glm::dvec3 unrealCameraFront =
      glm::normalize(unrealCenter - unrealCameraPosition);
  glm::dvec3 unrealCameraRight =
      glm::normalize(glm::cross(glm::dvec3(0.0, 0.0, 1.0), unrealCameraFront));
  glm::dvec3 unrealCameraUp =
      glm::normalize(glm::cross(unrealCameraFront, unrealCameraRight));
  FRotator cameraRotator =
      FMatrix(
          FVector(
              unrealCameraFront.x,
              unrealCameraFront.y,
              unrealCameraFront.z),
          FVector(
              unrealCameraRight.x,
              unrealCameraRight.y,
              unrealCameraRight.z),
          FVector(unrealCameraUp.x, unrealCameraUp.y, unrealCameraUp.z),
          FVector(0.0f, 0.0f, 0.0f))
          .Rotator();

  // Update all viewports.
  for (FLevelEditorViewportClient* LinkedViewportClient :
       GEditor->GetLevelViewportClients()) {
    // Dont move camera attach to an actor
    if (!LinkedViewportClient->IsAnyActorLocked()) {
      FViewportCameraTransform& ViewTransform =
          LinkedViewportClient->GetViewTransform();
      LinkedViewportClient->SetViewRotation(cameraRotator);
      LinkedViewportClient->SetViewLocation(FVector(
          unrealCameraPosition.x,
          unrealCameraPosition.y,
          unrealCameraPosition.z));
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

// Called when the game starts or when spawned
void ACesium3DTileset::BeginPlay() {
  Super::BeginPlay();

  this->LoadTileset();

  // Search for level sequence.
  for (auto sequenceActorIt = TActorIterator<ALevelSequenceActor>(GetWorld());
       sequenceActorIt;
       ++sequenceActorIt) {
    ALevelSequenceActor* sequenceActor = *sequenceActorIt;

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

class UnrealResourcePreparer
    : public Cesium3DTilesSelection::IPrepareRendererResources {
public:
  UnrealResourcePreparer(ACesium3DTileset* pActor)
      : _pActor(pActor)
#if PHYSICS_INTERFACE_PHYSX
        ,
        _pPhysXCookingModule(
            pActor->GetCreatePhysicsMeshes() ? GetPhysXCookingModule()
                                             : nullptr)
#endif
  {
  }

  virtual void* prepareInLoadThread(
      const CesiumGltf::Model& model,
      const glm::dmat4& transform,
      const std::any& rendererOptions) override {

    CreateGltfOptions::CreateModelOptions options;
    options.pModel = &model;
    options.alwaysIncludeTangents = this->_pActor->GetAlwaysIncludeTangents();
    options.createPhysicsMeshes = this->_pActor->GetCreatePhysicsMeshes();

#if PHYSICS_INTERFACE_PHYSX
    options.pPhysXCookingModule = this->_pPhysXCookingModule;
#endif

    options.pEncodedMetadataDescription =
        &this->_pActor->_encodedMetadataDescription;

    TUniquePtr<UCesiumGltfComponent::HalfConstructed> pHalf =
        UCesiumGltfComponent::CreateOffGameThread(transform, options);
    return pHalf.Release();
  }

  virtual void* prepareInMainThread(
      Cesium3DTilesSelection::Tile& tile,
      void* pLoadThreadResult) override {
    const Cesium3DTilesSelection::TileContent& content = tile.getContent();
    if (content.isRenderContent()) {
      TUniquePtr<UCesiumGltfComponent::HalfConstructed> pHalf(
          reinterpret_cast<UCesiumGltfComponent::HalfConstructed*>(
              pLoadThreadResult));
      return UCesiumGltfComponent::CreateOnGameThread(
          this->_pActor,
          std::move(pHalf),
          _pActor->GetCesiumTilesetToUnrealRelativeWorldTransform(),
          this->_pActor->GetMaterial(),
          this->_pActor->GetTranslucentMaterial(),
          this->_pActor->GetWaterMaterial(),
          this->_pActor->GetCustomDepthParameters(),
          tile.getContentBoundingVolume().value_or(tile.getBoundingVolume()));
    }
    // UE_LOG(LogCesium, VeryVerbose, TEXT("No content for tile"));
    return nullptr;
  }

  virtual void free(
      Cesium3DTilesSelection::Tile& tile,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept override {
    if (pLoadThreadResult) {
      UCesiumGltfComponent::HalfConstructed* pHalf =
          reinterpret_cast<UCesiumGltfComponent::HalfConstructed*>(
              pLoadThreadResult);
      delete pHalf;
    } else if (pMainThreadResult) {
      UCesiumGltfComponent* pGltf =
          reinterpret_cast<UCesiumGltfComponent*>(pMainThreadResult);
      CesiumLifetime::destroyComponentRecursively(pGltf);
    }
  }

  virtual void* prepareRasterInLoadThread(
      const CesiumGltf::ImageCesium& image,
      const std::any& rendererOptions) override {
    auto ppOptions =
        std::any_cast<FRasterOverlayRendererOptions*>(&rendererOptions);
    check(ppOptions != nullptr && *ppOptions != nullptr);
    if (ppOptions == nullptr || *ppOptions == nullptr) {
      return nullptr;
    }

    auto pOptions = *ppOptions;

    auto texture = CesiumTextureUtility::loadTextureAnyThreadPart(
        image,
        TextureAddress::TA_Clamp,
        TextureAddress::TA_Clamp,
        pOptions->filter,
        pOptions->group,
        pOptions->useMipmaps,
        true); // TODO: sRGB should probably be configurable on the raster
               // overlay
    return texture.Release();
  }

  virtual void* prepareRasterInMainThread(
      const Cesium3DTilesSelection::RasterOverlayTile& /*rasterTile*/,
      void* pLoadThreadResult) override {

    TUniquePtr<CesiumTextureUtility::LoadedTextureResult> pLoadedTexture{
        static_cast<CesiumTextureUtility::LoadedTextureResult*>(
            pLoadThreadResult)};

    UTexture2D* pTexture =
        CesiumTextureUtility::loadTextureGameThreadPart(pLoadedTexture.Get());
    if (!pLoadedTexture || !pTexture) {
      return nullptr;
    }

    pTexture->AddToRoot();
    return pTexture;
  }

  virtual void freeRaster(
      const Cesium3DTilesSelection::RasterOverlayTile& rasterTile,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept override {
    if (pLoadThreadResult) {
      CesiumTextureUtility::LoadedTextureResult* pLoadedTexture =
          static_cast<CesiumTextureUtility::LoadedTextureResult*>(
              pLoadThreadResult);
      delete pLoadedTexture;
    }

    if (pMainThreadResult) {
      UTexture2D* pTexture = static_cast<UTexture2D*>(pMainThreadResult);
      pTexture->RemoveFromRoot();
      CesiumLifetime::destroy(pTexture);
    }
  }

  virtual void attachRasterInMainThread(
      const Cesium3DTilesSelection::Tile& tile,
      int32_t overlayTextureCoordinateID,
      const Cesium3DTilesSelection::RasterOverlayTile& rasterTile,
      void* pMainThreadRendererResources,
      const glm::dvec2& translation,
      const glm::dvec2& scale) override {
    const Cesium3DTilesSelection::TileContent& content = tile.getContent();
    const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
        content.getRenderContent();
    if (pRenderContent) {
      UCesiumGltfComponent* pGltfContent =
          reinterpret_cast<UCesiumGltfComponent*>(
              pRenderContent->getRenderResources());
      if (pGltfContent) {
        pGltfContent->AttachRasterTile(
            tile,
            rasterTile,
            static_cast<UTexture2D*>(pMainThreadRendererResources),
            translation,
            scale,
            overlayTextureCoordinateID);
      }
    }
  }

  virtual void detachRasterInMainThread(
      const Cesium3DTilesSelection::Tile& tile,
      int32_t overlayTextureCoordinateID,
      const Cesium3DTilesSelection::RasterOverlayTile& rasterTile,
      void* pMainThreadRendererResources) noexcept override {
    const Cesium3DTilesSelection::TileContent& content = tile.getContent();
    const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
        content.getRenderContent();
    if (pRenderContent) {
      UCesiumGltfComponent* pGltfContent =
          reinterpret_cast<UCesiumGltfComponent*>(
              pRenderContent->getRenderResources());
      if (pGltfContent) {
        pGltfContent->DetachRasterTile(
            tile,
            rasterTile,
            static_cast<UTexture2D*>(pMainThreadRendererResources));
      }
    }
  }

private:
  ACesium3DTileset* _pActor;
#if PHYSICS_INTERFACE_PHYSX
  IPhysXCookingModule* _pPhysXCookingModule;
#endif
};

static std::string getCacheDatabaseName() {
#if PLATFORM_ANDROID
  FString BaseDirectory = FPaths::ProjectPersistentDownloadDir();
#elif PLATFORM_IOS
  FString BaseDirectory =
      FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Cesium"));
  if (!IFileManager::Get().DirectoryExists(*BaseDirectory)) {
    IFileManager::Get().MakeDirectory(*BaseDirectory, true);
  }
#else
  FString BaseDirectory = FPaths::EngineUserDir();
#endif

  FString CesiumDBFile =
      FPaths::Combine(*BaseDirectory, TEXT("cesium-request-cache.sqlite"));
  FString PlatformAbsolutePath =
      IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(
          *CesiumDBFile);

  UE_LOG(
      LogCesium,
      Display,
      TEXT("Caching Cesium requests in %s"),
      *PlatformAbsolutePath);

  return TCHAR_TO_UTF8(*PlatformAbsolutePath);
}

void ACesium3DTileset::UpdateLoadStatus() {
  this->LoadProgress = this->_pTileset->computeLoadProgress();

  if (this->LoadProgress < 100 ||
      this->_lastTilesWaitingForOcclusionResults > 0) {
    this->_activeLoading = true;
  } else if (this->_activeLoading && this->LoadProgress == 100) {

    // There might be a few frames where nothing needs to be loaded as we
    // are waiting for occlusion results to come back, which means we are not
    // done with loading all the tiles in the tileset yet.
    if (this->_lastTilesWaitingForOcclusionResults == 0) {

      // Tileset just finished loading, we broadcast the update
      UE_LOG(LogCesium, Verbose, TEXT("Broadcasting OnTileLoaded"));
      OnTilesetLoaded.Broadcast();

      // Tileset remains 100% loaded if we don't have to reload it
      // so we don't want to keep on sending finished loading updates
      this->_activeLoading = false;
    }
  }
}

void ACesium3DTileset::LoadTileset() {
  static std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
      std::make_shared<CesiumAsync::CachingAssetAccessor>(
          spdlog::default_logger(),
          std::make_shared<UnrealAssetAccessor>(),
          std::make_shared<CesiumAsync::SqliteCache>(
              spdlog::default_logger(),
              getCacheDatabaseName()));
  static CesiumAsync::AsyncSystem asyncSystem(
      std::make_shared<UnrealTaskProcessor>());
  static TSharedRef<CesiumViewExtension, ESPMode::ThreadSafe>
      cesiumViewExtension =
          GEngine->ViewExtensions->NewExtension<CesiumViewExtension>();

  this->RootComponent->SetMobility(Mobility);

  if (this->_pTileset) {
    // Tileset already loaded, do nothing.
    return;
  }

  UWorld* pWorld = this->GetWorld();
  if (!pWorld) {
    return;
  }

  AWorldSettings* pWorldSettings = pWorld->GetWorldSettings();
  if (pWorldSettings && !pWorldSettings->bEnableLargeWorlds) {
    pWorldSettings->bEnableLargeWorlds = true;
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Cesium for Unreal has enabled the \"Enable Large Worlds\" option in this world's settings, as it is required in order to avoid serious culling problems with Cesium3DTilesets in Unreal Engine 5."),
        *this->Url);
  }

  // Both the feature flag and the CesiumViewExtension are global, not owned by
  // the Tileset. We're just applying one to the other here out of convenience.
  cesiumViewExtension->SetEnabled(
      GetDefault<UCesiumRuntimeSettings>()
          ->EnableExperimentalOcclusionCullingFeature);

  TArray<UCesiumRasterOverlay*> rasterOverlays;
  this->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

  const UCesiumEncodedMetadataComponent* pEncodedMetadataDescriptionComponent =
      this->FindComponentByClass<UCesiumEncodedMetadataComponent>();
  if (pEncodedMetadataDescriptionComponent) {
    this->_encodedMetadataDescription = {
        pEncodedMetadataDescriptionComponent->FeatureTables,
        pEncodedMetadataDescriptionComponent->FeatureTextures};
  } else {
    this->_encodedMetadataDescription = {};
  }

  ACesiumCreditSystem* pCreditSystem = this->ResolveCreditSystem();

  this->_cesiumViewExtension = cesiumViewExtension;

  if (GetDefault<UCesiumRuntimeSettings>()
          ->EnableExperimentalOcclusionCullingFeature &&
      this->EnableOcclusionCulling && !this->BoundingVolumePoolComponent) {
    const glm::dmat4& cesiumToUnreal =
        GetCesiumTilesetToUnrealRelativeWorldTransform();
    this->BoundingVolumePoolComponent =
        NewObject<UCesiumBoundingVolumePoolComponent>(this);
    this->BoundingVolumePoolComponent->SetUsingAbsoluteLocation(true);
    this->BoundingVolumePoolComponent->SetFlags(
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
    this->BoundingVolumePoolComponent->RegisterComponent();
    this->BoundingVolumePoolComponent->UpdateTransformFromCesium(
        cesiumToUnreal);
  }

  if (this->BoundingVolumePoolComponent) {
    this->BoundingVolumePoolComponent->initPool(this->OcclusionPoolSize);
  }

  Cesium3DTilesSelection::TilesetExternals externals{
      pAssetAccessor,
      std::make_shared<UnrealResourcePreparer>(this),
      asyncSystem,
      pCreditSystem ? pCreditSystem->GetExternalCreditSystem() : nullptr,
      spdlog::default_logger(),
      (GetDefault<UCesiumRuntimeSettings>()
           ->EnableExperimentalOcclusionCullingFeature &&
       this->EnableOcclusionCulling && this->BoundingVolumePoolComponent)
          ? this->BoundingVolumePoolComponent->getPool()
          : nullptr};

  this->_startTime = std::chrono::high_resolution_clock::now();

  Cesium3DTilesSelection::TilesetOptions options;

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
        ueDetails.HttpStatusCode =
            details.pRequest && details.pRequest->response()
                ? details.pRequest->response()->statusCode()
                : 0;
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

  switch (this->TilesetSource) {
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
    FString token =
        this->IonAccessToken.IsEmpty()
            ? GetDefault<UCesiumRuntimeSettings>()->DefaultIonAccessToken
            : this->IonAccessToken;
    if (!IonAssetEndpointUrl.IsEmpty()) {
      this->_pTileset = MakeUnique<Cesium3DTilesSelection::Tileset>(
          externals,
          static_cast<uint32_t>(this->IonAssetID),
          TCHAR_TO_UTF8(*token),
          options,
          TCHAR_TO_UTF8(*IonAssetEndpointUrl));
    } else {
      this->_pTileset = MakeUnique<Cesium3DTilesSelection::Tileset>(
          externals,
          static_cast<uint32_t>(this->IonAssetID),
          TCHAR_TO_UTF8(*token),
          options);
    }
    break;
  }

  for (UCesiumRasterOverlay* pOverlay : rasterOverlays) {
    if (pOverlay->IsActive()) {
      pOverlay->AddToTileset();
    }
  }

  switch (this->TilesetSource) {
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

  if (!this->_pTileset) {
    return;
  }

  this->_pTileset.Reset();

  switch (this->TilesetSource) {
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

  ACesiumCameraManager* pCameraManager =
      ACesiumCameraManager::GetDefaultCameraManager(this->GetWorld());
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
#if ENGINE_MAJOR_VERSION >= 5
      const auto leftEye = EStereoscopicEye::eSSE_LEFT_EYE;
      const auto rightEye = EStereoscopicEye::eSSE_RIGHT_EYE;
#else
      const auto leftEye = EStereoscopicPass::eSSP_LEFT_EYE;
      const auto rightEye = EStereoscopicPass::eSSP_RIGHT_EYE;
#endif

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
        CesiumReal one_over_tan_half_hfov = projection.M[0][0];

        CesiumReal hfov =
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

        CesiumReal one_over_tan_half_hfov = projection.M[0][0];

        CesiumReal hfov =
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
    const glm::dmat4& unrealWorldToTileset) {

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

  return Cesium3DTilesSelection::ViewState::create(
      tilesetCameraLocation,
      tilesetCameraFront,
      tilesetCameraUp,
      size,
      horizontalFieldOfView,
      verticalFieldOfView);
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

PRAGMA_DISABLE_DEPRECATION_WARNINGS

/**
 * @brief Check if the given tile is contained in one of the given exclusion
 * zones.
 *
 * TODO Add details here what that means
 * Old comment:
 * Consider Exclusion zone to drop this tile... Ideally, should be
 * considered in Cesium3DTilesSelection::ViewState to avoid loading the tile
 * first...
 *
 * @param exclusionZones The exclusion zones
 * @param tile The tile
 * @return The result of the test
 */
bool isInExclusionZone(
    const TArray<FCesiumExclusionZone>& exclusionZones,
    Cesium3DTilesSelection::Tile const* tile) {
  if (exclusionZones.Num() == 0) {
    return false;
  }
  // Apparently, only tiles with bounding REGIONS are
  // checked for the exclusion...
  const CesiumGeospatial::BoundingRegion* pRegion =
      std::get_if<CesiumGeospatial::BoundingRegion>(&tile->getBoundingVolume());
  if (!pRegion) {
    return false;
  }
  for (FCesiumExclusionZone ExclusionZone : exclusionZones) {
    CesiumGeospatial::GlobeRectangle cgExclusionZone =
        CesiumGeospatial::GlobeRectangle::fromDegrees(
            ExclusionZone.West,
            ExclusionZone.South,
            ExclusionZone.East,
            ExclusionZone.North);
    if (cgExclusionZone.computeIntersection(pRegion->getRectangle())) {
      return true;
    }
  }
  return false;
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS

void removeVisibleTilesFromList(
    std::vector<Cesium3DTilesSelection::Tile*>& list,
    const std::vector<Cesium3DTilesSelection::Tile*>& visibleTiles) {
  if (list.empty()) {
    return;
  }

  for (Cesium3DTilesSelection::Tile* pTile : visibleTiles) {
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
void hideTiles(const std::vector<Cesium3DTilesSelection::Tile*>& tiles) {
  for (Cesium3DTilesSelection::Tile* pTile : tiles) {
    if (pTile->getState() != Cesium3DTilesSelection::TileLoadState::Done) {
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
    if (Gltf && Gltf->IsVisible()) {
      Gltf->SetVisibility(false, true);
    } else {
      // TODO: why is this happening?
      UE_LOG(
          LogCesium,
          Verbose,
          TEXT("Tile to no longer render does not have a visible Gltf"));
    }
  }
}

/**
 * @brief Removes collision for tiles that have been removed from the render
 * list. This includes tiles that are fading out.
 */
void removeCollisionForTiles(
    const std::unordered_set<Cesium3DTilesSelection::Tile*>& tiles) {

  for (Cesium3DTilesSelection::Tile* pTile : tiles) {
    if (pTile->getState() != Cesium3DTilesSelection::TileLoadState::Done) {
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
    if (Gltf) {
      Gltf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
  }
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
  if (!this->LogSelectionStats) {
    return;
  }

  if (result.tilesToRenderThisFrame.size() != this->_lastTilesRendered ||
      result.tilesLoadingLowPriority != this->_lastTilesLoadingLowPriority ||
      result.tilesLoadingMediumPriority !=
          this->_lastTilesLoadingMediumPriority ||
      result.tilesLoadingHighPriority != this->_lastTilesLoadingHighPriority ||
      result.tilesVisited != this->_lastTilesVisited ||
      result.culledTilesVisited != this->_lastCulledTilesVisited ||
      result.tilesCulled != this->_lastTilesCulled ||
      result.tilesOccluded != this->_lastTilesOccluded ||
      result.tilesWaitingForOcclusionResults !=
          this->_lastTilesWaitingForOcclusionResults ||
      result.maxDepthVisited != this->_lastMaxDepthVisited) {

    this->_lastTilesRendered = result.tilesToRenderThisFrame.size();
    this->_lastTilesLoadingLowPriority = result.tilesLoadingLowPriority;
    this->_lastTilesLoadingMediumPriority = result.tilesLoadingMediumPriority;
    this->_lastTilesLoadingHighPriority = result.tilesLoadingHighPriority;

    this->_lastTilesVisited = result.tilesVisited;
    this->_lastCulledTilesVisited = result.culledTilesVisited;
    this->_lastTilesCulled = result.tilesCulled;
    this->_lastTilesOccluded = result.tilesOccluded;
    this->_lastTilesWaitingForOcclusionResults =
        result.tilesWaitingForOcclusionResults;
    this->_lastMaxDepthVisited = result.maxDepthVisited;

    UE_LOG(
        LogCesium,
        Display,
        TEXT(
            "%s: %d ms, Visited %d, Culled Visited %d, Rendered %d, Culled %d, Occluded %d, Waiting For Occlusion Results %d, Max Depth Visited: %d, Loading-Low %d, Loading-Medium %d, Loading-High %d, Loaded tiles %g%%"),
        *this->GetName(),
        (std::chrono::high_resolution_clock::now() - this->_startTime).count() /
            1000000,
        result.tilesVisited,
        result.culledTilesVisited,
        result.tilesToRenderThisFrame.size(),
        result.tilesCulled,
        result.tilesOccluded,
        result.tilesWaitingForOcclusionResults,
        result.maxDepthVisited,
        result.tilesLoadingLowPriority,
        result.tilesLoadingMediumPriority,
        result.tilesLoadingHighPriority,
        this->LoadProgress);
  }
}

void ACesium3DTileset::showTilesToRender(
    const std::vector<Cesium3DTilesSelection::Tile*>& tiles) {
  for (Cesium3DTilesSelection::Tile* pTile : tiles) {
    if (pTile->getState() != Cesium3DTilesSelection::TileLoadState::Done) {
      continue;
    }

    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    if (isInExclusionZone(ExclusionZones_DEPRECATED, pTile)) {
      continue;
    }
    PRAGMA_ENABLE_DEPRECATION_WARNINGS

    // That looks like some reeeally entertaining debug session...:
    // const Cesium3DTilesSelection::TileID& id = pTile->getTileID();
    // const CesiumGeometry::QuadtreeTileID* pQuadtreeID =
    // std::get_if<CesiumGeometry::QuadtreeTileID>(&id); if (!pQuadtreeID ||
    // pQuadtreeID->level != 14 || pQuadtreeID->x != 5503 || pQuadtreeID->y !=
    // 11626) { 	continue;
    //}

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

    applyActorCollisionSettings(BodyInstance, Gltf);

    if (Gltf->GetAttachParent() == nullptr) {

      // The AttachToComponent method is ridiculously complex,
      // so print a warning if attaching fails for some reason
      bool attached = Gltf->AttachToComponent(
          this->RootComponent,
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

    if (!Gltf->IsVisible()) {
      Gltf->SetVisibility(true, true);
    }

    Gltf->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  }
}

static void updateTileFade(Cesium3DTilesSelection::Tile* pTile, bool fadingIn) {
  if (!pTile || !pTile->getContent().isRenderContent()) {
    return;
  }

  if (pTile->getState() != Cesium3DTilesSelection::TileLoadState::Done) {
    return;
  }

  const Cesium3DTilesSelection::TileContent& content = pTile->getContent();
  const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
      content.getRenderContent();
  if (!pRenderContent) {
    return;
  }

  UCesiumGltfComponent* pGltf = reinterpret_cast<UCesiumGltfComponent*>(
      pRenderContent->getRenderResources());
  if (!pGltf) {
    return;
  }

  float percentage =
      pTile->getContent().getRenderContent()->getLodTransitionFadePercentage();

  pGltf->UpdateFade(percentage, fadingIn);
}

// Called every frame
void ACesium3DTileset::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

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
  if (cameras.empty()) {
    return;
  }

  glm::dmat4 unrealWorldToTileset = glm::affineInverse(
      this->GetCesiumTilesetToUnrealRelativeWorldTransform());

  std::vector<Cesium3DTilesSelection::ViewState> frustums;
  for (const FCesiumCamera& camera : cameras) {
    frustums.push_back(
        CreateViewStateFromViewParameters(camera, unrealWorldToTileset));
  }

  const Cesium3DTilesSelection::ViewUpdateResult& result =
      this->_captureMovieMode
          ? this->_pTileset->updateViewOffline(frustums)
          : this->_pTileset->updateView(frustums, DeltaTime);
  updateLastViewUpdateResultState(result);
  this->UpdateLoadStatus();

  removeCollisionForTiles(result.tilesFadingOut);

  removeVisibleTilesFromList(
      _tilesToHideNextFrame,
      result.tilesToRenderThisFrame);
  hideTiles(_tilesToHideNextFrame);

  _tilesToHideNextFrame.clear();
  for (Cesium3DTilesSelection::Tile* pTile : result.tilesFadingOut) {
    Cesium3DTilesSelection::TileRenderContent* pRenderContent =
        pTile->getContent().getRenderContent();
    if (!this->UseLodTransitions ||
        (pRenderContent &&
         pRenderContent->getLodTransitionFadePercentage() >= 1.0f)) {
      _tilesToHideNextFrame.push_back(pTile);
    }
  }

  showTilesToRender(result.tilesToRenderThisFrame);

  for (Cesium3DTilesSelection::Tile* pTile : result.tilesToRenderThisFrame) {
    updateTileFade(pTile, true);
  }

  for (Cesium3DTilesSelection::Tile* pTile : result.tilesFadingOut) {
    updateTileFade(pTile, false);
  }
}

void ACesium3DTileset::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  this->DestroyTileset();
  AActor::EndPlay(EndPlayReason);
}

void ACesium3DTileset::PostLoad() {
  BodyInstance.FixupData(this); // We need to call this one after Loading the
                                // actor to have correct BodyInstance values.

  Super::PostLoad();
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
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, IonAssetEndpointUrl) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, CreatePhysicsMeshes) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, AlwaysIncludeTangents) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, GenerateSmoothNormals) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, EnableWaterMask) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, Material) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, TranslucentMaterial) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, WaterMaterial) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, ApplyDpiScaling) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, EnableOcclusionCulling) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, Mobility) ||
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
#endif

void ACesium3DTileset::BeginDestroy() {
  this->InvalidateResolvedGeoreference();
  this->DestroyTileset();

  AActor::BeginDestroy();
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

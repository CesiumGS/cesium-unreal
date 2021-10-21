// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "Cesium3DTileset.h"
#include "Camera/CameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Cesium3DTilesSelection/BingMapsRasterOverlay.h"
#include "Cesium3DTilesSelection/CreditSystem.h"
#include "Cesium3DTilesSelection/GltfContent.h"
#include "Cesium3DTilesSelection/IPrepareRendererResources.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTilesSelection/TilesetOptions.h"
#include "Cesium3DTilesetRoot.h"
#include "CesiumAsync/CachingAssetAccessor.h"
#include "CesiumAsync/SqliteCache.h"
#include "CesiumCustomVersion.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumRasterOverlay.h"
#include "CesiumRuntime.h"
#include "CesiumTextureUtility.h"
#include "CesiumTransforms.h"
#include "Components/SceneCaptureComponent2D.h"
#include "CreateModelOptions.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "HttpModule.h"
#include "IPhysXCookingModule.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/EnumRange.h"
#include "PhysicsPublicCore.h"
#include "StereoRendering.h"
#include "UnrealAssetAccessor.h"
#include "UnrealTaskProcessor.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <spdlog/spdlog.h>

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
      _lastMaxDepthVisited(0),

      _captureMovieMode{false},
      _beforeMoviePreloadAncestors{PreloadAncestors},
      _beforeMoviePreloadSiblings{PreloadSiblings},
      _beforeMovieLoadingDescendantLimit{LoadingDescendantLimit},
      _beforeMovieKeepWorldOriginNearCamera{true},
      _tilesToNoLongerRenderNextFrame{} {

  PrimaryActorTick.bCanEverTick = true;

  this->SetActorEnableCollision(true);

  this->RootComponent =
      CreateDefaultSubobject<UCesium3DTilesetRoot>(TEXT("Tileset"));
  this->RootComponent->SetMobility(EComponentMobility::Static);

  PlatformName = UGameplayStatics::GetPlatformName();
}

ACesium3DTileset::~ACesium3DTileset() { this->DestroyTileset(); }

ACesiumGeoreference* ACesium3DTileset::GetGeoreference() const {
  return this->Georeference;
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
}

void ACesium3DTileset::SetTilesetSource(ETilesetSource InSource) {
  if (InSource != this->TilesetSource) {
    this->TilesetSource = InSource;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::SetUrl(FString InUrl) {
  if (InUrl != this->Url) {
    this->Url = InUrl;
    if (this->TilesetSource == ETilesetSource::FromUrl) {
      this->DestroyTileset();
    }
  }
}

void ACesium3DTileset::SetIonAssetID(int32 InAssetID) {
  if (InAssetID >= 0 && InAssetID != this->IonAssetID) {
    this->IonAssetID = InAssetID;
    if (this->TilesetSource == ETilesetSource::FromCesiumIon) {
      this->DestroyTileset();
    }
  }
}

void ACesium3DTileset::SetIonAccessToken(FString InAccessToken) {
  if (this->IonAccessToken != InAccessToken) {
    this->IonAccessToken = InAccessToken;
    if (this->TilesetSource == ETilesetSource::FromCesiumIon) {
      this->DestroyTileset();
    }
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

void ACesium3DTileset::SetWaterMaterial(UMaterialInterface* InMaterial) {
  if (this->WaterMaterial != InMaterial) {
    this->WaterMaterial = InMaterial;
    this->DestroyTileset();
  }
}

void ACesium3DTileset::PlayMovieSequencer() {
  this->_beforeMoviePreloadAncestors = this->PreloadAncestors;
  this->_beforeMoviePreloadSiblings = this->PreloadSiblings;
  this->_beforeMovieLoadingDescendantLimit = this->LoadingDescendantLimit;
  if (IsValid(this->ResolveGeoreference())) {
    this->_beforeMovieKeepWorldOriginNearCamera =
        this->ResolveGeoreference()->KeepWorldOriginNearCamera;
  }

  this->_captureMovieMode = true;
  this->PreloadAncestors = false;
  this->PreloadSiblings = false;
  this->LoadingDescendantLimit = 10000;
  if (IsValid(this->ResolveGeoreference())) {
    this->ResolveGeoreference()->KeepWorldOriginNearCamera = false;
  }
}

void ACesium3DTileset::StopMovieSequencer() {
  this->_captureMovieMode = false;
  this->PreloadAncestors = this->_beforeMoviePreloadAncestors;
  this->PreloadSiblings = this->_beforeMoviePreloadSiblings;
  this->LoadingDescendantLimit = this->_beforeMovieLoadingDescendantLimit;
  if (IsValid(this->ResolveGeoreference())) {
    this->ResolveGeoreference()->KeepWorldOriginNearCamera =
        this->_beforeMovieKeepWorldOriginNearCamera;
  }
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
      glm::dmat4 ENU = localGeoTransforms.ComputeEastNorthUpToEcef(center);
      glm::dvec3 offset =
          sphere.getRadius() * glm::normalize(ENU[0] + ENU[1] + ENU[2]);
      glm::dvec3 position = center + offset;
      return position;
    }

    glm::dvec3
    operator()(const CesiumGeometry::OrientedBoundingBox& orientedBoundingBox) {
      const glm::dvec3& center = orientedBoundingBox.getCenter();
      glm::dmat4 ENU = localGeoTransforms.ComputeEastNorthUpToEcef(center);
      const glm::dmat3& halfAxes = orientedBoundingBox.getHalfAxes();
      glm::dvec3 offset = glm::length(halfAxes[0] + halfAxes[1] + halfAxes[2]) *
                          glm::normalize(ENU[0] + ENU[1] + ENU[2]);
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
      transform * glm::dvec4(ecefCameraPosition, 1.0);

  // calculate unreal camera orientation
  glm::dvec3 ecefCenter =
      Cesium3DTilesSelection::getBoundingVolumeCenter(boundingVolume);
  glm::dvec3 unrealCenter = transform * glm::dvec4(ecefCenter, 1.0);
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
        _pPhysXCooking(
            pActor->GetCreatePhysicsMeshes()
                ? GetPhysXCookingModule()->GetPhysXCooking()
                : nullptr)
#endif
  {
  }

  virtual void* prepareInLoadThread(
      const CesiumGltf::Model& model,
      const glm::dmat4& transform) override {

    CreateModelOptions options;
    options.alwaysIncludeTangents = this->_pActor->GetAlwaysIncludeTangents();

#if PHYSICS_INTERFACE_PHYSX
    options.pPhysXCooking = this->_pPhysXCooking;
#endif

    std::unique_ptr<UCesiumGltfComponent::HalfConstructed> pHalf =
        UCesiumGltfComponent::CreateOffGameThread(model, transform, options);
    return pHalf.release();
  }

  virtual void* prepareInMainThread(
      Cesium3DTilesSelection::Tile& tile,
      void* pLoadThreadResult) override {
    const Cesium3DTilesSelection::TileContentLoadResult* pContent =
        tile.getContent();
    if (pContent && pContent->model) {
      std::unique_ptr<UCesiumGltfComponent::HalfConstructed> pHalf(
          reinterpret_cast<UCesiumGltfComponent::HalfConstructed*>(
              pLoadThreadResult));
      return UCesiumGltfComponent::CreateOnGameThread(
          this->_pActor,
          std::move(pHalf),
          _pActor->GetCesiumTilesetToUnrealRelativeWorldTransform(),
          this->_pActor->GetMaterial(),
          this->_pActor->GetWaterMaterial());
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
      this->destroyRecursively(pGltf);
    }
  }

  virtual void*
  prepareRasterInLoadThread(const CesiumGltf::ImageCesium& image) override {
    return (void*)CesiumTextureUtility::loadTextureAnyThreadPart(
        image,
        TextureAddress::TA_Clamp,
        TextureAddress::TA_Clamp,
        TextureFilter::TF_Bilinear);
  }

  virtual void* prepareRasterInMainThread(
      const Cesium3DTilesSelection::RasterOverlayTile& /*rasterTile*/,
      void* pLoadThreadResult) override {

    CesiumTextureUtility::LoadedTextureResult* pLoadedTexture =
        static_cast<CesiumTextureUtility::LoadedTextureResult*>(
            pLoadThreadResult);

    CesiumTextureUtility::loadTextureGameThreadPart(pLoadedTexture);

    if (!pLoadedTexture) {
      return nullptr;
    }

    UTexture2D* pTexture = pLoadedTexture->pTexture;
    pTexture->AddToRoot();

    delete pLoadedTexture;

    return (void*)pTexture;
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
    }
  }

  virtual void attachRasterInMainThread(
      const Cesium3DTilesSelection::Tile& tile,
      int32_t overlayTextureCoordinateID,
      const Cesium3DTilesSelection::RasterOverlayTile& rasterTile,
      void* pMainThreadRendererResources,
      const glm::dvec2& translation,
      const glm::dvec2& scale) override {
    const Cesium3DTilesSelection::TileContentLoadResult* pContent =
        tile.getContent();
    if (pContent && pContent->model) {
      UCesiumGltfComponent* pGltfContent =
          reinterpret_cast<UCesiumGltfComponent*>(tile.getRendererResources());
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
    const Cesium3DTilesSelection::TileContentLoadResult* pContent =
        tile.getContent();
    if (pContent && pContent->model) {
      UCesiumGltfComponent* pGltfContent =
          reinterpret_cast<UCesiumGltfComponent*>(tile.getRendererResources());
      if (pGltfContent) {
        pGltfContent->DetachRasterTile(
            tile,
            rasterTile,
            static_cast<UTexture2D*>(pMainThreadRendererResources));
      }
    }
  }

private:
  void destroyRecursively(USceneComponent* pComponent) {

    UE_LOG(
        LogCesium,
        VeryVerbose,
        TEXT("Destroying scene component recursively"));

    if (!pComponent) {
      return;
    }

    if (pComponent->IsRegistered()) {
      pComponent->UnregisterComponent();
    }

    TArray<USceneComponent*> children = pComponent->GetAttachChildren();
    for (USceneComponent* pChild : children) {
      this->destroyRecursively(pChild);
    }

    pComponent->DestroyPhysicsState();
    pComponent->DestroyComponent();

    UE_LOG(LogCesium, VeryVerbose, TEXT("Destroying scene component done"));
  }

  ACesium3DTileset* _pActor;
#if PHYSICS_INTERFACE_PHYSX
  IPhysXCooking* _pPhysXCooking;
#endif
};

static std::string getCacheDatabaseName() {
  // On Android, EngineUserDir returns a "fake" directory. UE's IPlatformFile
  // knows how to resolve it, but we can't pass it to cesium-native because
  // cesium-native expects a real path. IAndroidPlatformFile::FileRootPath
  // looks like it should be able to resolve it for us, but that's difficult
  // to call from here.

  // At the same time, apps on Android are isolated from each other and so we
  // can't really share a cache between them anyway. So we store the cache in a
  // different directory on Android.
#if PLATFORM_ANDROID
  FString baseDirectory = FPaths::ProjectPersistentDownloadDir();
#else
  FString baseDirectory = FPaths::EngineUserDir();
#endif

  FString filename =
      FPaths::Combine(baseDirectory, TEXT("cesium-request-cache.sqlite"));
  UE_LOG(LogCesium, Verbose, TEXT("Caching Cesium requests in %s"), *filename);
  return TCHAR_TO_UTF8(*filename);
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

  if (this->_pTileset) {
    // Tileset already loaded, do nothing.
    return;
  }

  TArray<UCesiumRasterOverlay*> rasterOverlays;
  this->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

  ACesiumCreditSystem* pCreditSystem = this->ResolveCreditSystem();

  Cesium3DTilesSelection::TilesetExternals externals{
      pAssetAccessor,
      std::make_shared<UnrealResourcePreparer>(this),
      asyncSystem,
      pCreditSystem ? pCreditSystem->GetExternalCreditSystem() : nullptr,
      spdlog::default_logger()};

  this->_startTime = std::chrono::high_resolution_clock::now();

  Cesium3DTilesSelection::TilesetOptions options;

  options.contentOptions.generateMissingNormalsSmooth =
      this->GenerateSmoothNormals;

  // TODO: figure out why water material crashes mac
#if PLATFORM_MAC
#else
  options.contentOptions.enableWaterMask = this->EnableWaterMask;
#endif

  switch (this->TilesetSource) {
  case ETilesetSource::FromUrl:
    UE_LOG(LogCesium, Log, TEXT("Loading tileset from URL %s"), *this->Url);
    this->_pTileset = new Cesium3DTilesSelection::Tileset(
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
    this->_pTileset = new Cesium3DTilesSelection::Tileset(
        externals,
        static_cast<uint32_t>(this->IonAssetID),
        TCHAR_TO_UTF8(*this->IonAccessToken),
        options);
    break;
  }

  for (UCesiumRasterOverlay* pOverlay : rasterOverlays) {
    if (pOverlay->IsActive()) {
      pOverlay->AddToTileset();
    }
  }

  if (this->Url.Len() > 0) {
    UE_LOG(
        LogCesium,
        Log,
        TEXT("Loading tileset from URL %s done"),
        *this->Url);
  } else {
    UE_LOG(
        LogCesium,
        Log,
        TEXT("Loading tileset for asset ID %d done"),
        this->IonAssetID);
  }
}

void ACesium3DTileset::DestroyTileset() {

  if (this->Url.Len() > 0) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Destroying tileset from URL %s"),
        *this->Url);
  } else {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Destroying tileset for asset ID %d"),
        this->IonAssetID);
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

  delete this->_pTileset;
  this->_pTileset = nullptr;

  if (this->Url.Len() > 0) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Destroying tileset from URL %s done"),
        *this->Url);
  } else {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Destroying tileset for asset ID %d done"),
        this->IonAssetID);
  }
}

std::vector<ACesium3DTileset::UnrealCameraParameters>
ACesium3DTileset::GetCameras() const {
  std::vector<UnrealCameraParameters> cameras = this->GetPlayerCameras();

  std::vector<UnrealCameraParameters> sceneCaptures = this->GetSceneCaptures();
  cameras.insert(
      cameras.end(),
      std::make_move_iterator(sceneCaptures.begin()),
      std::make_move_iterator(sceneCaptures.end()));

#if WITH_EDITOR
  std::vector<UnrealCameraParameters> editorCameras = this->GetEditorCameras();
  cameras.insert(
      cameras.end(),
      std::make_move_iterator(editorCameras.begin()),
      std::make_move_iterator(editorCameras.end()));
#endif

  return cameras;
}

std::vector<ACesium3DTileset::UnrealCameraParameters>
ACesium3DTileset::GetPlayerCameras() const {
  UWorld* pWorld = this->GetWorld();
  if (!pWorld) {
    return {};
  }

  float worldToMeters = 100.0f;
  AWorldSettings* pWorldSettings = pWorld->GetWorldSettings();
  if (pWorldSettings) {
    worldToMeters = pWorldSettings->WorldToMeters;
  }

  UGameViewportClient* pViewport = pWorld->GetGameViewport();
  if (!pViewport) {
    return {};
  }

  FVector2D size;
  pViewport->GetViewportSize(size);
  if (size.X < 1.0 || size.Y < 1.0) {
    return {};
  }

  TSharedPtr<IStereoRendering, ESPMode::ThreadSafe> pStereoRendering = nullptr;
  if (GEngine) {
    pStereoRendering = GEngine->StereoRenderingDevice;
  }

  bool useStereoRendering = false;
  if (pStereoRendering && pStereoRendering->IsStereoEnabled()) {
    useStereoRendering = true;
  }

  uint32 stereoLeftSizeX = static_cast<uint32>(size.X);
  uint32 stereoLeftSizeY = static_cast<uint32>(size.Y);
  uint32 stereoRightSizeX = static_cast<uint32>(size.X);
  uint32 stereoRightSizeY = static_cast<uint32>(size.Y);
  if (useStereoRendering) {
    int32 _x;
    int32 _y;

    pStereoRendering->AdjustViewRect(
        EStereoscopicPass::eSSP_LEFT_EYE,
        _x,
        _y,
        stereoLeftSizeX,
        stereoLeftSizeY);

    pStereoRendering->AdjustViewRect(
        EStereoscopicPass::eSSP_RIGHT_EYE,
        _x,
        _y,
        stereoRightSizeX,
        stereoRightSizeY);
  }

  FVector2D stereoLeftSize(stereoLeftSizeX, stereoRightSizeY);
  FVector2D stereoRightSize(stereoRightSizeX, stereoRightSizeY);

  std::vector<ACesium3DTileset::UnrealCameraParameters> cameras;
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

    if (useStereoRendering) {
      if (stereoLeftSize.X >= 1.0 && stereoLeftSize.Y >= 1.0) {
        FVector leftEyeLocation = location;
        FRotator leftEyeRotation = rotation;
        pStereoRendering->CalculateStereoViewOffset(
            EStereoscopicPass::eSSP_LEFT_EYE,
            leftEyeRotation,
            worldToMeters,
            leftEyeLocation);

        FMatrix projection = pStereoRendering->GetStereoProjectionMatrix(
            EStereoscopicPass::eSSP_LEFT_EYE);

        // TODO: consider assymetric frustums using 4 fovs
        float one_over_tan_half_hfov = projection.M[0][0];

        float hfov =
            glm::degrees(2.0f * glm::atan(1.0f / one_over_tan_half_hfov));

        cameras.push_back(UnrealCameraParameters{
            stereoLeftSize,
            leftEyeLocation,
            leftEyeRotation,
            hfov});
      }

      if (stereoRightSize.X >= 1.0 && stereoRightSize.Y >= 1.0) {
        FVector rightEyeLocation = location;
        FRotator rightEyeRotation = rotation;
        pStereoRendering->CalculateStereoViewOffset(
            EStereoscopicPass::eSSP_RIGHT_EYE,
            rightEyeRotation,
            worldToMeters,
            rightEyeLocation);

        FMatrix projection = pStereoRendering->GetStereoProjectionMatrix(
            EStereoscopicPass::eSSP_RIGHT_EYE);

        float one_over_tan_half_hfov = projection.M[0][0];

        float hfov =
            glm::degrees(2.0f * glm::atan(1.0f / one_over_tan_half_hfov));

        cameras.push_back(UnrealCameraParameters{
            stereoRightSize,
            rightEyeLocation,
            rightEyeRotation,
            hfov});
      }
    } else {
      cameras.push_back(UnrealCameraParameters{size, location, rotation, fov});
    }
  }

  return cameras;
}

std::vector<ACesium3DTileset::UnrealCameraParameters>
ACesium3DTileset::GetSceneCaptures() const {
  // TODO: really USceneCaptureComponent2D can be attached to any actor, is it
  // worth searching every actor? Might it be better to provide an interface
  // where users can volunteer cameras to be used with the tile selection as
  // needed?
  TArray<AActor*> sceneCaptures;
  static TSubclassOf<ASceneCapture2D> SceneCapture2D =
      ASceneCapture2D::StaticClass();
  UGameplayStatics::GetAllActorsOfClass(this, SceneCapture2D, sceneCaptures);

  std::vector<ACesium3DTileset::UnrealCameraParameters> cameras;
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
    float captureFov = pSceneCaptureComponent->FOVAngle;

    cameras.push_back(UnrealCameraParameters{
        renderTargetSize,
        captureLocation,
        captureRotation,
        captureFov});
  }

  return cameras;
}

/*static*/ Cesium3DTilesSelection::ViewState
ACesium3DTileset::CreateViewStateFromViewParameters(
    const UnrealCameraParameters& camera,
    const glm::dmat4& unrealWorldToTileset) {

  double horizontalFieldOfView =
      FMath::DegreesToRadians(camera.fieldOfViewDegrees);
  double aspectRatio = camera.viewportSize.X / camera.viewportSize.Y;
  double verticalFieldOfView =
      atan(tan(horizontalFieldOfView * 0.5) / aspectRatio) * 2.0;

  FVector direction = camera.rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
  FVector up = camera.rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));

  glm::dvec3 tilesetCameraLocation =
      unrealWorldToTileset *
      glm::dvec4(camera.location.X, camera.location.Y, camera.location.Z, 1.0);
  glm::dvec3 tilesetCameraFront = glm::normalize(
      unrealWorldToTileset *
      glm::dvec4(direction.X, direction.Y, direction.Z, 0.0));
  glm::dvec3 tilesetCameraUp =
      glm::normalize(unrealWorldToTileset * glm::dvec4(up.X, up.Y, up.Z, 0.0));

  return Cesium3DTilesSelection::ViewState::create(
      tilesetCameraLocation,
      tilesetCameraFront,
      tilesetCameraUp,
      glm::dvec2(camera.viewportSize.X, camera.viewportSize.Y),
      horizontalFieldOfView,
      verticalFieldOfView);
}

#if WITH_EDITOR
std::vector<ACesium3DTileset::UnrealCameraParameters>
ACesium3DTileset::GetEditorCameras() const {
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

  std::vector<ACesium3DTileset::UnrealCameraParameters> cameras;
  cameras.reserve(viewportClients.Num());

  for (FEditorViewportClient* pEditorViewportClient : viewportClients) {
    if (!pEditorViewportClient) {
      continue;
    }

    const FVector& location = pEditorViewportClient->GetViewLocation();
    const FRotator& rotation = pEditorViewportClient->GetViewRotation();
    double fov = pEditorViewportClient->FOVAngle;
    FIntPoint offset;
    FIntPoint size;
    pEditorViewportClient->GetViewportDimensions(offset, size);

    if (size.X < 1 || size.Y < 1) {
      continue;
    }

    cameras.push_back({size, location, rotation, fov});
  }

  return cameras;
}
#endif

bool ACesium3DTileset::ShouldTickIfViewportsOnly() const {
  return this->UpdateInEditor;
}

namespace {

// TODO These could or should be members, but extracted here as a first step:

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
void hideTilesToNoLongerRender(
    const std::vector<Cesium3DTilesSelection::Tile*>& tiles) {
  for (Cesium3DTilesSelection::Tile* pTile : tiles) {
    if (pTile->getState() != Cesium3DTilesSelection::Tile::LoadState::Done) {
      continue;
    }

    UCesiumGltfComponent* Gltf =
        static_cast<UCesiumGltfComponent*>(pTile->getRendererResources());
    if (Gltf && Gltf->IsVisible()) {
      Gltf->SetVisibility(false, true);
      Gltf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
  UCesiumGltfPrimitiveComponent* PrimitiveComponent =
      static_cast<UCesiumGltfPrimitiveComponent*>(Gltf->GetChildComponent(0));
  if (PrimitiveComponent != nullptr) {
    if (PrimitiveComponent->GetCollisionObjectType() !=
        BodyInstance.GetObjectType()) {
      PrimitiveComponent->SetCollisionObjectType(BodyInstance.GetObjectType());
    }
    const UEnum* ChannelEnum = StaticEnum<ECollisionChannel>();
    if (ChannelEnum) {
      FCollisionResponseContainer responseContainer =
          BodyInstance.GetResponseToChannels();
      PrimitiveComponent->SetCollisionResponseToChannels(responseContainer);
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
  options.enableFogCulling = this->EnableFogCulling;
  options.enforceCulledScreenSpaceError = this->EnforceCulledScreenSpaceError;
  options.culledScreenSpaceError =
      static_cast<double>(this->CulledScreenSpaceError);
}

void ACesium3DTileset::updateLastViewUpdateResultState(
    const Cesium3DTilesSelection::ViewUpdateResult& result) {
  if (result.tilesToRenderThisFrame.size() != this->_lastTilesRendered ||
      result.tilesLoadingLowPriority != this->_lastTilesLoadingLowPriority ||
      result.tilesLoadingMediumPriority !=
          this->_lastTilesLoadingMediumPriority ||
      result.tilesLoadingHighPriority != this->_lastTilesLoadingHighPriority ||
      result.tilesVisited != this->_lastTilesVisited ||
      result.culledTilesVisited != this->_lastCulledTilesVisited ||
      result.tilesCulled != this->_lastTilesCulled ||
      result.maxDepthVisited != this->_lastMaxDepthVisited) {

    this->_lastTilesRendered = result.tilesToRenderThisFrame.size();
    this->_lastTilesLoadingLowPriority = result.tilesLoadingLowPriority;
    this->_lastTilesLoadingMediumPriority = result.tilesLoadingMediumPriority;
    this->_lastTilesLoadingHighPriority = result.tilesLoadingHighPriority;

    this->_lastTilesVisited = result.tilesVisited;
    this->_lastCulledTilesVisited = result.culledTilesVisited;
    this->_lastTilesCulled = result.tilesCulled;
    this->_lastMaxDepthVisited = result.maxDepthVisited;

    UE_LOG(
        LogCesium,
        VeryVerbose,
        TEXT(
            "%s: %d ms, Visited %d, Culled Visited %d, Rendered %d, Culled %d, Max Depth Visited: %d, Loading-Low %d, Loading-Medium %d, Loading-High %d"),
        *this->GetName(),
        (std::chrono::high_resolution_clock::now() - this->_startTime).count() /
            1000000,
        result.tilesVisited,
        result.culledTilesVisited,
        result.tilesToRenderThisFrame.size(),
        result.tilesCulled,
        result.maxDepthVisited,
        result.tilesLoadingLowPriority,
        result.tilesLoadingMediumPriority,
        result.tilesLoadingHighPriority);
  }
}

void ACesium3DTileset::showTilesToRender(
    const std::vector<Cesium3DTilesSelection::Tile*>& tiles) {
  for (Cesium3DTilesSelection::Tile* pTile : tiles) {
    if (pTile->getState() != Cesium3DTilesSelection::Tile::LoadState::Done) {
      continue;
    }

    if (isInExclusionZone(ExclusionZones, pTile)) {
      continue;
    }

    // That looks like some reeeally entertaining debug session...:
    // const Cesium3DTilesSelection::TileID& id = pTile->getTileID();
    // const CesiumGeometry::QuadtreeTileID* pQuadtreeID =
    // std::get_if<CesiumGeometry::QuadtreeTileID>(&id); if (!pQuadtreeID ||
    // pQuadtreeID->level != 14 || pQuadtreeID->x != 5503 || pQuadtreeID->y !=
    // 11626) { 	continue;
    //}

    UCesiumGltfComponent* Gltf =
        static_cast<UCesiumGltfComponent*>(pTile->getRendererResources());
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
      Gltf->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
  }
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

  updateTilesetOptionsFromProperties();

  std::vector<UnrealCameraParameters> cameras = this->GetCameras();
  if (cameras.empty()) {
    return;
  }

  glm::dmat4 unrealWorldToTileset = glm::affineInverse(
      this->GetCesiumTilesetToUnrealRelativeWorldTransform());

  std::vector<Cesium3DTilesSelection::ViewState> frustums;
  for (const UnrealCameraParameters& camera : cameras) {
    frustums.push_back(
        CreateViewStateFromViewParameters(camera, unrealWorldToTileset));
  }

  const Cesium3DTilesSelection::ViewUpdateResult& result =
      this->_captureMovieMode ? this->_pTileset->updateViewOffline(frustums)
                              : this->_pTileset->updateView(frustums);
  updateLastViewUpdateResultState(result);

  removeVisibleTilesFromList(
      this->_tilesToNoLongerRenderNextFrame,
      result.tilesToRenderThisFrame);
  hideTilesToNoLongerRender(this->_tilesToNoLongerRenderNextFrame);
  this->_tilesToNoLongerRenderNextFrame = result.tilesToNoLongerRenderThisFrame;
  showTilesToRender(result.tilesToRenderThisFrame);
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

  if (PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, TilesetSource) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, Url) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, IonAssetID) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, IonAccessToken) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, CreatePhysicsMeshes) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, AlwaysIncludeTangents) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, GenerateSmoothNormals) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, EnableWaterMask) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, Material) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, WaterMaterial)) {
    this->DestroyTileset();
  } else if (
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, Georeference)) {
    this->InvalidateResolvedGeoreference();
  } else if (
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, CreditSystem)) {
    this->InvalidateResolvedCreditSystem();
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

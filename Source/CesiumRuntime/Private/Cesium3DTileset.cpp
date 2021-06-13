// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "Cesium3DTileset.h"
#include "Camera/PlayerCameraManager.h"
#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "Cesium3DTiles/CreditSystem.h"
#include "Cesium3DTiles/GltfContent.h"
#include "Cesium3DTiles/IPrepareRendererResources.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/TilesetOptions.h"
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
#include "CesiumTransforms.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HttpModule.h"
#include "IPhysXCookingModule.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequenceActor.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/EnumRange.h"
#include "PhysicsPublicCore.h"
#include "UnrealAssetAccessor.h"
#include "UnrealTaskProcessor.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
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
      CreditSystem(nullptr),

      _pTileset(nullptr),

      _lastTilesRendered(0),
      _lastTilesLoadingLowPriority(0),
      _lastTilesLoadingMediumPriority(0),
      _lastTilesLoadingHighPriority(0),

      _lastTilesVisited(0),
      _lastTilesCulled(0),
      _lastMaxDepthVisited(0),

      _waitingForBoundingVolume(false),

      _captureMovieMode{false},
      _beforeMoviePreloadAncestors{PreloadAncestors},
      _beforeMoviePreloadSiblings{PreloadSiblings},
      _beforeMovieLoadingDescendantLimit{LoadingDescendantLimit},
      _beforeMovieKeepWorldOriginNearCamera{true} {

  PrimaryActorTick.bCanEverTick = true;
  PrimaryActorTick.TickGroup = ETickingGroup::TG_PostUpdateWork;

  this->SetActorEnableCollision(true);

  this->RootComponent =
      CreateDefaultSubobject<UCesium3DTilesetRoot>(TEXT("Tileset"));
  this->RootComponent->SetMobility(EComponentMobility::Static);

  PlatformName = UGameplayStatics::GetPlatformName();
}

ACesium3DTileset::~ACesium3DTileset() { this->DestroyTileset(); }

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
    this->MarkTilesetDirty();
  }
}

void ACesium3DTileset::SetUrl(FString InUrl) {
  if (InUrl != this->Url) {
    this->Url = InUrl;
    if (this->TilesetSource == ETilesetSource::FromUrl) {
      this->MarkTilesetDirty();
    }
  }
}

void ACesium3DTileset::SetIonAssetID(int32 InAssetID) {
  if (InAssetID >= 0 && InAssetID != this->IonAssetID) {
    this->IonAssetID = InAssetID;
    if (this->TilesetSource == ETilesetSource::FromCesiumIon) {
      this->MarkTilesetDirty();
    }
  }
}

void ACesium3DTileset::SetIonAccessToken(FString InAccessToken) {
  if (this->IonAccessToken != InAccessToken) {
    this->IonAccessToken = InAccessToken;
    if (this->TilesetSource == ETilesetSource::FromCesiumIon) {
      this->MarkTilesetDirty();
    }
  }
}

void ACesium3DTileset::SetEnableWaterMask(bool bEnableMask) {
  if (this->EnableWaterMask != bEnableMask) {
    this->EnableWaterMask = bEnableMask;
    this->MarkTilesetDirty();
  }
}

void ACesium3DTileset::SetMaterial(UMaterialInterface* InMaterial) {
  if (this->Material != InMaterial) {
    this->Material = InMaterial;
    this->MarkTilesetDirty();
  }
}

void ACesium3DTileset::SetWaterMaterial(UMaterialInterface* InMaterial) {
  if (this->WaterMaterial != InMaterial) {
    this->WaterMaterial = InMaterial;
    this->MarkTilesetDirty();
  }
}

void ACesium3DTileset::SetOpacityMaskMaterial(UMaterialInterface* InMaterial) {
  if (this->OpacityMaskMaterial != InMaterial) {
    this->OpacityMaskMaterial = InMaterial;
    this->MarkTilesetDirty();
  }
}

void ACesium3DTileset::PlayMovieSequencer() {
  ACesiumGeoreference* cesiumGeoreference =
      ACesiumGeoreference::GetDefaultGeoreference(this);

  this->_beforeMoviePreloadAncestors = this->PreloadAncestors;
  this->_beforeMoviePreloadSiblings = this->PreloadSiblings;
  this->_beforeMovieLoadingDescendantLimit = this->LoadingDescendantLimit;
  this->_beforeMovieKeepWorldOriginNearCamera =
      cesiumGeoreference->KeepWorldOriginNearCamera;

  this->_captureMovieMode = true;
  this->PreloadAncestors = false;
  this->PreloadSiblings = false;
  this->LoadingDescendantLimit = 10000;
  cesiumGeoreference->KeepWorldOriginNearCamera = false;
}

void ACesium3DTileset::StopMovieSequencer() {
  ACesiumGeoreference* cesiumGeoreference =
      ACesiumGeoreference::GetDefaultGeoreference(this);
  this->_captureMovieMode = false;
  this->PreloadAncestors = this->_beforeMoviePreloadAncestors;
  this->PreloadSiblings = this->_beforeMoviePreloadSiblings;
  this->LoadingDescendantLimit = this->_beforeMovieLoadingDescendantLimit;
  cesiumGeoreference->KeepWorldOriginNearCamera =
      this->_beforeMovieKeepWorldOriginNearCamera;
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

    ACesiumGeoreference* localGeoreference;

    glm::dvec3 operator()(const CesiumGeometry::BoundingSphere& sphere) {
      const glm::dvec3& center = sphere.getCenter();
      glm::dmat4 ENU = glm::dmat4(1.0);
      if (IsValid(localGeoreference)) {
        ENU = localGeoreference->ComputeEastNorthUpToEcef(center);
      }
      glm::dvec3 offset =
          sphere.getRadius() * glm::normalize(ENU[0] + ENU[1] + ENU[2]);
      glm::dvec3 position = center + offset;
      return position;
    }

    glm::dvec3
    operator()(const CesiumGeometry::OrientedBoundingBox& orientedBoundingBox) {
      const glm::dvec3& center = orientedBoundingBox.getCenter();
      glm::dmat4 ENU = glm::dmat4(1.0);
      if (IsValid(localGeoreference)) {
        ENU = localGeoreference->ComputeEastNorthUpToEcef(center);
      }
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

  const Cesium3DTiles::Tile* pRootTile = this->_pTileset->getRootTile();
  if (!pRootTile) {
    return;
  }

  const Cesium3DTiles::BoundingVolume& boundingVolume =
      pRootTile->getBoundingVolume();

  // calculate unreal camera position
  const glm::dmat4& transform =
      this->GetCesiumTilesetToUnrealRelativeWorldTransform();
  glm::dvec3 ecefCameraPosition =
    std::visit(CalculateECEFCameraPosition{this->Georeference}, boundingVolume);
  glm::dvec3 unrealCameraPosition =
      transform * glm::dvec4(ecefCameraPosition, 1.0);

  // calculate unreal camera orientation
  glm::dvec3 ecefCenter =
      Cesium3DTiles::getBoundingVolumeCenter(boundingVolume);
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

bool ACesium3DTileset::_isBoundingVolumeReady() const {
  // TODO: detect failures that will cause the root tile to never exist.
  // That counts as "ready" too.
  return this->_pTileset && this->_pTileset->getRootTile();
}

std::optional<Cesium3DTiles::BoundingVolume>
ACesium3DTileset::GetBoundingVolume() const {
  if (!this->_isBoundingVolumeReady()) {
    return std::nullopt;
  }

  return this->_pTileset->getRootTile()->getBoundingVolume();
}

void ACesium3DTileset::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnreal) {
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

class UnrealResourcePreparer : public Cesium3DTiles::IPrepareRendererResources {
public:
  UnrealResourcePreparer(ACesium3DTileset* pActor)
      : _pActor(pActor)
#if PHYSICS_INTERFACE_PHYSX
        ,
        _pPhysXCooking(GetPhysXCookingModule()->GetPhysXCooking())
#endif
  {
  }

  virtual void* prepareInLoadThread(
      const CesiumGltf::Model& model,
      const glm::dmat4& transform) {
    std::unique_ptr<UCesiumGltfComponent::HalfConstructed> pHalf =
        UCesiumGltfComponent::CreateOffGameThread(
            model,
            transform
#if PHYSICS_INTERFACE_PHYSX
            ,
            this->_pPhysXCooking
#endif
        );
    return pHalf.release();
  }

  virtual void*
  prepareInMainThread(Cesium3DTiles::Tile& tile, void* pLoadThreadResult) {
    const Cesium3DTiles::TileContentLoadResult* pContent = tile.getContent();
    if (pContent && pContent->model) {
      std::unique_ptr<UCesiumGltfComponent::HalfConstructed> pHalf(
          reinterpret_cast<UCesiumGltfComponent::HalfConstructed*>(
              pLoadThreadResult));
      return UCesiumGltfComponent::CreateOnGameThread(
          this->_pActor,
          std::move(pHalf),
          _pActor->GetCesiumTilesetToUnrealRelativeWorldTransform(),
          this->_pActor->GetMaterial(),
          this->_pActor->GetWaterMaterial(),
          this->_pActor->GetOpacityMaskMaterial());
    }
    // UE_LOG(LogCesium, VeryVerbose, TEXT("No content for tile"));
    return nullptr;
  }

  virtual void free(
      Cesium3DTiles::Tile& tile,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept {
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
    return nullptr;
  }

  virtual void* prepareRasterInMainThread(
      const Cesium3DTiles::RasterOverlayTile& rasterTile,
      void* pLoadThreadResult) {
    const CesiumGltf::ImageCesium& image = rasterTile.getImage();
    if (image.width <= 0 || image.height <= 0) {
      return nullptr;
    }

    UTexture2D* pTexture =
        UTexture2D::CreateTransient(image.width, image.height, PF_R8G8B8A8);
    pTexture->AddToRoot();
    pTexture->AddressX = TextureAddress::TA_Clamp;
    pTexture->AddressY = TextureAddress::TA_Clamp;

    unsigned char* pTextureData = static_cast<unsigned char*>(
        pTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
    FMemory::Memcpy(
        pTextureData,
        image.pixelData.data(),
        image.pixelData.size());
    pTexture->PlatformData->Mips[0].BulkData.Unlock();

    pTexture->UpdateResource();

    return pTexture;
  }

  virtual void freeRaster(
      const Cesium3DTiles::RasterOverlayTile& rasterTile,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept override {
    UTexture2D* pTexture = static_cast<UTexture2D*>(pMainThreadResult);
    if (!pTexture) {
      return;
    }

    pTexture->RemoveFromRoot();
  }

  virtual void attachRasterInMainThread(
      const Cesium3DTiles::Tile& tile,
      uint32_t overlayTextureCoordinateID,
      const Cesium3DTiles::RasterOverlayTile& rasterTile,
      void* pMainThreadRendererResources,
      const CesiumGeometry::Rectangle& textureCoordinateRectangle,
      const glm::dvec2& translation,
      const glm::dvec2& scale) {
    const Cesium3DTiles::TileContentLoadResult* pContent = tile.getContent();
    if (pContent && pContent->model) {
      UCesiumGltfComponent* pGltfContent =
          reinterpret_cast<UCesiumGltfComponent*>(tile.getRendererResources());
      if (pGltfContent) {
        pGltfContent->AttachRasterTile(
            tile,
            rasterTile,
            static_cast<UTexture2D*>(pMainThreadRendererResources),
            textureCoordinateRectangle,
            translation,
            scale);
      }
    }
    // UE_LOG(LogCesium, VeryVerbose, TEXT("No content for attaching raster to
    // tile"));
  }

  virtual void detachRasterInMainThread(
      const Cesium3DTiles::Tile& tile,
      uint32_t overlayTextureCoordinateID,
      const Cesium3DTiles::RasterOverlayTile& rasterTile,
      void* pMainThreadRendererResources,
      const CesiumGeometry::Rectangle& textureCoordinateRectangle) noexcept
      override {
    const Cesium3DTiles::TileContentLoadResult* pContent = tile.getContent();
    if (pContent && pContent->model) {
      UCesiumGltfComponent* pGltfContent =
          reinterpret_cast<UCesiumGltfComponent*>(tile.getRendererResources());
      if (pGltfContent) {
        pGltfContent->DetachRasterTile(
            tile,
            rasterTile,
            static_cast<UTexture2D*>(pMainThreadRendererResources),
            textureCoordinateRectangle);
      }
    }
    // UE_LOG(LogCesium, VeryVerbose, TEXT("No content for detaching raster
    // from tile"));
  }

private:
  // static int32 GetObjReferenceCount(UObject* Obj, TArray<UObject*>*
  // OutReferredToObjects = nullptr)
  // {
  // 	if (!Obj ||
  // 		!Obj->IsValidLowLevelFast())
  // 	{
  // 		return -1;
  // 	}

  // 	TArray<UObject*> ReferredToObjects;             //req outer, ignore
  // archetype, recursive, ignore transient 	FReferenceFinder
  // ObjectReferenceCollector(ReferredToObjects, nullptr, false, true, true,
  // false); 	ObjectReferenceCollector.FindReferences(Obj);

  // 	if (OutReferredToObjects)
  // 	{
  // 		OutReferredToObjects->Append(ReferredToObjects);
  // 	}
  // 	return ReferredToObjects.Num();
  // }

  void destroyRecursively(USceneComponent* pComponent) {

    UE_LOG(
        LogCesium,
        VeryVerbose,
        TEXT("Destroying scene component recursively"));

    if (!pComponent) {
      return;
    }

    // FString newName = TEXT("Destroyed_") + pComponent->GetName();
    // pComponent->Rename(*newName);

    // TArray<UObject*> before;
    // GetObjReferenceCount(pComponent, &before);

    if (pComponent->IsRegistered()) {
      pComponent->UnregisterComponent();
    }

    TArray<USceneComponent*> children = pComponent->GetAttachChildren();
    for (USceneComponent* pChild : children) {
      this->destroyRecursively(pChild);
    }

    pComponent->DestroyPhysicsState();
    pComponent->DestroyComponent();

    // TArray<UObject*> after;
    // GetObjReferenceCount(pComponent, &after);

    // UE_LOG(LogActor, Warning, TEXT("Before %d, After %d"), before.Num(),
    // after.Num());

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

  Cesium3DTiles::Tileset* pTileset = this->_pTileset;

  TArray<UCesiumRasterOverlay*> rasterOverlays;
  this->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

  // The tileset already exists. If properties have been changed that require
  // the tileset to be recreated, then destroy the tileset. Otherwise, ignore.
  if (_tilesetIsDirty) {
    this->DestroyTileset();
    _tilesetIsDirty = false;
  } else {
    return;
  }

  if (!this->Georeference) {
    this->Georeference = ACesiumGeoreference::GetDefaultGeoreference(this);
  }

  UCesium3DTilesetRoot* pRoot = Cast<UCesium3DTilesetRoot>(this->RootComponent);
  if (pRoot) {
    this->Georeference->OnGeoreferenceUpdated.AddUniqueDynamic(
        pRoot,
        &UCesium3DTilesetRoot::HandleGeoreferenceUpdated);
  }
  // Add this as a ICesiumBoundingVolumeProvider once the bounding volume is
  // ready.
  this->_waitingForBoundingVolume = true;

  if (!this->CreditSystem) {
    this->CreditSystem = ACesiumCreditSystem::GetDefaultForActor(this);
  }

  Cesium3DTiles::TilesetExternals externals{
      pAssetAccessor,
      std::make_shared<UnrealResourcePreparer>(this),
      std::make_shared<UnrealTaskProcessor>(),
      this->CreditSystem ? this->CreditSystem->GetExternalCreditSystem()
                         : nullptr,
      spdlog::default_logger()};

  this->_startTime = std::chrono::high_resolution_clock::now();

  Cesium3DTiles::TilesetOptions options;
  // TODO: figure out why water material crashes mac
#if PLATFORM_MAC
#else
  options.contentOptions.enableWaterMask = this->EnableWaterMask;
#endif

  switch (this->TilesetSource) {
  case ETilesetSource::FromUrl:
    UE_LOG(LogCesium, Log, TEXT("Loading tileset from URL %s"), *this->Url);
    pTileset = new Cesium3DTiles::Tileset(
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
    pTileset = new Cesium3DTiles::Tileset(
        externals,
        static_cast<uint32_t>(this->IonAssetID),
        TCHAR_TO_UTF8(*this->IonAccessToken),
        options);
    break;
  }

  this->_pTileset = pTileset;

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

void ACesium3DTileset::MarkTilesetDirty() {
  _tilesetIsDirty = true;
  UE_LOG(LogCesium, Verbose, TEXT("Tileset marked Dirty"));
}

std::optional<ACesium3DTileset::UnrealCameraParameters>
ACesium3DTileset::GetCamera() const {
  std::optional<UnrealCameraParameters> camera = this->GetPlayerCamera();

#if WITH_EDITOR
  if (!camera) {
    camera = this->GetEditorCamera();
  }
#endif

  return camera;
}

std::optional<ACesium3DTileset::UnrealCameraParameters>
ACesium3DTileset::GetPlayerCamera() const {
  UWorld* pWorld = this->GetWorld();
  if (!pWorld) {
    return std::optional<UnrealCameraParameters>();
  }

  APlayerController* pPlayerController = pWorld->GetFirstPlayerController();
  if (!pPlayerController) {
    return std::optional<UnrealCameraParameters>();
  }

  APlayerCameraManager* pCameraManager = pPlayerController->PlayerCameraManager;
  if (!pCameraManager) {
    return std::optional<UnrealCameraParameters>();
  }

  UGameViewportClient* pViewport = pWorld->GetGameViewport();
  if (!pViewport) {
    return std::optional<UnrealCameraParameters>();
  }

  const FMinimalViewInfo& pov = pCameraManager->ViewTarget.POV;
  const FVector& location = pov.Location;
  const FRotator& rotation = pCameraManager->ViewTarget.POV.Rotation;
  double fov = pov.FOV;

  FVector2D size;
  pViewport->GetViewportSize(size);

  if (size.X < 1.0 || size.Y < 1.0) {
    return std::nullopt;
  }

  return UnrealCameraParameters{size, location, rotation, fov};
}

Cesium3DTiles::ViewState ACesium3DTileset::CreateViewStateFromViewParameters(
    const FVector2D& viewportSize,
    const FVector& location,
    const FRotator& rotation,
    double fieldOfViewDegrees) const {
  double horizontalFieldOfView = FMath::DegreesToRadians(fieldOfViewDegrees);

  double aspectRatio = viewportSize.X / viewportSize.Y;
  double verticalFieldOfView =
      atan(tan(horizontalFieldOfView * 0.5) / aspectRatio) * 2.0;

  FVector direction = rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
  FVector up = rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));

  glm::dmat4 unrealWorldToTileset = glm::affineInverse(
      this->GetCesiumTilesetToUnrealRelativeWorldTransform());

  return Cesium3DTiles::ViewState::create(
      unrealWorldToTileset *
          glm::dvec4(location.X, location.Y, location.Z, 1.0),
      glm::normalize(
          unrealWorldToTileset *
          glm::dvec4(direction.X, direction.Y, direction.Z, 0.0)),
      glm::normalize(unrealWorldToTileset * glm::dvec4(up.X, up.Y, up.Z, 0.0)),
      glm::dvec2(viewportSize.X, viewportSize.Y),
      horizontalFieldOfView,
      verticalFieldOfView);
}

#if WITH_EDITOR
std::optional<ACesium3DTileset::UnrealCameraParameters>
ACesium3DTileset::GetEditorCamera() const {
  FViewport* pViewport = GEditor->GetActiveViewport();
  FViewportClient* pViewportClient = pViewport->GetClient();
  FEditorViewportClient* pEditorViewportClient =
      static_cast<FEditorViewportClient*>(pViewportClient);
  const FVector& location = pEditorViewportClient->GetViewLocation();
  const FRotator& rotation = pEditorViewportClient->GetViewRotation();
  double fov = pEditorViewportClient->FOVAngle;
  FVector2D size = pViewport->GetSizeXY();

  if (size.X < 1.0 || size.Y < 1.0) {
    return std::nullopt;
  }

  return UnrealCameraParameters{size, location, rotation, fov};
}
#endif

bool ACesium3DTileset::ShouldTickIfViewportsOnly() const {
  return this->UpdateInEditor;
}

// Called every frame
void ACesium3DTileset::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);

  UCesium3DTilesetRoot* pRoot = Cast<UCesium3DTilesetRoot>(this->RootComponent);
  if (!pRoot) {
    return;
  }

  if (pRoot->IsTransformChanged()) {
    this->UpdateTransformFromCesium(
        this->GetCesiumTilesetToUnrealRelativeWorldTransform());
    pRoot->MarkTransformUnchanged();
  }

  // If a georeference update is waiting on the bounding volume being ready,
  // update when ready
  if (this->_waitingForBoundingVolume && this->_isBoundingVolumeReady()) {
    this->_waitingForBoundingVolume = false;
    // The bounding volume is ready so register as a
    // ICesiumBoundingVolumeProvider
    if (IsValid(this->Georeference)) {
      this->Georeference->AddBoundingVolumeProvider(this);
    }
  }

  if (this->SuspendUpdate) {
    return;
  }

  Cesium3DTiles::TilesetOptions& options = this->_pTileset->getOptions();
  options.maximumScreenSpaceError =
      static_cast<double>(this->MaximumScreenSpaceError);

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

  if (_tilesetIsDirty) {
    LoadTileset();
  }

  std::optional<UnrealCameraParameters> camera = this->GetCamera();
  if (!camera) {
    return;
  }

  Cesium3DTiles::ViewState tilesetViewState =
      this->CreateViewStateFromViewParameters(
          camera.value().viewportSize,
          camera.value().location,
          camera.value().rotation,
          camera.value().fieldOfViewDegrees);

  const Cesium3DTiles::ViewUpdateResult& result =
      this->_captureMovieMode
          ? this->_pTileset->updateViewOffline(tilesetViewState)
          : this->_pTileset->updateView(tilesetViewState);

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
        Verbose,
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

  for (Cesium3DTiles::Tile* pTile : result.tilesToNoLongerRenderThisFrame) {
    if (pTile->getState() != Cesium3DTiles::Tile::LoadState::Done) {
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
          VeryVerbose,
          TEXT("Tile to no longer render does not have a visible Gltf"));
    }
  }

  for (Cesium3DTiles::Tile* pTile : result.tilesToRenderThisFrame) {
    if (pTile->getState() != Cesium3DTiles::Tile::LoadState::Done) {
      continue;
    }

    // Consider Exclusion zone to drop this tile... Ideally, should be
    // considered in Cesium3DTiles::ViewState to avoid loading the tile
    // first...
    if (ExclusionZones.Num() > 0) {
      const CesiumGeospatial::BoundingRegion* pRegion =
          std::get_if<CesiumGeospatial::BoundingRegion>(
              &pTile->getBoundingVolume());
      if (pRegion) {
        bool culled = false;

        for (FCesiumExclusionZone ExclusionZone : ExclusionZones) {
          CesiumGeospatial::GlobeRectangle cgExclusionZone =
              CesiumGeospatial::GlobeRectangle::fromDegrees(
                  ExclusionZone.West,
                  ExclusionZone.South,
                  ExclusionZone.East,
                  ExclusionZone.North);
          if (cgExclusionZone.intersect(pRegion->getRectangle())) {
            culled = true;
            continue;
          }
        }

        if (culled) {
          continue;
        }
      }
    }

    // const Cesium3DTiles::TileID& id = pTile->getTileID();
    // const CesiumGeometry::QuadtreeTileID* pQuadtreeID =
    // std::get_if<CesiumGeometry::QuadtreeTileID>(&id); if (!pQuadtreeID ||
    // pQuadtreeID->level != 14 || pQuadtreeID->x != 5503 || pQuadtreeID->y !=
    // 11626) { 	continue;
    //}

    UCesiumGltfComponent* Gltf =
        static_cast<UCesiumGltfComponent*>(pTile->getRendererResources());
    if (!Gltf) {
      // TODO: Not-yet-renderable tiles shouldn't be here.
      // UE_LOG(LogCesium, VeryVerbose, TEXT("Tile to render does not have a
      // Gltf"));
      continue;
    }

    // Apply Actor-defined collision settings to the newly-created component.
    UCesiumGltfPrimitiveComponent* PrimitiveComponent =
        static_cast<UCesiumGltfPrimitiveComponent*>(Gltf->GetChildComponent(0));
    if (PrimitiveComponent != nullptr) {
      const UEnum* ChannelEnum = StaticEnum<ECollisionChannel>();
      if (ChannelEnum) {
        for (int32 ChannelValue = 0; ChannelValue < ECollisionChannel::ECC_MAX;
             ChannelValue++) {
          ECollisionResponse response =
              BodyInstance.GetCollisionResponse().GetResponse(
                  ECollisionChannel(ChannelValue));
          PrimitiveComponent->SetCollisionResponseToChannel(
              ECollisionChannel(ChannelValue),
              response);
        }
      }
    }

    if (Gltf->GetAttachParent() == nullptr) {
      Gltf->AttachToComponent(
          this->RootComponent,
          FAttachmentTransformRules::KeepRelativeTransform);
    }

    if (!Gltf->IsVisible()) {
      Gltf->SetVisibility(true, true);
      Gltf->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
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
  const FName PropName = (PropertyChangedEvent.Property)
                             ? PropertyChangedEvent.Property->GetFName()
                             : NAME_None;
  if (PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, TilesetSource) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, Url) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, IonAssetID) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, IonAccessToken) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, EnableWaterMask) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, Material) ||
      PropName == GET_MEMBER_NAME_CHECKED(ACesium3DTileset, WaterMaterial) ||
      PropName ==
          GET_MEMBER_NAME_CHECKED(ACesium3DTileset, OpacityMaskMaterial)) {
    MarkTilesetDirty();
  }
  Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void ACesium3DTileset::BeginDestroy() {
  this->DestroyTileset();

  AActor::BeginDestroy();
}

void ACesium3DTileset::Destroyed() {
  this->DestroyTileset();

  AActor::Destroyed();
}

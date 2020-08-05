// Fill out your copyright notice in the Description page of Project Settings.


#include "ACesium3DTileset.h"
#include "tiny_gltf.h"
#include "UnrealConversions.h"
#include "CesiumGltfComponent.h"
#include "HttpModule.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/GameViewportClient.h"
#include "UnrealAssetAccessor.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/Batched3DModelContent.h"
#include "UnrealTaskProcessor.h"
#include "Math/UnrealMathUtility.h"
#include "CesiumGeospatial/Transforms.h"
#include "IPhysXCookingModule.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Cartographic.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <iostream>

#ifdef WITH_EDITOR
#include "Slate/SceneViewport.h"
#include "EditorViewportClient.h"
#include "Editor.h"
#endif

#pragma warning(push)
#pragma warning(disable: 4946)
#include "json.hpp"
#pragma warning(pop)

// Sets default values
ACesium3DTileset::ACesium3DTileset() :
	_pTileset(nullptr)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	this->SetActorEnableCollision(true);

	this->RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Tileset"));
	this->RootComponent->SetMobility(EComponentMobility::Static);
}

ACesium3DTileset::~ACesium3DTileset() {
	this->DestroyTileset();
}

glm::dmat4x4 ACesium3DTileset::GetWorldToTilesetTransform() const {
	if (this->OriginPlacement == EOriginPlacement::TrueOrigin) {
		return glm::dmat4(1.0);
	}

	Cesium3DTiles::Tile* pRootTile = this->_pTileset->getRootTile();
	if (!pRootTile) {
		return glm::dmat4(1.0);
	}

	glm::dvec3 bvCenter;

	if (this->OriginPlacement == EOriginPlacement::BoundingVolumeOrigin) {
		const Cesium3DTiles::BoundingVolume& tilesetBoundingVolume = pRootTile->getBoundingVolume();
		bvCenter = Cesium3DTiles::getBoundingVolumeCenter(tilesetBoundingVolume);
	} else if (this->OriginPlacement == EOriginPlacement::CartographicOrigin) {
		const CesiumGeospatial::Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;
		bvCenter = ellipsoid.cartographicToCartesian(CesiumGeospatial::Cartographic::fromDegrees(this->OriginLongitude, this->OriginLatitude, this->OriginHeight));
	}

	if (this->AlignTilesetUpWithZ) {
		return CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(bvCenter);
	}
	else {
		return glm::translate(glm::dmat4(1.0), bvCenter);
	}
}

glm::dmat4x4 ACesium3DTileset::GetTilesetToWorldTransform() const
{
	return glm::affineInverse(this->GetWorldToTilesetTransform());
}

glm::dmat4x4 ACesium3DTileset::GetGlobalWorldToLocalWorldTransform() const {
	const FIntVector& originLocation = this->GetWorld()->OriginLocation;
	return glm::translate(
		glm::dmat4x4(1.0),
		glm::dvec3(-originLocation.X, originLocation.Y, -originLocation.Z) / 100.0
	);
}

glm::dmat4x4 ACesium3DTileset::GetLocalWorldToGlobalWorldTransform() const {
	const FIntVector& originLocation = this->GetWorld()->OriginLocation;
	return glm::translate(
		glm::dmat4x4(1.0),
		glm::dvec3(originLocation.X, -originLocation.Y, originLocation.Z) / 100.0
	);
}

// Called when the game starts or when spawned
void ACesium3DTileset::BeginPlay()
{
	Super::BeginPlay();

	this->LoadTileset();
}

void ACesium3DTileset::OnConstruction(const FTransform& Transform)
{
	this->LoadTileset();
}

void ACesium3DTileset::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	//std::cout << "Hit face index: " << Hit.FaceIndex << std::endl;

	//FHitResult detailedHit;
	//FCollisionQueryParams params;
	//params.bReturnFaceIndex = true;
	//params.bTraceComplex = true;
	//MyComp->LineTraceComponent(detailedHit, Hit.TraceStart, Hit.TraceEnd, params);

	//std::cout << "Hit face index 2: " << detailedHit.FaceIndex << std::endl;
}

static double centimetersPerMeter = 100.0;

// Scale Cesium's meters up to Unreal's centimeters.
static glm::dmat4x4 scaleToUnrealWorld = glm::dmat4x4(glm::dmat3x3(centimetersPerMeter));

// Transform Cesium's right-handed, Z-up coordinate system to Unreal's left-handed, Z-up coordinate
// system by inverting the Y coordinate. This same transformation can also go the other way.
static glm::dmat4x4 unrealToOrFromCesium(
	glm::dvec4(1.0, 0.0, 0.0, 0.0),
	glm::dvec4(0.0, -1.0, 0.0, 0.0),
	glm::dvec4(0.0, 0.0, 1.0, 0.0),
	glm::dvec4(0.0, 0.0, 0.0, 1.0)
);

class UnrealResourcePreparer : public Cesium3DTiles::IPrepareRendererResources {
public:
	UnrealResourcePreparer(ACesium3DTileset* pActor) :
		_pActor(pActor),
		_pPhysXCooking(GetPhysXCookingModule()->GetPhysXCooking())
	{}

	virtual void* prepareInLoadThread(const Cesium3DTiles::Tile& tile) {
		const Cesium3DTiles::TileContent* pContent = tile.getContent();
		if (!pContent) {
			return nullptr;
		}

		if (pContent->getType() == Cesium3DTiles::Batched3DModelContent::TYPE) {
			const Cesium3DTiles::Batched3DModelContent* pB3dm = static_cast<const Cesium3DTiles::Batched3DModelContent*>(tile.getContent());
			std::unique_ptr<UCesiumGltfComponent::HalfConstructed> pHalf = UCesiumGltfComponent::CreateOffGameThread(pB3dm->gltf(), tile.getTransform(), this->_pPhysXCooking);
			return pHalf.release();
		}

		return nullptr;
	}

	virtual void* prepareInMainThread(Cesium3DTiles::Tile& tile, void* pLoadThreadResult) {
		const Cesium3DTiles::TileContent* pContent = tile.getContent();
		if (!pContent) {
			return nullptr;
		}

		if (pContent->getType() == Cesium3DTiles::Batched3DModelContent::TYPE) {
			std::unique_ptr<UCesiumGltfComponent::HalfConstructed> pHalf(reinterpret_cast<UCesiumGltfComponent::HalfConstructed*>(pLoadThreadResult));
			glm::dmat4 globalToLocal = _pActor->GetGlobalWorldToLocalWorldTransform();
			glm::dmat4 tilesetToWorld = _pActor->GetTilesetToWorldTransform();
			glm::dmat4 tilesetToUnrealTransform = unrealToOrFromCesium * scaleToUnrealWorld * globalToLocal * tilesetToWorld;
			return UCesiumGltfComponent::CreateOnGameThread(this->_pActor, std::move(pHalf), tilesetToUnrealTransform);
		}

		return nullptr;
	}

	virtual void free(Cesium3DTiles::Tile& tile, void* pLoadThreadResult, void* pMainThreadResult) {
		if (pLoadThreadResult) {
			UCesiumGltfComponent::HalfConstructed* pHalf = reinterpret_cast<UCesiumGltfComponent::HalfConstructed*>(pLoadThreadResult);
			delete pHalf;
		} else if (pMainThreadResult) {
			UCesiumGltfComponent* pGltf = reinterpret_cast<UCesiumGltfComponent*>(pMainThreadResult);
			if (pGltf) {
				this->destroyRecursively(pGltf);
			}
		}
	}

private:
	void destroyRecursively(USceneComponent* pComponent) {
		if (pComponent->IsRegistered()) {
			pComponent->UnregisterComponent();
		}

		TArray<USceneComponent*> children = pComponent->GetAttachChildren();
		for (USceneComponent* pChild : children) {
			this->destroyRecursively(pChild);
		}

		pComponent->DestroyComponent();
	}

	ACesium3DTileset* _pActor;
	IPhysXCooking* _pPhysXCooking;
};

void ACesium3DTileset::LoadTileset()
{
	Cesium3DTiles::Tileset* pTileset = this->_pTileset;
	if (pTileset)
	{
		if (this->Url.Len() > 0)
		{
			if (pTileset->getUrl() && wstr_to_utf8(this->Url) == pTileset->getUrl())
			{
				// Already using this URL.
				return;
			}
		}
		else
		{
			if (pTileset->getIonAssetID() && pTileset->getIonAccessToken() && this->IonAssetID == pTileset->getIonAssetID() && wstr_to_utf8(this->IonAccessToken) == pTileset->getIonAccessToken())
			{
				// Already using this asset ID and access token.
				return;
			}
		}

		this->DestroyTileset();
	}

	Cesium3DTiles::TilesetExternals externals{
		new UnrealAssetAccessor(),
		new UnrealResourcePreparer(this),
		new UnrealTaskProcessor()
	};

	if (this->Url.Len() > 0)
	{
		pTileset = new Cesium3DTiles::Tileset(externals, wstr_to_utf8(this->Url));
	}
	else
	{
		pTileset = new Cesium3DTiles::Tileset(externals, this->IonAssetID, wstr_to_utf8(this->IonAccessToken));
	}

	this->_pTileset = pTileset;
}

void ACesium3DTileset::DestroyTileset() {
	if (!this->_pTileset) {
		return;
	}

	Cesium3DTiles::TilesetExternals externals = this->_pTileset->getExternals();
	delete this->_pTileset;
	delete externals.pAssetAccessor;
	delete externals.pPrepareRendererResources;
	delete externals.pTaskProcessor;
	this->_pTileset = nullptr;
}

std::optional<ACesium3DTileset::UnrealCameraParameters> ACesium3DTileset::GetCamera() const {
	std::optional<UnrealCameraParameters> camera = this->GetPlayerCamera();

#ifdef WITH_EDITOR
	//if (!camera) {
	//	camera = this->GetEditorCamera();
	//}
#endif

	return camera;
}

std::optional<ACesium3DTileset::UnrealCameraParameters> ACesium3DTileset::GetPlayerCamera() const
{
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

	Cesium3DTiles::Tile* pRootTile = this->_pTileset->getRootTile();
	if (!pRootTile) {
		return std::optional<UnrealCameraParameters>();
	}

	const FMinimalViewInfo& pov = pCameraManager->ViewTarget.POV;
	const FVector& location = pov.Location;
	const FRotator& rotation = pCameraManager->ViewTarget.POV.Rotation;
	double fov = pov.FOV;

	FVector2D size;
	pViewport->GetViewportSize(size);

	return UnrealCameraParameters {
		size,
		location,
		rotation,
		fov
	};
}

Cesium3DTiles::Camera ACesium3DTileset::CreateCameraFromViewParameters(
	const FVector2D& viewportSize,
	const FVector& location,
	const FRotator& rotation,
	double fieldOfViewDegrees
) const {
	double horizontalFieldOfView = FMath::DegreesToRadians(fieldOfViewDegrees);

	double aspectRatio = viewportSize.X / viewportSize.Y;
	double verticalFieldOfView = atan(tan(horizontalFieldOfView * 0.5) / aspectRatio) * 2.0;

	FVector direction = rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
	FVector up = rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));

	auto tryTransform = [](const FVector& v) {
		return glm::dvec3(v.X, -v.Y, v.Z);
	};

	const FTransform& tilesetToWorld = this->RootComponent->GetComponentToWorld();
	FVector locationRelativeToTileset = tilesetToWorld.InverseTransformPosition(location);
	FVector directionRelativeToTileset = tilesetToWorld.InverseTransformVector(direction);
	FVector upRelativeToTileset = tilesetToWorld.InverseTransformVector(up);

	glm::dvec3 cesiumPosition = tryTransform(locationRelativeToTileset) / 100.0;
	glm::dvec3 cesiumDirection = tryTransform(directionRelativeToTileset);
	glm::dvec3 cesiumUp = tryTransform(upRelativeToTileset);

	glm::dmat4x4 transform = this->GetWorldToTilesetTransform(); /* * this->GetLocalWorldToGlobalWorldTransform();*/
	//glm::dvec3 first = this->GetLocalWorldToGlobalWorldTransform() * glm::dvec4(cesiumPosition, 1.0);
	//glm::dvec3 second = this->GetWorldToTilesetTransform() * glm::dvec4(first, 1.0);

	return Cesium3DTiles::Camera(
		//second,
		transform * glm::dvec4(cesiumPosition, 1.0),
		transform * glm::dvec4(cesiumDirection, 0.0),
		transform * glm::dvec4(cesiumUp, 0.0),
		glm::dvec2(viewportSize.X, viewportSize.Y),
		horizontalFieldOfView,
		verticalFieldOfView
	);
}

#ifdef WITH_EDITOR
std::optional<ACesium3DTileset::UnrealCameraParameters> ACesium3DTileset::GetEditorCamera() const
{
	FViewport* pViewport = GEditor->GetActiveViewport();
	FViewportClient* pViewportClient = pViewport->GetClient();
	FEditorViewportClient* pEditorViewportClient = (FEditorViewportClient*)pViewportClient;
	const FVector& location = pEditorViewportClient->GetViewLocation();
	const FRotator& rotation = pEditorViewportClient->GetViewRotation();
	double fov = pEditorViewportClient->FOVAngle;
	FVector2D size = pViewport->GetSizeXY();

	return UnrealCameraParameters{
		size,
		location,
		rotation,
		fov
	};
}
#endif

bool ACesium3DTileset::ShouldTickIfViewportsOnly() const {
	return this->ShowInEditor;
}

// Called every frame
void ACesium3DTileset::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (this->SuspendUpdate) {
		return;
	}

	std::optional<UnrealCameraParameters> camera = this->GetCamera();
	if (!camera) {
		return;
	}

	// Adjust the world origin to be near the camera.
	const FVector& ueCameraPosition = camera.value().location;
	const FIntVector& originLocation = this->GetWorld()->OriginLocation;
	GetWorld()->SetNewWorldOrigin(FIntVector(
		static_cast<int32>(ueCameraPosition.X) + static_cast<int32>(originLocation.X),
		static_cast<int32>(ueCameraPosition.Y) + static_cast<int32>(originLocation.Y),
		static_cast<int32>(ueCameraPosition.Z) + static_cast<int32>(originLocation.Z)
	));

	camera = this->GetCamera();
	
	Cesium3DTiles::Camera tilesetCamera = this->CreateCameraFromViewParameters(
		camera.value().viewportSize,
		camera.value().location,
		camera.value().rotation,
		camera.value().fieldOfViewDegrees
	);

	const Cesium3DTiles::ViewUpdateResult& result = this->_pTileset->updateView(tilesetCamera);

	for (Cesium3DTiles::Tile* pTile : result.tilesToNoLongerRenderThisFrame) {
		if (pTile->getState() != Cesium3DTiles::Tile::LoadState::Done) {
			continue;
		}

		UCesiumGltfComponent* Gltf = static_cast<UCesiumGltfComponent*>(pTile->getRendererResources());
		if (Gltf && Gltf->IsVisible()) {
			Gltf->SetVisibility(false, true);
		} else {
			// TODO: why is this happening?
		}
	}

	for (Cesium3DTiles::Tile* pTile : result.tilesToRenderThisFrame) {
		if (pTile->getState() != Cesium3DTiles::Tile::LoadState::Done) {
			continue;
		}

		Cesium3DTiles::TileContent* pContent = pTile->getContent();
		if (!pContent) {
			continue;
		}

		UCesiumGltfComponent* Gltf = static_cast<UCesiumGltfComponent*>(pTile->getRendererResources());
		if (!Gltf) {
			// TODO: Not-yet-renderable tiles shouldn't be here.
			continue;
		}

		if (Gltf->GetAttachParent() == nullptr) {
			Gltf->AttachToComponent(this->RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}

		if (!Gltf->IsVisible()) {
			Gltf->SetVisibility(true, true);
		}
	}
}

void ACesium3DTileset::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	this->DestroyTileset();
	AActor::EndPlay(EndPlayReason);
}

void ACesium3DTileset::BeginDestroy() {
	this->DestroyTileset();
	AActor::BeginDestroy();
}
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include <glm/mat4x4.hpp>
#include "Cesium3DTiles/ViewState.h"
#include "CesiumGeoreferenceable.h"
#include "CesiumGeoreference.h"
#include "CesiumCreditSystem.h"
#include <chrono>
#include "ACesium3DTileset.generated.h"

class UMaterial;

namespace Cesium3DTiles {
	class Tileset;
	class TilesetView;
}

UCLASS()
class CESIUM_API ACesium3DTileset :
	public AActor,
	public ICesiumGeoreferenceable
{
	GENERATED_BODY()
	
public:	
	ACesium3DTileset();
	virtual ~ACesium3DTileset();
	
	/**
	 * The URL of this tileset's "tileset.json" file. If this property is specified, the ion asset ID
	 * and token are ignored.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	FString Url;

	/**
	 * The ID of the Cesium ion asset to use. This property is ignored if the Url is
	 * specified.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	uint32 IonAssetID;

	/**
	 * The access token to use to access the Cesium ion resource.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="IonAssetID"))
	FString IonAccessToken;

	/**
	 * The actor controlling how this tileset's coordinate system relates to the coordinate system in
	 * this Unreal Engine level.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	ACesiumGeoreference* Georeference;

	/**
	 * The actor managing this tileset's content attributions.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	ACesiumCreditSystem* CreditSystem;

	/**
	 * The maximum number of pixels of error when rendering this tileset.
     * This is used to select an appropriate level-of-detail.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium|Level of Detail")
	double MaximumScreenSpaceError = 16.0;

	/**
	 * Whether to preload ancestor tiles.
	 */ 
	UPROPERTY(EditAnywhere, Category="Cesium|Tile Loading")
	bool PreloadAncestors = true;

	/**
	 * Whether to preload sibling tiles.
	 */ 
	UPROPERTY(EditAnywhere, Category="Cesium|Tile Loading")
	bool PreloadSiblings = true;

	/**
	 * Whether to unrefine back to a parent tile when a child isn't done loading.
	 */ 
	UPROPERTY(EditAnywhere, Category="Cesium|Tile Loading")
	bool ForbidHoles = false;

	/**
	 * The maximum number of tiles that may be loaded at once.
	 */ 
	UPROPERTY(EditAnywhere, Category="Cesium|Tile Loading")
	int MaximumSimultaneousTileLoads = 20;

	/**
	 * The number of loading descendents a tile should allow before deciding to render itself instead of waiting.
	 */ 
	UPROPERTY(EditAnywhere, Category="Cesium|Tile Loading")
	int LoadingDescendantLimit = 20;

	/**
	 * Whether to cull tiles that are outside the frustum.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Tile Culling")
	bool EnableFrustumCulling = true;

	/**
	 * Whether to cull tiles that are occluded by fog.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Tile Culling")
	bool EnableFogCulling = true;

	/**
	 * Whether culled screen-space error should be enforced.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Tile Culling")
	bool EnforceCulledScreenSpaceError = false;

	/**
	 * The screen-space error to be used for culled tiles.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Tile Culling", meta=(EditCondition="EnforceCulledScreenSpaceError"))
	double CulledScreenSpaceError = 64.0;

	UPROPERTY(EditAnywhere, Category = "Cesium|Rendering")
	UMaterial* Material = nullptr;

	/**
	 * Pauses level-of-detail and culling updates of this tileset.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Debug")
	bool SuspendUpdate;

	/**
	 * If true, this tileset is loaded and shown in the editor. If false, is only shown while
	 * playing (including Play-in-Editor).
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
	virtual std::optional<Cesium3DTiles::BoundingVolume> GetBoundingVolume() const override;
	void UpdateTransformFromCesium(const glm::dmat4& cesiumToUnreal);
	virtual void UpdateGeoreferenceTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void NotifyHit
	(
		class UPrimitiveComponent* MyComp,
		AActor* Other,
		class UPrimitiveComponent* OtherComp,
		bool bSelfMoved,
		FVector HitLocation,
		FVector HitNormal,
		FVector NormalImpulse,
		const FHitResult& Hit
	) override;

	void LoadTileset();
	void DestroyTileset();
	Cesium3DTiles::ViewState CreateViewStateFromViewParameters(
		const FVector2D& viewportSize,
		const FVector& location,
		const FRotator& rotation,
		double fieldOfViewDegrees
	) const;

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
#endif

public:	
	// Called every frame
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void Tick(float DeltaTime) override;
	virtual void BeginDestroy() override;
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
	bool _captureMovieMode;
	std::chrono::high_resolution_clock::time_point _startTime;
};

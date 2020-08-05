// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include <glm/mat4x4.hpp>
#include "Cesium3DTiles/Camera.h"
#include "ACesium3DTileset.generated.h"

namespace Cesium3DTiles {
	class Tileset;
	class TilesetView;
}

UENUM(BlueprintType)
enum class EOriginPlacement : uint8 {
	/**
	 * Use the tileset's true origin as the Actor's origin. For georeferenced
	 * tilesets, this usually means the Actor's origin will be at the center
	 * of the Earth.
	 */
	TrueOrigin UMETA(DisplayName = "True origin"),

	/*
	 * Use the center of the tileset's bounding volume as the Actor's origin. This option
	 * preserves precision by keeping all tileset vertices as close to the Actor's origin
	 * as possible.
	 */
	BoundingVolumeOrigin UMETA(DisplayName = "Bounding volume center"),

	/**
	 * Use a custom position within the tileset as the Actor's origin. The position is
	 * expressed as a longitude, latitude, and height, and that position within the tileset
	 * will be at coordinate (0,0,0) in the Actor's coordinate system.
	 */
	CartographicOrigin UMETA(DisplayName = "Longitude / latitude / height")
};

UCLASS()
class CESIUM_API ACesium3DTileset : public AActor
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
	 * The placement of this Actor's origin (coordinate 0,0,0) within the tileset. 3D Tiles tilesets often
	 * use Earth-centered, Earth-fixed coordinates, such that the tileset content is in a small bounding
	 * volume 6-7 million meters (the radius of the Earth) away from the coordinate system origin.
	 * This property allows an alternative position, other then the tileset's true origin, to be treated
	 * as the origin for the purpose of this Actor. Using this property will preserve vertex precision
	 * (and thus avoid jittering) much better precision than setting the Actor's Transform property.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium")
	EOriginPlacement OriginPlacement = EOriginPlacement::BoundingVolumeOrigin;

	/**
	 * The longitude of the custom origin placement in degrees.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginLongitude = 0.0;

	/**
	 * The latitude of the custom origin placement in degrees.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium", meta = (EditCondition = "OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginLatitude= 0.0;

	/**
	 * The height of the custom origin placement in meters above the WGS84 ellipsoid.
	 */
	UPROPERTY(EditAnywhere, Category = "Cesium", meta = (EditCondition = "OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginHeight = 0.0;

	/**
	 * If true, the tileset is rotated so that the local up at the center of the tileset's bounding
	 * volume is aligned with the usual Unreal Engine up direction, +Z. This is useful because
	 * 3D Tiles tilesets often use Earth-centered, Earth-fixed coordinates in which the local
	 * up direction depends on where you are on the Earth. If false, the tileset's true rotation
	 * is used.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	bool AlignTilesetUpWithZ = true;

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

	/**
	 * Gets a 4x4 matrix that transforms coordinates in the global Unreal world coordinate system
	 * (that is, accounting for the world's OriginLocation) and transforms them to the tileset's
	 * coordinate system.
	 */
	glm::dmat4x4 GetWorldToTilesetTransform() const;
	
	/**
	 * Gets a 4x4 matrix that transforms coordinates in the tileset's coordinate system to the
	 * global Unreal coordinate system (that is, accounting for the world's OriginLocation).
	 */
	glm::dmat4x4 GetTilesetToWorldTransform() const;

	/**
	 * Gets a 4x4 matrix that transforms coordinates in the global world coordinate system (that is,
	 * the one that accounts for the world's OriginLocation) and transforms them to the local world
	 * coordinate system (that is, relative to the floating OriginLocation).
	 */
	glm::dmat4x4 GetGlobalWorldToLocalWorldTransform() const;

	/**
	 * Gets a 4x4 matrix that transforms coordinate in the local world coordinate system (that is,
	 * relative to the floating OriginLocation) and transforms them to the global world coordinate
	 * system (that is, account for the world's OriginLocation).
	 */
	glm::dmat4x4 GetLocalWorldToGlobalWorldTransform() const;

	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;

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
	Cesium3DTiles::Camera CreateCameraFromViewParameters(
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

#ifdef WITH_EDITOR
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
};

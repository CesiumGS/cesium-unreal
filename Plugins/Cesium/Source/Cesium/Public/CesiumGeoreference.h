// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CesiumGeoreferenceable.h"
#include "UObject/WeakInterfacePtr.h"
#include "CesiumGeoreference.generated.h"

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

class APlayerCameraManager;

/**
 * Controls how global geospatial coordinates are mapped to coordinates in the Unreal Engine level.
 * Internally, Cesium uses a global Earth-centered, Earth-fixed (ECEF) ellipsoid-centered coordinate system,
 * where the ellipsoid is usually the World Geodetic System 1984 (WGS84) ellipsoid.
 * This is a right-handed system centered at the Earth's center of mass, where +X is in the direction of the
 * intersection of the Equator and the Prime Meridian (zero degrees longitude), +Y is in the direction of the
 * intersection of the Equator and +90 degrees longitude, and +Z is through the North Pole. This Actor is used
 * by other Cesium Actors to control how this coordinate system is mapped into an Unreal Engine world and level.
 */
UCLASS()
class CESIUM_API ACesiumGeoreference : public AActor
{
	GENERATED_BODY()

public:
	static ACesiumGeoreference* GetDefaultForActor(AActor* Actor);

	ACesiumGeoreference();

	/**
	 * The placement of this Actor's origin (coordinate 0,0,0) within the tileset. 3D Tiles tilesets often
	 * use Earth-centered, Earth-fixed coordinates, such that the tileset content is in a small bounding
	 * volume 6-7 million meters (the radius of the Earth) away from the coordinate system origin.
	 * This property allows an alternative position, other then the tileset's true origin, to be treated
	 * as the origin for the purpose of this Actor. Using this property will preserve vertex precision
	 * (and thus avoid jittering) much better than setting the Actor's Transform property.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	EOriginPlacement OriginPlacement = EOriginPlacement::BoundingVolumeOrigin;

	/**
	 * The longitude of the custom origin placement in degrees.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginLongitude = 0.0;

	/**
	 * The latitude of the custom origin placement in degrees.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginLatitude = 0.0;

	/**
	 * The height of the custom origin placement in meters above the WGS84 ellipsoid.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginHeight = 0.0;

	/**
	 * If true, the tileset is rotated so that the local up at the center of the tileset's bounding
	 * volume is aligned with the usual Unreal Engine up direction, +Z. This is useful because
	 * 3D Tiles tilesets often use Earth-centered, Earth-fixed coordinates in which the local
	 * up direction depends on where you are on the Earth. If false, the tileset's true rotation
	 * is used.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin || OriginPlacement==EOriginPlacement::BoundingVolumeOrigin"))
	bool AlignTilesetUpWithZ = true;

	/**
	 * If true, the world origin is periodically rebased to keep it near the camera. This is important for maintaining vertex
	 * precision in large worlds. Setting it to false can lead to jiterring artifacts when the camera gets far away from
	 * the origin.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	bool KeepWorldOriginNearCamera = true;

	/**
	 * The maximum distance that the camera may move from the world's OriginLocation before the
	 * world origin is moved closer to the camera.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="KeepWorldOriginNearCamera"))
	double MaximumWorldOriginDistanceFromCamera = 10000.0;

	/**
	 * The camera to use for setting the world origin.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="KeepWorldOriginNearCamera"))
	APlayerCameraManager* WorldOriginCamera;

	// TODO: Allow user to select/configure the ellipsoid.

	/**
	 * Gets the current transformation from the Cesium coordinate system (ECEF / WGS84) to
	 * the Unreal level's local coordinate system, accounting for the properties of this
	 * instance as well as {@see UWorld::OriginLocation}.
	 */
	glm::dmat4x4 GetCurrentCesiumToUnrealLocalTransform() const;

	/**
	 * Gets the transformation from the Cesium coordinate system (ECEF / WGS84) to
	 * the Unreal level's local coordinate system, just like {@see GetCurrentCesiumToUnrealLocalTransform}.
	 * The difference is that this method can be used to get the transformation while a world origin
	 * rebase is in progress by accounting for an origin offset that has not yet been applied to
	 * {@see UWorld::OriginLocation}.
	 */
	glm::dmat4x4 GetNextCesiumToUnrealLocalTransform(const FIntVector& WorldOriginOffset) const;

	/**
	 * Gets a 4x4 matrix to transform coordinates from the Unreal Engine absolute world coordinates (i.e. the world
	 * coordinates accounting for the {@see UWorld::OriginLocation}) to the Cesium ellipsoid-centered coordinate system.
	 */
	glm::dmat4x4 GetAbsoluteUnrealWorldToEllipsoidCenteredTransform() const;

	/**
	 * Gets a 4x4 matrix to transform coordinates from the Cesium ellipsoid-centered coordinate system to the Unreal Engine
	 * absolute world coordinates (i.e. the world coordinates accounting for the {@see UWorld::OriginLocation}.
	 */
	glm::dmat4x4 GetEllipsoidCenteredToAbsoluteUnrealWorldTransform() const;

	void AddGeoreferencedObject(ICesiumGeoreferenceable* Object);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void UpdateGeoreference();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	TArray<TWeakInterfacePtr<ICesiumGeoreferenceable>> _georeferencedObjects;
};                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                

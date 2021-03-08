// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Containers/UnrealString.h"
#include <glm/mat3x3.hpp>
#include "UObject/WeakInterfacePtr.h"
#include "CesiumGeoreference.generated.h"

class ICesiumGeoreferenceable;

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

/*
 * Sublevels can be georeferenced to the globe by filling out this struct.
 */ 
USTRUCT()
struct FCesiumSubLevel {
	GENERATED_BODY()

	/**
	 * The plain name of the sub level, without any prefixes.
	 */
	UPROPERTY(EditAnywhere)
	FString LevelName;

	/**
	 * The longitude of where on the WGS84 globe this level should sit.
	 */
	UPROPERTY(EditAnywhere)
	double LevelLongitude = 0.0;
	
	/**
	 * The latitude of where on the WGS84 globe this level should sit.
	 */
	UPROPERTY(EditAnywhere)
	double LevelLatitude = 0.0;

	/**
	 * The height in meters above the WGS84 globe this level should sit.
	 */
	UPROPERTY(EditAnywhere)
	double LevelHeight = 0.0;

	/**
	 * How far in meters from the sublevel local origin the camera needs to be to load the level.
	 */
	UPROPERTY(EditAnywhere)
	double LoadRadius = 0.0;

	/**
	 * Whether or not this level is currently loaded. Not relevant in the editor.
	 */
	bool CurrentlyLoaded = false;
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
class CESIUMRUNTIME_API ACesiumGeoreference : public AActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static ACesiumGeoreference* GetDefaultForActor(AActor* Actor);

	ACesiumGeoreference();

	/*
	 * Whether to continue origin rebasing once inside a sublevel. If actors inside the sublevels react 
	 * poorly to origin rebasing, it might be worth turning this option off.
	 */
	UPROPERTY(EditAnywhere, Category="CesiumSublevels", meta=(EditCondition="KeepWorldOriginNearCamera"))
	bool OriginRebaseInsideSublevels = true;

	/*
	 * Rescan for sublevels that have not been georeferenced yet. New levels are placed at the Unreal origin and
	 * georeferenced automatically.
	 */
	UFUNCTION(CallInEditor, Category="CesiumSublevels")
	void CheckForNewSubLevels();

	/*
	 * Jump to the level specified by "Current Level Index".
	 * 
	 * Warning: Before clicking, ensure that all non-Cesium objects in the persistent level are georeferenced with the "CesiumGeoreferenceComponent" or
	 * attached to a "CesiumGlobeAnchorParent". Ensure that static actors only exist in georeferenced sublevels.
	 */
	UFUNCTION(CallInEditor, Category="CesiumSublevels")
	void JumpToCurrentLevel();

	/*
	 * The index of the level the georeference origin should be set to. This aligns the globe with the specified level so that it can be worked on in the editor.
	 * 
	 * Warning: Before changing, ensure the last level you worked on has been properly georeferenced. Ensure all actors are georeferenced, either by inclusion in
	 * a georeferenced sublevel, by adding the "CesiumGeoreferenceComponent", or by attaching to a "CesiumGlobeAnchorParent".
	 */
	UPROPERTY(EditAnywhere, Category="CesiumSublevels", meta=(ArrayClamp="CesiumSubLevels"))
	int CurrentLevelIndex;

	/*
	 * The list of georeferenced sublevels. Each of these has a corresponding world location that can be jumped to. Only one level can be worked on in the editor at a time.
	 */
	UPROPERTY(EditAnywhere, Category="CesiumSublevels")
	TArray<FCesiumSubLevel> CesiumSubLevels;

	/**
	 * The placement of this Actor's origin (coordinate 0,0,0) within the tileset. 
	 *
	 * 3D Tiles tilesets often use Earth-centered, Earth-fixed coordinates, such that the tileset 
	 * content is in a small bounding volume 6-7 million meters (the radius of the Earth) away from 
	 * the coordinate system origin. This property allows an alternative position, other then the 
	 * tileset's true origin, to be treated as the origin for the purpose of this Actor. Using this 
	 * property will preserve vertex precision (and thus avoid jittering) much better than setting 
	 * the Actor's Transform property.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	EOriginPlacement OriginPlacement = EOriginPlacement::CartographicOrigin;

	/**
	 * The longitude of the custom origin placement in degrees.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginLongitude = -105.25737;

	/**
	 * The latitude of the custom origin placement in degrees.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginLatitude = 39.736401;

	/**
	 * The height of the custom origin placement in meters above the WGS84 ellipsoid.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium", meta=(EditCondition="OriginPlacement==EOriginPlacement::CartographicOrigin"))
	double OriginHeight = 2250.0;

	/**
	 * EXPERIMENTAL TODO: figure out if meta tag is needed
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	bool EditOriginInViewport = false;

	/**
	 * If true, the world origin is periodically rebased to keep it near the camera. 
	 *
	 * This is important for maintaining vertex precision in large worlds. Setting it 
	 * to false can lead to jiterring artifacts when the camera gets far away from
	 * the origin.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	bool KeepWorldOriginNearCamera = true;

	/**
	 * Places the georeference origin at the camera's current location. Rotates the globe so the current longitude/latitude/height
	 * of the camera is at the Unreal origin. The camera is also teleported to the Unreal origin.
	 * 
	 * Warning: Before clicking, ensure that all non-Cesium objects in the persistent level are georeferenced with the "CesiumGeoreferenceComponent" or
	 * attached to a "CesiumGlobeAnchorParent". Ensure that static actors only exist in georeferenced sublevels.
	 */ 
	UFUNCTION(CallInEditor, Category="Cesium")
	void PlaceGeoreferenceOriginHere();

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
	 * This aligns the specified global coordinates to Unreal's world origin, i.e. it rotates the globe so that these coordinates exactly fall on the origin.
	 */
	UFUNCTION()
	void SetGeoreferenceOrigin(double targetLongitude, double targetLatitude, double targetHeight);

	/**
	 * This aligns the specified global coordinates to Unreal's world origin, i.e. it rotates the globe so that these coordinates exactly fall on the origin.
	 */
	UFUNCTION(BlueprintCallable)
	void InaccurateSetGeoreferenceOrigin(float targetLongitude, float targetLatitude, float targetHeight);

	// TODO: add utility conversion functions between relative / absolute -> ecef -> long/lat/height

	/**
	 * @brief Gets the transformation from the "Georeferenced" reference frame defined by this instance to the "Ellipsoid-centered" reference frame (i.e. ECEF).
	 * 
	 * Gets a matrix that transforms coordinates from the "Georeference" reference frame defined by this instance to the "Ellipsoid-centered"
	 * reference frame, which is usually Earth-centered, Earth-fixed. See {@link reference-frames.md}.
	 */
	const glm::dmat4& GetGeoreferencedToEllipsoidCenteredTransform() const {
		return this->_georeferencedToEcef;
	}

	/**
	 * @brief Gets the transformation from the "Ellipsoid-centered" reference frame (i.e. ECEF) to the georeferenced reference frame defined by this instance.
	 *
	 * Gets a matrix that transforms coordinates from the "Ellipsoid-centered" reference frame (which is usually Earth-centered, Earth-fixed) to
	 * the "Georeferenced" reference frame defined by this instance. See {@link reference-frames.md}.
	 */
	const glm::dmat4& GetEllipsoidCenteredToGeoreferencedTransform() const {
		return this->_ecefToGeoreferenced;
	}

	/**
	 * @brief Gets the transformation from the "Unreal World" reference frame to the "Ellipsoid-centered" reference frame (i.e. ECEF).
	 * 
	 * Gets a matrix that transforms coordinates from the "Unreal World" reference frame (with respect to the absolute world origin, not the floating origin) to 
	 * the "Ellipsoid-centered" reference frame (which is usually Earth-centered, Earth-fixed). See {@link reference-frames.md}.
	 */
	const glm::dmat4& GetUnrealWorldToEllipsoidCenteredTransform() const {
		return this->_ueToEcef;
	}

	/**
	 * @brief Gets the transformation from the "Ellipsoid-centered" reference frame (i.e. ECEF) to the "Unreal World" reference frame.
	 *
	 * Gets a matrix that transforms coordinates from the "Ellipsoid-centered" reference frame (which is usually Earth-centered, Earth-fixed) to
	 * the "Unreal world" reference frame (with respect to the absolute world origin, not the floating origin). See {@link reference-frames.md}.
	 */
	const glm::dmat4& GetEllipsoidCenteredToUnrealWorldTransform() const {
		return this->_ecefToUe;
	}

	void AddGeoreferencedObject(ICesiumGeoreferenceable* Object);

	void UpdateGeoreference();

protected:
	

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	// Called every frame
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void Tick(float DeltaTime) override;

private:
	glm::dmat4 _georeferencedToEcef;
	glm::dmat4 _ecefToGeoreferenced;
	glm::dmat4 _ueToEcef;
	glm::dmat4 _ecefToUe;

	void _jumpToLevel(const FCesiumSubLevel& level);
#if WITH_EDITOR
	void _lineTraceViewportMouse(const bool ShowTrace, bool& Success, FHitResult& HitResult);
#endif
	TArray<TWeakInterfacePtr<ICesiumGeoreferenceable>> _georeferencedObjects;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CesiumGeoreference.h"
#include "CesiumGeoreferenceable.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Cesium3DTiles/BoundingVolume.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <optional>

#include "CesiumGeoreferenceComponent.generated.h"

UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUM_API UCesiumGeoreferenceComponent : 
	public USceneComponent,
	public ICesiumGeoreferenceable
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCesiumGeoreferenceComponent();

	// TODO: Probably should use custom details builder to make the UI a little more friendly,
	// it would be nice to have the same options reflected in the actor's details panel as well
	// if that's even possible.

	/**
	 * The georeference actor controlling how the owning actor's coordinate system relates to the 
	 * coordinate system in this Unreal Engine level.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	ACesiumGeoreference* Georeference;

	/**
	 * The current longitude of this actor.
	 */ 
	UPROPERTY(VisibleAnywhere, Category="Cesium")
	double CurrentLongitude = 0.0;

	/**
	 * The current latitude of this actor.
	 */ 
	UPROPERTY(VisibleAnywhere, Category="Cesium")
	double CurrentLatitude = 0.0;

	/**
	 * The current height in meters (above the WGS84 ellipsoid) of this actor.
	 */ 
	UPROPERTY(VisibleAnywhere, Category="Cesium")
	double CurrentHeight = 0.0;

	/**
	 * Aligns the local up direction with the ellipsoid normal at the current location. 
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Cesium|Orientation")
	void SnapLocalUpToEllipsoidNormal();

	/**
	 * Aligns the local X, Y, Z axes to West, North, and Up (the ellipsoid normal) respectively.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Cesium|Orientation")
	void SnapToEastSouthUpTangentPlane(); 

	/**
	 * The longitude to move this actor to.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Teleport|Longitude Latitude Height")
	double TargetLongitude = 0.0;
	
	/**
	 * The latitude to move this actor to.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Teleport|Longitude Latitude Height")
	double TargetLatitude = 0.0;
	
	/**
	 * The height to move this actor to (in meters above the WGS84 ellipsoid).
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Teleport|Longitude Latitude Height")
	double TargetHeight = 0.0;

	/**
	 * The Earth-Centered Earth-Fixed X-coordinate to move this actor to.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Teleport|Earth-Centered, Earth-Fixed")
	double TargetECEF_X = 0.0;
	
	/**
	 * The Earth-Centered Earth-Fixed Y-coordinate to move this actor to.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Teleport|Earth-Centered, Earth-Fixed")
	double TargetECEF_Y = 0.0;
	
	/**
	 * The Earth-Centered Earth-Fixed Z-coordinate to move this actor to.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium|Teleport|Earth-Centered, Earth-Fixed")
	double TargetECEF_Z = 0.0;

	/**
	 * Move the actor to the specified longitude/latitude/height.
	 */
	void MoveToLongLatHeight(double longitude, double latitude, double height);

	/**
	 * Move the actor to the specified longitude/latitude/height. Inaccurate since this takes single-precision floats.
	 */ 
	UFUNCTION(BlueprintCallable)
	void InaccurateMoveToLongLatHeight(float longitude, float latitude, float height);

	/**
	 * Move the actor to the specified ECEF coordinates.
	 */ 
	void MoveToECEF(double ecef_x, double ecef_y, double ecef_z);

	/**
	 * Move the actor to the specified ECEF coordinates. Inaccurate since this takes single-precision floats.
	 */ 
	UFUNCTION(BlueprintCallable)
	void InaccurateMoveToECEF(float ecef_x, float ecef_y, float ecef_z);

	virtual void OnRegister() override;

	/**
	 * Delegate implementation to recieve a notification when the owner's root component has changed.
	 */ 
	UFUNCTION()
	void OnRootComponentChanged(USceneComponent* newRoot, bool idk);

	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	// ICesiumGeoreferenceable virtual functions
	virtual bool IsBoundingVolumeReady() const override;
	virtual std::optional<Cesium3DTiles::BoundingVolume> GetBoundingVolume() const override;
	virtual void UpdateGeoreferenceTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform) override;

	void SetAutoSnapToEastSouthUp(bool value);

private:
	void _initRootComponent();
	void _initWorldOriginLocation();
	void _updateAbsoluteLocation();
	void _updateRelativeLocation();
	void _initGeoreference();
	void _updateActorToECEF();
	void _updateActorToUnrealRelativeWorldTransform();
	void _updateActorToUnrealRelativeWorldTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform);
	void _setTransform(const glm::dmat4& transform);
	void _updateLongLatHeight();

	glm::dvec3 _worldOriginLocation;
	glm::dvec3 _absoluteLocation;
	glm::dvec3 _relativeLocation;
	glm::dmat4 _actorToECEF;
	glm::dmat4 _actorToUnrealRelativeWorld;

	USceneComponent* _ownerRoot;

	bool _ignoreOnUpdateTransform;
	bool _autoSnapToEastSouthUp;
};

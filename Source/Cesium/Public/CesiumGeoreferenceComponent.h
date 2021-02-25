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
	 * The longitude of this actor.
	 */ 
	UPROPERTY(EditAnywhere, Category="Cesium")
	double Longitude = 0.0;

	/**
	 * The latitude of this actor.
	 */ 
	UPROPERTY(EditAnywhere, Category="Cesium")
	double Latitude = 0.0;

	/**
	 * The height in meters (above the WGS84 ellipsoid) of this actor.
	 */ 
	UPROPERTY(EditAnywhere, Category="Cesium")
	double Altitude = 0.0;

	/**
	 * The Earth-Centered Earth-Fixed X-coordinate of this actor.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	double ECEF_X = 0.0;
	
	/**
	 * The Earth-Centered Earth-Fixed Y-coordinate of this actor.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	double ECEF_Y = 0.0;
	
	/**
	 * The Earth-Centered Earth-Fixed Z-coordinate of this actor.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	double ECEF_Z = 0.0;

	/**
	 * Aligns the local up direction with the ellipsoid normal at the current location. 
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Cesium")
	void SnapLocalUpToEllipsoidNormal();

	/**
	 * Turns the actor's local coordinate system into a East-South-Up tangent space in centimeters.
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Cesium")
	void SnapToEastSouthUp(); 

	/**
	 * Move the actor to the specified longitude/latitude/height.
	 */
	void MoveToLongLatHeight(double targetLongitude, double targetLatitude, double targetAltitude);

	/**
	 * Move the actor to the specified longitude/latitude/height. Inaccurate since this takes single-precision floats.
	 */ 
	UFUNCTION(BlueprintCallable)
	void InaccurateMoveToLongLatHeight(float targetLongitude, float targetLatitude, float targetAltitude);

	/**
	 * Move the actor to the specified ECEF coordinates.
	 */ 
	void MoveToECEF(double targetEcef_x, double targetEcef_y, double targetEcef_z);

	/**
	 * Move the actor to the specified ECEF coordinates. Inaccurate since this takes single-precision floats.
	 */ 
	UFUNCTION(BlueprintCallable)
	void InaccurateMoveToECEF(float targetEcef_x, float targetEcef_y, float targetEcef_z);

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

	bool CheckCoordinatesChanged() const {
		return this->_dirty;
	}

	void MarkCoordinatesUnchanged() {
		this->_dirty = false;
	}

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
	void _updateGeospatialCoordinates();

	glm::dvec3 _worldOriginLocation;
	glm::dvec3 _absoluteLocation;
	glm::dvec3 _relativeLocation;
	glm::dmat4 _actorToECEF;
	glm::dmat4 _actorToUnrealRelativeWorld;

	USceneComponent* _ownerRoot;

	bool _ignoreOnUpdateTransform;
	bool _autoSnapToEastSouthUp;
	bool _dirty;
};

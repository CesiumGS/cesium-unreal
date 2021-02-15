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

	/**
	 * The georeference actor controlling how the owning actor's coordinate system relates to the 
	 * coordinate system in this Unreal Engine level.
	 */
	UPROPERTY(EditAnywhere, Category="Cesium")
	ACesiumGeoreference* Georeference;

	/**
	 * Aligns the local up direction with the ellipsoid normal at the current location. 
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Cesium|Orientation")
	void SnapLocalUpToEllipsoidNormal();

    /**
     * The longitude to move this actor to.
     */
    UPROPERTY(EditAnywhere, Category="Cesium|LongLatHeight")
    double Longitude = 0.0;
    
    /**
     * The latitude to move this actor to.
     */
    UPROPERTY(EditAnywhere, Category="Cesium|LongLatHeight")
    double Latitude = 0.0;
    
    /**
     * The height to move this actor to (in meters above the WGS84 ellipsoid).
     */
    UPROPERTY(EditAnywhere, Category="Cesium|LongLatHeight")
    double Height = 0.0;

    /**
     * Move the actor to the above longitude/latitude/height.
     */
    UFUNCTION(BlueprintCallable, CallinEditor, Category="Cesium|LongLatHeight")
    void MoveToLongLatHeight();

    // TODO: can we make these accuracy warnings more clear? it would be cool if there was a way to write a warning note in the details panel above these.

    /**
     * The Earth-Centered Earth-Fixed X-coordinate to move this actor to. Note that this will be cast down to a float regardless of the inputted precision.
     * Use SetAccurateECEF from C++ code if double precision is needed.
     */
    UPROPERTY(EditAnywhere, Category="Cesium|ECEF")
    double ECEF_X = 0.0;
    
    /**
     * The Earth-Centered Earth-Fixed Y-coordinate to move this actor to. Note that this will be cast down to a float regardless of the inputted precision.
     * Use SetAccurateECEF from C++ code if double precision is needed.
     */
    UPROPERTY(EditAnywhere, Category="Cesium|ECEF")
    double ECEF_Y = 0.0;
    
    /**
     * The Earth-Centered Earth-Fixed Z-coordinate to move this actor to. Note that this will be cast down to a float regardless of the inputted precision.
     * Use SetAccurateECEF from C++ code if double precision is needed.
     */
    UPROPERTY(EditAnywhere, Category="Cesium|ECEF")
    double ECEF_Z = 0.0;

    /**
     * Move the actor to the above Earth-Centered Earth-Fixed coordinate. Note that since Unreal currently does not support the double type, this might be slightly inaccurate.
     * Use SetAccurateECEF from C++ code if double precision is needed.
     */
    UFUNCTION(BlueprintCallable, CallinEditor, Category="Cesium|ECEF")
    void MoveToECEF();

    /**
     * Set the position of the actor in ECEF coordinates.
     */ 
    void SetAccurateECEF(double x, double y, double z);

	virtual void OnRegister() override;

	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	virtual void Activate(bool bReset) override;
	virtual void Deactivate() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	// ICesiumGeoreferenceable virtual functions
	virtual bool IsBoundingVolumeReady() const override;
	virtual std::optional<Cesium3DTiles::BoundingVolume> GetBoundingVolume() const override;
	virtual void UpdateGeoreferenceTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform) override;

private:
	void _updateAbsoluteLocation();
	void _updateRelativeLocation();
	void _initGeoreference();
	void _updateActorToECEF();
	void _updateActorToUnrealRelativeWorldTransform();
	void _updateActorToUnrealRelativeWorldTransform(const glm::dmat4& ellipsoidCenteredToGeoreferencedTransform);
	void _setTransform(const glm::dmat4& transform);

	glm::dvec3 _worldOriginLocation;
	glm::dvec3 _absoluteLocation;
	glm::dvec3 _relativeLocation;
	glm::dmat4 _actorToECEF;
	glm::dmat4 _actorToUnrealRelativeWorld;

	USceneComponent* _ownerRoot;

	bool _ignoreOnUpdateTransform;
};

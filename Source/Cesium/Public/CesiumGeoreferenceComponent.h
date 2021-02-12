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

    /**
     * The georeference actor controlling how the owning actor's coordinate system relates to the 
     * coordinate system in this Unreal Engine level.
     */
    UPROPERTY(EditAnywhere)
    ACesiumGeoreference* Georeference;

    /**
     * Aligns the local up direction with the ellipsoid normal at the current location. 
     */
    UFUNCTION(BlueprintCallable, CallInEditor)
    void SnapLocalUpToEllipsoidNormal();

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

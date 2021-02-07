// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CesiumGeoreference.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Cesium3DTiles/BoundingVolume.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <optional>

#include "CesiumGeoreferenceComponent.generated.h"

UCLASS()
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
    UPROPERTY(EditAnywhere, Category="Cesium")
    ACesiumGeoreference* Georeference;

    virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;
    virtual bool MoveComponentImpl(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit, EMoveComponentFlags MoveFlags, ETeleportType Teleport) override;

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
    void _setGeoreference();
    void _updateAbsoluteLocation()

    glm::dvec3 _worldOriginLocation;
	glm::dvec3 _absoluteLocation;
	glm::dmat4 _actorToUnrealRelativeWorld;
	bool _isDirty;
};

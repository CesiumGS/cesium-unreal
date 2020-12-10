// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include <glm/vec3.hpp>
#include "Cesium3DTilesetRoot.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UCesium3DTilesetRoot : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCesium3DTilesetRoot();

	void BeginOriginRebase();
	void EndOriginRebase();

	const glm::dvec3& GetAbsoluteLocation() const { return this->_absoluteLocation; }
	bool IsDirty() const { return this->_isDirty; }
	void MarkClean();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual bool MoveComponentImpl(const FVector& Delta, const FQuat& NewRotation, bool bSweep, FHitResult* OutHit = NULL, EMoveComponentFlags MoveFlags = MOVECOMP_NoFlags, ETeleportType Teleport = ETeleportType::None) override;

private:
	bool _originIsRebasing;
	glm::dvec3 _absoluteLocation;
	bool _isDirty;
};

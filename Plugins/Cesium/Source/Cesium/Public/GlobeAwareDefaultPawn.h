// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "CesiumGeoreference.h"
#include <glm/mat3x3.hpp>
#include "GlobeAwareDefaultPawn.generated.h"

/**
 * 
 */
UCLASS()
class CESIUM_API AGlobeAwareDefaultPawn : public ADefaultPawn
{
	GENERATED_BODY()

	AGlobeAwareDefaultPawn();
	
	UPROPERTY(EditAnywhere, Category = "Cesium")
	ACesiumGeoreference* Georeference;

	/**
		* Input callback to move forward in local space (or backward if Val is negative).
		* @param Val Amount of movement in the forward direction (or backward if negative).
		* @see APawn::AddMovementInput()
		*/
	virtual void MoveForward(float Val) override;

	/**
	 * Input callback to strafe right in local space (or left if Val is negative).
	 * @param Val Amount of movement in the right direction (or left if negative).
	 * @see APawn::AddMovementInput()
	 */
	virtual void MoveRight(float Val) override;

	/**
	 * Input callback to move up in world space (or down if Val is negative).
	 * @param Val Amount of movement in the world up direction (or down if negative).
	 * @see APawn::AddMovementInput()
	 */
	virtual void MoveUp_World(float Val) override;

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	virtual void TurnAtRate(float Rate) override;

	/**
	* Called via input to look up at a given rate (or down if Rate is negative).
	* @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	*/
	virtual void LookUpAtRate(float Rate) override;

	virtual void AddControllerPitchInput(float Val) override;
	virtual void AddControllerYawInput(float Val) override;
	virtual void AddControllerRollInput(float Val) override;
	virtual FRotator GetViewRotation() const override;

private:
	/**
	 * Computes the local East-North-Up to Fixed frame transformation based on the current
	 * {@see GetPawnViewLocation}. The returned transformation works in Unreal's left-handed
	 * coordinate system.
	 */
	glm::dmat3 computeEastNorthUpToFixedFrame() const;
};

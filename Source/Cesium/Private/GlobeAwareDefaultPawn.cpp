// Fill out your copyright notice in the Description page of Project Settings.


#include "GlobeAwareDefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumTransforms.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "CesiumUtility/Math.h"

AGlobeAwareDefaultPawn::AGlobeAwareDefaultPawn() :
	ADefaultPawn()
{
}

void AGlobeAwareDefaultPawn::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		if (Controller)
		{
			FRotator const ControlSpaceRot = this->GetViewRotation();

			// transform to world space and add it
			AddMovementInput(FRotationMatrix(ControlSpaceRot).GetScaledAxis(EAxis::Y), Val);
		}
	}
}

void AGlobeAwareDefaultPawn::MoveForward(float Val)
{
	if (Val != 0.f)
	{
		if (Controller)
		{
			FRotator const ControlSpaceRot = this->GetViewRotation();

			// transform to world space and add it
			AddMovementInput(FRotationMatrix(ControlSpaceRot).GetScaledAxis(EAxis::X), Val);
		}
	}
}

void AGlobeAwareDefaultPawn::MoveUp_World(float Val)
{
	if (Val != 0.f)
	{
		glm::dmat3 enuToFixed = this->computeEastNorthUpToFixedFrame();
		FVector up(enuToFixed[2].x, enuToFixed[2].y, enuToFixed[2].z);
		AddMovementInput(up, Val);
	}
}

void AGlobeAwareDefaultPawn::TurnAtRate(float Rate)
{
	ADefaultPawn::TurnAtRate(Rate);
}

void AGlobeAwareDefaultPawn::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds() * CustomTimeDilation);
}

void AGlobeAwareDefaultPawn::AddControllerPitchInput(float Val)
{
	if (Val != 0.f && Controller && Controller->IsLocalPlayerController())
	{
		APlayerController* const PC = CastChecked<APlayerController>(Controller);
		PC->AddPitchInput(Val);
	}
}

void AGlobeAwareDefaultPawn::AddControllerYawInput(float Val)
{
	ADefaultPawn::AddControllerYawInput(Val);
}

void AGlobeAwareDefaultPawn::AddControllerRollInput(float Val)
{
	if (Val != 0.f && Controller && Controller->IsLocalPlayerController())
	{
		APlayerController* const PC = CastChecked<APlayerController>(Controller);
		PC->AddRollInput(Val);
	}
}

FRotator AGlobeAwareDefaultPawn::GetViewRotation() const
{
	FRotator localRotation = ADefaultPawn::GetViewRotation();

	glm::dmat3 enuToFixedUE = this->computeEastNorthUpToFixedFrame();

	FMatrix enuAdjustmentMatrix(
		FVector(enuToFixedUE[0].x, enuToFixedUE[0].y, enuToFixedUE[0].z),
		FVector(enuToFixedUE[1].x, enuToFixedUE[1].y, enuToFixedUE[1].z),
		FVector(enuToFixedUE[2].x, enuToFixedUE[2].y, enuToFixedUE[2].z),
		FVector(0.0, 0.0, 0.0)
	);

	return FRotator(enuAdjustmentMatrix.ToQuat() * localRotation.Quaternion());
}

FRotator AGlobeAwareDefaultPawn::GetBaseAimRotation() const
{
	return this->GetViewRotation();
}

void AGlobeAwareDefaultPawn::BeginPlay()
{
	Super::BeginPlay();

	if (!this->Georeference) {
		this->Georeference = ACesiumGeoreference::GetDefaultForActor(this);
	}
}

glm::dmat3 AGlobeAwareDefaultPawn::computeEastNorthUpToFixedFrame() const {
	if (!this->Georeference) {
		return glm::dmat3(1.0);
	}

	FVector ueLocation = this->GetPawnViewLocation();
	FIntVector ueOrigin = this->GetWorld()->OriginLocation;
	glm::dvec3 location = glm::dvec3(
		static_cast<double>(ueLocation.X) + static_cast<double>(ueOrigin.X),
		-(static_cast<double>(ueLocation.Y) + static_cast<double>(ueOrigin.Y)),
		static_cast<double>(ueLocation.Z) + static_cast<double>(ueOrigin.Z)
	) / 100.0;

	glm::dmat4 unrealToEcef = this->Georeference->GetAbsoluteUnrealWorldToEllipsoidCenteredTransform();
	glm::dvec3 cameraEcef = unrealToEcef * glm::dvec4(location, 1.0);
	glm::dmat4 enuToEcefAtCamera = CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(cameraEcef);
	glm::dmat4 ecefToUnreal = glm::affineInverse(unrealToEcef);

	// Camera Axes = ENU
	// Unreal Axes = controlled by Georeference
	glm::dmat3 rotationCesium =
		glm::dmat3(ecefToUnreal) *
		glm::dmat3(enuToEcefAtCamera);

	glm::dmat3 cameraToUnreal =
		glm::dmat3(CesiumTransforms::unrealToOrFromCesium) *
		rotationCesium *
		glm::dmat3(CesiumTransforms::unrealToOrFromCesium);

	return cameraToUnreal;
}

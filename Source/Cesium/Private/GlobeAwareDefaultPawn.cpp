// Fill out your copyright notice in the Description page of Project Settings.


#include "GlobeAwareDefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumTransforms.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/ext/matrix_transform.hpp>

#undef PI
#include "CesiumUtility/Math.h"

void AGlobeAwareDefaultPawn::MoveRight(float Val)
{
	if (Val != 0.f)
	{
		if (Controller)
		{
			FRotator const ControlSpaceRot = Controller->GetControlRotation();

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
			FRotator const ControlSpaceRot = Controller->GetControlRotation();

			// transform to world space and add it
			AddMovementInput(FRotationMatrix(ControlSpaceRot).GetScaledAxis(EAxis::X), Val);
		}
	}
}

void AGlobeAwareDefaultPawn::MoveUp_World(float Val)
{
	if (Val != 0.f)
	{
		AddMovementInput(FVector::UpVector, Val);
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
	//if (Val != 0.f && Controller && Controller->IsLocalPlayerController())
	//{
	//	APlayerController* const PC = CastChecked<APlayerController>(Controller);

	//	Val *= PC->InputYawScale;

	//	// In Unreal, "yaw" means rotation about the Z-axis (up axis).
	//	// On a globe, Z in world coordinates is usually not up. So do a rotation
	//	// about the globe's local up axis instead.
	//	// TODO: don't assume the center of the Earth is at (0,0,0). But how?
	//	FVector ueLocation = this->GetActorLocation();
	//	FIntVector ueOrigin = this->GetWorld()->OriginLocation;
	//	glm::dvec3 location = glm::dvec3(
	//		static_cast<double>(ueLocation.X) + static_cast<double>(ueOrigin.X),
	//		static_cast<double>(-ueLocation.Y) + static_cast<double>(-ueOrigin.Y),
	//		static_cast<double>(ueLocation.Z) + static_cast<double>(ueOrigin.Z)
	//	) / 100.0;

	//	//location = CesiumGeospatial::Ellipsoid::WGS84.scaleToGeodeticSurface(location).value();
	//	//glm::dmat3 enuToFixed = CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(location);
	//	//glm::dmat3 fixedToEnu = glm::inverse(enuToFixed);
	//	glm::dvec3 up = CesiumGeospatial::Ellipsoid::WGS84.geodeticSurfaceNormal(location);
	//	FVector upInCameraFrame = this->GetActorTransform().InverseTransformVector(FVector(up.x, -up.y, up.z));
	//	FQuat quaternion(upInCameraFrame, Val * 3.14159 / 180.0);
	//	FRotator increment(quaternion);
	//	//glm::dmat3 transform = glm::rotate(glm::dmat4(1.0), static_cast<double>(Val), up);
	//	//glm::dmat3 transform = enuToFixed * rotation * fixedToEnu;

	//	//FRotator increment = FMatrix(
	//	//	FVector(transform[0].x, transform[1].x, transform[2].x),
	//	//	FVector(transform[0].y, transform[1].y, transform[2].y),
	//	//	FVector(transform[0].z, transform[1].z, transform[2].z),

	//	//	//FVector(transform[0].x, transform[0].y, transform[0].z),
	//	//	//FVector(transform[1].x, transform[1].y, transform[1].z),
	//	//	//FVector(transform[2].x, transform[2].y, transform[2].z),

	//	//	FVector(0.0, 0.0, 0.0)
	//	//).Rotator();

	//	//increment.Pitch = -increment.Pitch;

	//	//PC->RotationInput.Yaw += !PC->IsLookInputIgnored() ? Val : 0.f;
	//	PC->RotationInput += increment;
	//	//		PC->AddYawInput(Val);
	//}
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

	FVector ueLocation = this->GetPawnViewLocation();
	FIntVector ueOrigin = this->GetWorld()->OriginLocation;
	glm::dvec3 location = glm::dvec3(
		static_cast<double>(ueLocation.X) + static_cast<double>(ueOrigin.X),
		-(static_cast<double>(ueLocation.Y) + static_cast<double>(ueOrigin.Y)),
		static_cast<double>(ueLocation.Z) + static_cast<double>(ueOrigin.Z)
	) / 100.0;

	glm::dmat3 enuToFixed = CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(location);
	glm::dmat3 enuToFixedUE = glm::dmat3(CesiumTransforms::unrealToOrFromCesium) * enuToFixed * glm::dmat3(CesiumTransforms::unrealToOrFromCesium);

	FMatrix enuAdjustmentMatrix(
		FVector(enuToFixedUE[0].x, enuToFixedUE[0].y, enuToFixedUE[0].z),
		FVector(enuToFixedUE[1].x, enuToFixedUE[1].y, enuToFixedUE[1].z),
		FVector(enuToFixedUE[2].x, enuToFixedUE[2].y, enuToFixedUE[2].z),
		FVector(0.0, 0.0, 0.0)
	);

	return FRotator(enuAdjustmentMatrix.ToQuat() * localRotation.Quaternion());
}

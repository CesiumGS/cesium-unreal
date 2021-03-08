// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "GlobeAwareDefaultPawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "CesiumGeoreference.h"
#include "CesiumGeospatial/Transforms.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumTransforms.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "CesiumUtility/Math.h"

//
#include "DrawDebugHelpers.h"
#include <glm/ext/vector_double3.hpp>

AGlobeAwareDefaultPawn::AGlobeAwareDefaultPawn() :
	ADefaultPawn()
{
	PrimaryActorTick.bCanEverTick = true;
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


//////////////////////////////////////////////////////////////////////////
// Useful transformations
//////////////////////////////////////////////////////////////////////////
FVector AGlobeAwareDefaultPawn::InaccurateTransformECEFToUE(FVector& point)
{
	glm::dvec3 cameraUnreal = ecefToUnreal * glm::dvec4(point.X, point.Y, point.Z, 1.0);

	return FVector(cameraUnreal.x, cameraUnreal.y, cameraUnreal.z) - FVector(this->GetWorld()->OriginLocation);
}

void AGlobeAwareDefaultPawn::AccurateTransformECEFToUE(double X, double Y, double Z, double& ResultX, double& ResultY, double& ResultZ)
{
	FIntVector ueOrigin = this->GetWorld()->OriginLocation;
	
	glm::dvec3 cameraUnreal = ecefToUnreal * glm::dvec4(X, Y, Z, 1.0);
	glm::dvec3 location = glm::dvec3(cameraUnreal.x, cameraUnreal.y, cameraUnreal.z) - 
							glm::dvec3(static_cast<double>(ueOrigin.X), static_cast<double>(ueOrigin.Y), static_cast<double>(ueOrigin.Z));

	ResultX = location.x; 
	ResultY = location.y;
	ResultZ = location.z;
}

void AGlobeAwareDefaultPawn::AccurateTransformUEToECEF(double X, double Y, double Z, double& ResultX, double& ResultY, double& ResultZ)
{
	FIntVector ueOrigin = this->GetWorld()->OriginLocation;
	glm::dvec3 location = glm::dvec3(
		X + static_cast<double>(ueOrigin.X),
		Y + static_cast<double>(ueOrigin.Y), // ? TBC
		Z + static_cast<double>(ueOrigin.Z)
	);
	glm::dmat4 unrealToEcef = this->Georeference->GetUnrealWorldToEllipsoidCenteredTransform();
	glm::dvec3 locationEcef = unrealToEcef * glm::dvec4(location, 1.0);

	ResultX = locationEcef.x;
	ResultY = locationEcef.y;
	ResultZ = locationEcef.z;
}

FRotator AGlobeAwareDefaultPawn::TransformRotatorUEToENU(FRotator UERotator)
{
	glm::dmat3 enuToFixedUE = this->computeEastNorthUpToFixedFrame();

	FMatrix enuAdjustmentMatrix(
		FVector(enuToFixedUE[0].x, enuToFixedUE[0].y, enuToFixedUE[0].z),
		FVector(enuToFixedUE[1].x, enuToFixedUE[1].y, enuToFixedUE[1].z),
		FVector(enuToFixedUE[2].x, enuToFixedUE[2].y, enuToFixedUE[2].z),
		FVector(0.0, 0.0, 0.0)
	);

	return FRotator(enuAdjustmentMatrix.ToQuat() * UERotator.Quaternion());
}

FRotator AGlobeAwareDefaultPawn::TransformRotatorENUToUE(FRotator ENURotator)
{
	glm::dmat3 enuToFixedUE = this->computeEastNorthUpToFixedFrame();
	FMatrix enuAdjustmentMatrix(
		FVector(enuToFixedUE[0].x, enuToFixedUE[0].y, enuToFixedUE[0].z),
		FVector(enuToFixedUE[1].x, enuToFixedUE[1].y, enuToFixedUE[1].z),
		FVector(enuToFixedUE[2].x, enuToFixedUE[2].y, enuToFixedUE[2].z),
		FVector(0.0, 0.0, 0.0)
	);

	FMatrix inverse = enuAdjustmentMatrix.InverseFast();

	return FRotator(inverse.ToQuat() * ENURotator.Quaternion());
}

void AGlobeAwareDefaultPawn::GetECEFCameraLocation(double& X, double& Y, double& Z)
{
	FVector ueLocation = this->GetPawnViewLocation();
	double ecefX, ecefY, ecefZ;
	AccurateTransformUEToECEF(ueLocation.X, ueLocation.Y, ueLocation.Z, ecefX, ecefY, ecefZ);

	X = ecefX;
	Y = ecefY;
	Z = ecefZ;
}

void AGlobeAwareDefaultPawn::SetECEFCameraLocation(double X, double Y, double Z)
{
	double ueX, ueY, ueZ;
	AccurateTransformECEFToUE(X, Y, Z, ueX, ueY, ueZ);
	ADefaultPawn::SetActorLocation(FVector(static_cast<float>(ueX), static_cast<float>(ueY), static_cast<float>(ueZ)));
}

void AGlobeAwareDefaultPawn::FlyToLocation(double ECEFDestinationX, double ECEFDestinationY, double ECEFDestinationZ, float YawAtDestination, float PitchAtDestination)
{
	// We will work in ECEF space, but using Unreal Classes to benefit from Math tools. 
	// Actual precision might suffer, but this is for a cosmetic flight...
	
	// Compute source and destination locations in ECEF
	double sourceLocationX, sourceLocationY, sourceLocationZ;
	GetECEFCameraLocation(sourceLocationX, sourceLocationY, sourceLocationZ);
	FVector sourceECEFLocation = FVector(sourceLocationX, sourceLocationY, sourceLocationZ);
	FVector destinationECEFLocation = FVector(ECEFDestinationX, ECEFDestinationY, ECEFDestinationZ);

	// Compute the source and destination rotations in ENU
	// As during the flight, we can go around the globe, this is better to interpolate in ENU coordinates
	flyToSourceRotation = ADefaultPawn::GetViewRotation();
	flyToDestinationRotation = FRotator(PitchAtDestination, YawAtDestination, 0);

	// Compute axis/Angle transform and initialize key points 
	FQuat flyQuat = FQuat::FindBetweenVectors(sourceECEFLocation, destinationECEFLocation);
	float flyTotalAngle;
	FVector flyRotationAxis;
	flyQuat.ToAxisAndAngle(flyRotationAxis, flyTotalAngle);
	int steps = FMath::Max(int(flyTotalAngle / FMath::DegreesToRadians(FlyToGranulatiryDegrees)) - 1, 0);
	Keypoints.Empty(steps + 2);

	// We will not create a curve projected along the ellipsoid as we want to take altitude while flying. 
	// The radius of the current point will evolve as follow
	//  - Project the point on the ellipsoid - Will give a default radius depending on ellipsoid location. 
	//  - Interpolate the altitudes : get source/destination altitude, and make a linear interpolation between them. This will allow for flying from/to any point smoothly. 
	//  - Add as flightProfile offset /-\ defined by a curve. 

	// Compute global radius at source and destination points
	FVector sourceUpVector, destinationUpVector;
	float sourceRadius, destinationRadius;
	sourceECEFLocation.ToDirectionAndLength(sourceUpVector, sourceRadius);
	destinationECEFLocation.ToDirectionAndLength(destinationUpVector, destinationRadius);

	// Compute actual altitude at source and destination points by scaling on ellipsoid. 
	float sourceAltitude, destinationAltitude = 0;
	const CesiumGeospatial::Ellipsoid& ellipsoid = CesiumGeospatial::Ellipsoid::WGS84;
	if (auto scaled = ellipsoid.scaleToGeodeticSurface(glm::dvec3(sourceECEFLocation.X, sourceECEFLocation.Y, sourceECEFLocation.Z))) {
		sourceAltitude = FVector::Dist(sourceECEFLocation, FVector(scaled->x, scaled->y, scaled->z));
	}
	if (auto scaled = ellipsoid.scaleToGeodeticSurface(glm::dvec3(destinationECEFLocation.X, destinationECEFLocation.Y, destinationECEFLocation.Z))) {
		destinationAltitude = FVector::Dist(destinationECEFLocation, FVector(scaled->x, scaled->y, scaled->z));
	}

	// Get distance between source and destination points to compute a wanted altitude from curve
	float flyTodistance = FVector::Dist(sourceECEFLocation, destinationECEFLocation);

	// Add first keypoint
	Keypoints.Add(sourceECEFLocation);
	// DrawDebugPoint(GetWorld(), InaccurateTransformECEFToUE(sourceECEFLocation), 8, FColor::Red, true, 30);
	for (int step = 1; step <= steps; step++)
	{
		float percentage = (float)step / (steps + 1);
		float altitude = FMath::Lerp<float>(sourceAltitude, destinationAltitude, percentage);
		float phi = FlyToGranulatiryDegrees * static_cast<float>(step);

		FVector rotated = sourceUpVector.RotateAngleAxis(phi, flyRotationAxis);
		if (auto scaled = ellipsoid.scaleToGeodeticSurface(glm::dvec3(rotated.X, rotated.Y, rotated.Z))) 
		{
			FVector projected(scaled->x, scaled->y, scaled->z);
			FVector upVector = projected.GetSafeNormal();
			
			
			// Add an altitude if we have a profile curve for it
			float offsetAltitude = 0;
			if (FlyToAltitudeProfileCurve != NULL)
			{
				float maxAltitude = 30000;
				if (FlyToMaximumAltitudeCurve != NULL)
				{
					maxAltitude = FlyToMaximumAltitudeCurve->GetFloatValue(flyTodistance);
				}
				offsetAltitude = maxAltitude * FlyToAltitudeProfileCurve->GetFloatValue(percentage);
			}
			
			FVector point = projected + upVector * (altitude + offsetAltitude);
			Keypoints.Add(point);
			// DrawDebugPoint(GetWorld(), InaccurateTransformECEFToUE(point), 8, FColor::Red, true, 30);
		}
	}
	Keypoints.Add(destinationECEFLocation);
	// DrawDebugPoint(GetWorld(), InaccurateTransformECEFToUE(destinationECEFLocation), 8, FColor::Red, true, 30);

	// Tell the tick we will be flying from now
	bFlyingToLocation = true;
}

void AGlobeAwareDefaultPawn::Tick(float DeltaSeconds)
{
	RefreshMatricesCache();

	if (bFlyingToLocation)
	{
		currentFlyTime += DeltaSeconds;
		if (currentFlyTime < FlyToDuration)
		{
			float rawPercentage = currentFlyTime / FlyToDuration;

			// In order to accelerate at start and slow down at end, we use a progress profile curve
			float flyPercentage = rawPercentage;
			if (FlyToProgressCurve != NULL)
			{
				flyPercentage = FMath::Clamp<float>(FlyToProgressCurve->GetFloatValue(rawPercentage), 0.0, 1.0);
			}
					
			// Find the keypoint indexes corresponding to the current percentage			
			int lastIndex = FMath::Floor(rawPercentage * (Keypoints.Num() - 1));
			float segmentPercentage = rawPercentage * (Keypoints.Num() - 1) - lastIndex;
			int nextIndex = lastIndex + 1;

			// Get the current position by interpolating between those two points
			FVector lastPosition = Keypoints[lastIndex];
			FVector nextPosition = Keypoints[nextIndex];
			FVector currentPosition = FMath::Lerp<FVector>(lastPosition, nextPosition, segmentPercentage);
			// Set Location
			SetECEFCameraLocation(currentPosition.X, currentPosition.Y, currentPosition.Z);

			// Interpolate rotation - Computation has to be done at each step because the ENU CRS is depending on locatiom
			FRotator currentRotator = FMath::Lerp<FRotator>(TransformRotatorUEToENU(flyToSourceRotation), TransformRotatorUEToENU(flyToDestinationRotation), flyPercentage);
			GetController()->SetControlRotation(TransformRotatorENUToUE(currentRotator));
		}
		else
		{
			// We reached the end - Set actual destination location 
			FVector finalPoint = Keypoints.Last();
			SetECEFCameraLocation(finalPoint.X, finalPoint.Y, finalPoint.Z);
			bFlyingToLocation = false;
			currentFlyTime = 0;
		}
	}
}

void AGlobeAwareDefaultPawn::BeginPlay()
{
	Super::BeginPlay();

	if (!this->Georeference) {
		this->Georeference = ACesiumGeoreference::GetDefaultForActor(this);
	}
}

void AGlobeAwareDefaultPawn::RefreshMatricesCache()
{
	// Optim - Refresh is needed only when CesiumGeoReference actor is changed...
	ecefToUnreal = this->Georeference->GetEllipsoidCenteredToUnrealWorldTransform();
}

glm::dmat3 AGlobeAwareDefaultPawn::computeEastNorthUpToFixedFrame() const {
	if (!this->Georeference) {
		return glm::dmat3(1.0);
	}

	FVector ueLocation = this->GetPawnViewLocation();
	FIntVector ueOrigin = this->GetWorld()->OriginLocation;
	glm::dvec3 location = glm::dvec3(
		static_cast<double>(ueLocation.X) + static_cast<double>(ueOrigin.X),
		static_cast<double>(ueLocation.Y) + static_cast<double>(ueOrigin.Y),
		static_cast<double>(ueLocation.Z) + static_cast<double>(ueOrigin.Z)
	);

	const glm::dmat4& unrealToEcef = this->Georeference->GetUnrealWorldToEllipsoidCenteredTransform();
	glm::dvec3 cameraEcef = unrealToEcef * glm::dvec4(location, 1.0);
	glm::dmat4 enuToEcefAtCamera = CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(cameraEcef);
	const glm::dmat4& ecefToGeoreferenced = this->Georeference->GetEllipsoidCenteredToGeoreferencedTransform();

	// Camera Axes = ENU
	// Unreal Axes = controlled by Georeference
	glm::dmat3 rotationCesium =
		glm::dmat3(ecefToGeoreferenced) *
		glm::dmat3(enuToEcefAtCamera);

	glm::dmat3 cameraToUnreal =
		glm::dmat3(CesiumTransforms::unrealToOrFromCesium) *
		rotationCesium *
		glm::dmat3(CesiumTransforms::unrealToOrFromCesium);

	return cameraToUnreal;
}

// Copyright CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"

#include <glm/mat3x3.hpp>
#include "GlobeAwareDefaultPawn.generated.h"

class ACesiumGeoreference;
class UCurveFloat;

/**
 * 
 */
UCLASS()
class CESIUM_API AGlobeAwareDefaultPawn : public ADefaultPawn
{
	GENERATED_BODY()

	AGlobeAwareDefaultPawn();
	
	/**
	 * The actor controlling how this camera's location in the Cesium world relates to the coordinate system in
	 * this Unreal Engine level.
	 */
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
	virtual FRotator GetBaseAimRotation() const override;

public :

	// Useful transformations

	/**
	 * Transforms a point expressed in ECEF coordinates to UE Coordinates.
	 * WARNING - For debugging purpose only as computations are done in Floating point precision 
	 */
	FVector InaccurateTransformECEFToUE(FVector& Point);

	/** 
	* Transforms accurately (double precision) coordinates from ECEF to UE coordinates, taking into account the current World rebasing origin
	*/
	void AccurateTransformECEFToUE(double X, double Y, double Z, double& ResultX, double& ResultY, double& ResultZ );

	/**
	* Transforms accurately (double precision) coordinates from UE to ECEF coordinates, taking into account the current World rebasing origin
	*/
	void AccurateTransformUEToECEF(double X, double Y, double Z, double& ResultX, double& ResultY, double& ResultZ);

	/**
	* Transforms a rotator expressed in UE coordinates to one expressed in ENU coordinates. 
	(Single precision, but this should not be an issue)
	*/
	FRotator TransformRotatorUEToENU(FRotator UERotator);

	/**
	* Transforms a rotator expressed in ENU coordinates to one expressed in UE coordinates.
	(Single precision, but this should not be an issue)
	*/
	FRotator TransformRotatorENUToUE(FRotator UERotator);

	/** 
	* Get the pawn Camera location accurately in ECEF Coordinates
	*/
	void GetECEFCameraLocation(double& X, double& Y, double& Z);
	/**
	* Set the pawn Camera location accurately from ECEF Coordinates
	*/
	void SetECEFCameraLocation(double X, double Y, double Z);



	UPROPERTY(EditAnywhere, Category = "Cesium")
	UCurveFloat* FlyToAltitudeProfileCurve;
	
	UPROPERTY(EditAnywhere, Category = "Cesium")
	UCurveFloat* FlyToProgressCurve;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	UCurveFloat* FlyToMaximumAltitudeCurve;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	float FlyToDuration = 5;

	UPROPERTY(EditAnywhere, Category = "Cesium")
	float FlyToGranulatiryDegrees = 0.01;

	void FlyToLocation(double ECEFDestinationX, double ECEFDestinationY, double ECEFDestinationZ, float YawAtDestination, float PitchAtDestination);

	

	bool bFlyingToLocation = false;
	float currentFlyTime = 0;
	FRotator flyToSourceRotation;
	FRotator flyToDestinationRotation;

	virtual void Tick(float DeltaSeconds) override;
	TArray<FVector> Keypoints;

protected:
	virtual void BeginPlay() override;

	virtual void RefreshMatricesCache();

private:
	/**
	 * Computes the local East-North-Up to Fixed frame transformation based on the current
	 * {@see GetPawnViewLocation}. The returned transformation works in Unreal's left-handed
	 * coordinate system.
	 */
	glm::dmat3 computeEastNorthUpToFixedFrame() const;

	glm::dmat4x4 ecefToUnreal;
};

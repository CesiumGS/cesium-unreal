// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "GlobeAwareDefaultPawn.h"
#include "Camera/CameraComponent.h"
#include "CesiumActors.h"
#include "CesiumCustomVersion.h"
#include "CesiumFlyToComponent.h"
#include "CesiumGeoreference.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumRuntime.h"
#include "CesiumTransforms.h"
#include "CesiumUtility/Math.h"
#include "CesiumWgs84Ellipsoid.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "VecMath.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

#if WITH_EDITOR
#include "Editor.h"
#endif

AGlobeAwareDefaultPawn::AGlobeAwareDefaultPawn() : ADefaultPawn() {
  // Structure to hold one-time initialization
  struct FConstructorStatics {
    ConstructorHelpers::FObjectFinder<UCurveFloat> ProgressCurve;
    ConstructorHelpers::FObjectFinder<UCurveFloat> HeightPercentageCurve;
    ConstructorHelpers::FObjectFinder<UCurveFloat> MaximumHeightByDistanceCurve;
    FConstructorStatics()
        : ProgressCurve(TEXT(
              "/CesiumForUnreal/Curves/FlyTo/Curve_CesiumFlyToDefaultProgress_Float.Curve_CesiumFlyToDefaultProgress_Float")),
          HeightPercentageCurve(TEXT(
              "/CesiumForUnreal/Curves/FlyTo/Curve_CesiumFlyToDefaultHeightPercentage_Float.Curve_CesiumFlyToDefaultHeightPercentage_Float")),
          MaximumHeightByDistanceCurve(TEXT(
              "/CesiumForUnreal/Curves/FlyTo/Curve_CesiumFlyToDefaultMaximumHeightByDistance_Float.Curve_CesiumFlyToDefaultMaximumHeightByDistance_Float")) {
    }
  };
  static FConstructorStatics ConstructorStatics;

  this->FlyToProgressCurve_DEPRECATED = ConstructorStatics.ProgressCurve.Object;
  this->FlyToAltitudeProfileCurve_DEPRECATED =
      ConstructorStatics.HeightPercentageCurve.Object;
  this->FlyToMaximumAltitudeCurve_DEPRECATED =
      ConstructorStatics.MaximumHeightByDistanceCurve.Object;

#if WITH_EDITOR
  this->SetIsSpatiallyLoaded(false);
#endif
  this->GlobeAnchor =
      CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("GlobeAnchor"));
}

void AGlobeAwareDefaultPawn::MoveRight(float Val) {
  this->_moveAlongViewAxis(EAxis::Y, Val);
}

void AGlobeAwareDefaultPawn::MoveForward(float Val) {
  this->_moveAlongViewAxis(EAxis::X, Val);
}

void AGlobeAwareDefaultPawn::MoveUp_World(float Val) {
  if (Val == 0.0f) {
    return;
  }

  ACesiumGeoreference* pGeoreference = this->GetGeoreference();
  if (!IsValid(pGeoreference)) {
    return;
  }

  FVector upEcef = UCesiumWgs84Ellipsoid::GeodeticSurfaceNormal(
      this->GlobeAnchor->GetEarthCenteredEarthFixedPosition());
  FVector up =
      pGeoreference->TransformEarthCenteredEarthFixedDirectionToUnreal(upEcef);

  FTransform transform{};
  USceneComponent* pRootComponent = this->GetRootComponent();
  if (IsValid(pRootComponent)) {
    USceneComponent* pParent = pRootComponent->GetAttachParent();
    if (IsValid(pParent)) {
      transform = pParent->GetComponentToWorld();
    }
  }

  this->_moveAlongVector(transform.TransformVector(up), Val);
}

FRotator AGlobeAwareDefaultPawn::GetViewRotation() const {
  if (!Controller) {
    return this->GetActorRotation();
  }

  ACesiumGeoreference* pGeoreference = this->GetGeoreference();
  if (!pGeoreference) {
    return this->GetActorRotation();
  }

  // The control rotation is expressed in a left-handed East-South-Up (ESU)
  // coordinate system:
  // * Yaw: Clockwise from East: 0 is East, 90 degrees is
  // South, 180 degrees is West, 270 degrees is North.
  // * Pitch: Angle above level, Positive is looking up, negative is looking
  // down
  // * Roll: Rotation around the look direction. Positive is a barrel roll to
  // the right (clockwise).
  FRotator localRotation = Controller->GetControlRotation();

  FTransform transform{};
  USceneComponent* pRootComponent = this->GetRootComponent();
  if (IsValid(pRootComponent)) {
    USceneComponent* pParent = pRootComponent->GetAttachParent();
    if (IsValid(pParent)) {
      transform = pParent->GetComponentToWorld();
    }
  }

  // Transform the rotation in the ESU frame to the Unreal world frame.
  FVector globePosition =
      transform.InverseTransformPosition(this->GetPawnViewLocation());
  FMatrix esuAdjustmentMatrix =
      pGeoreference->ComputeEastSouthUpToUnrealTransformation(globePosition) *
      transform.ToMatrixNoScale();

  return FRotator(esuAdjustmentMatrix.ToQuat() * localRotation.Quaternion());
}

FRotator AGlobeAwareDefaultPawn::GetBaseAimRotation() const {
  return this->GetViewRotation();
}

const FTransform&
AGlobeAwareDefaultPawn::GetGlobeToUnrealWorldTransform() const {
  AActor* pParent = this->GetAttachParentActor();
  if (IsValid(pParent)) {
    return pParent->GetActorTransform();
  }
  return FTransform::Identity;
}

void AGlobeAwareDefaultPawn::FlyToLocationECEF(
    const FVector& ECEFDestination,
    double YawAtDestination,
    double PitchAtDestination,
    bool CanInterruptByMoving) {
  UCesiumFlyToComponent* FlyTo =
      this->FindComponentByClass<UCesiumFlyToComponent>();
  if (!IsValid(FlyTo)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Cannot call deprecated FlyToLocationLongitudeLatitudeHeight because the GlobeAwareDefaultPawn does not have a CesiumFlyToComponent."))
    return;
  }

  // Make sure functions attached to the deprecated delegates will be called.
  FlyTo->OnFlightComplete.AddUniqueDynamic(
      this,
      &AGlobeAwareDefaultPawn::_onFlightComplete);
  FlyTo->OnFlightInterrupted.AddUniqueDynamic(
      this,
      &AGlobeAwareDefaultPawn::_onFlightInterrupted);

  FlyTo->FlyToLocationEarthCenteredEarthFixed(
      ECEFDestination,
      YawAtDestination,
      PitchAtDestination,
      CanInterruptByMoving);
}

void AGlobeAwareDefaultPawn::FlyToLocationLongitudeLatitudeHeight(
    const FVector& LongitudeLatitudeHeightDestination,
    double YawAtDestination,
    double PitchAtDestination,
    bool CanInterruptByMoving) {
  UCesiumFlyToComponent* FlyTo =
      this->FindComponentByClass<UCesiumFlyToComponent>();
  if (!IsValid(FlyTo)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Cannot call deprecated FlyToLocationLongitudeLatitudeHeight because the GlobeAwareDefaultPawn does not have a CesiumFlyToComponent."))
    return;
  }

  // Make sure functions attached to the deprecated delegates will be called.
  FlyTo->OnFlightComplete.AddUniqueDynamic(
      this,
      &AGlobeAwareDefaultPawn::_onFlightComplete);
  FlyTo->OnFlightInterrupted.AddUniqueDynamic(
      this,
      &AGlobeAwareDefaultPawn::_onFlightInterrupted);

  FlyTo->FlyToLocationLongitudeLatitudeHeight(
      LongitudeLatitudeHeightDestination,
      YawAtDestination,
      PitchAtDestination,
      CanInterruptByMoving);
}

void AGlobeAwareDefaultPawn::Serialize(FArchive& Ar) {
  Super::Serialize(Ar);

  Ar.UsingCustomVersion(FCesiumCustomVersion::GUID);
}

void AGlobeAwareDefaultPawn::PostLoad() {
  Super::PostLoad();

  // For backward compatibility, copy the value of the deprecated Georeference
  // property to its new home in the GlobeAnchor. It doesn't appear to be
  // possible to do this in Serialize:
  // https://udn.unrealengine.com/s/question/0D54z00007CAbHFCA1/backward-compatibile-serialization-for-uobject-pointers
  const int32 CesiumVersion =
      this->GetLinkerCustomVersion(FCesiumCustomVersion::GUID);
  if (CesiumVersion < FCesiumCustomVersion::GeoreferenceRefactoring) {
    if (this->Georeference_DEPRECATED != nullptr && this->GlobeAnchor &&
        this->GlobeAnchor->GetGeoreference() == nullptr) {
      this->GlobeAnchor->SetGeoreference(this->Georeference_DEPRECATED);
    }
  }

#if WITH_EDITOR
  if (CesiumVersion < FCesiumCustomVersion::FlyToComponent &&
      !HasAnyFlags(RF_ClassDefaultObject)) {
    // If this is a Blueprint object, like DynamicPawn, its construction
    // scripts may not have been run yet at this point. Doing so might cause
    // a Fly To component to be added. So we force it to happen here so
    // that we don't end up adding a duplicate CesiumFlyToComponent.
    this->RerunConstructionScripts();

    UCesiumFlyToComponent* FlyTo =
        this->FindComponentByClass<UCesiumFlyToComponent>();
    if (!FlyTo) {
      FlyTo = Cast<UCesiumFlyToComponent>(this->AddComponentByClass(
          UCesiumFlyToComponent::StaticClass(),
          false,
          FTransform::Identity,
          false));
      FlyTo->SetFlags(RF_Transactional);
      this->AddInstanceComponent(FlyTo);

      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Added CesiumFlyToComponent to %s in order to preserve backward compatibility."),
          *this->GetName());
    }

    FlyTo->RotationToUse = ECesiumFlyToRotation::ControlRotationInEastSouthUp;
    FlyTo->ProgressCurve = this->FlyToProgressCurve_DEPRECATED;
    FlyTo->HeightPercentageCurve = this->FlyToAltitudeProfileCurve_DEPRECATED;
    FlyTo->MaximumHeightByDistanceCurve =
        this->FlyToMaximumAltitudeCurve_DEPRECATED;
    FlyTo->Duration = this->FlyToDuration_DEPRECATED;
  }
#endif
}

ACesiumGeoreference* AGlobeAwareDefaultPawn::GetGeoreference() const {
  if (!IsValid(this->GlobeAnchor)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "GlobeAwareDefaultPawn %s does not have a valid GlobeAnchorComponent."),
        *this->GetName());
    return nullptr;
  }

  ACesiumGeoreference* pGeoreference = this->GlobeAnchor->ResolveGeoreference();
  if (!IsValid(pGeoreference)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "GlobeAwareDefaultPawn %s does not have a valid CesiumGeoreference."),
        *this->GetName());
    pGeoreference = nullptr;
  }

  return pGeoreference;
}

UCurveFloat* AGlobeAwareDefaultPawn::GetFlyToProgressCurve_DEPRECATED() const {
  UCesiumFlyToComponent* FlyTo =
      this->FindComponentByClass<UCesiumFlyToComponent>();
  if (!IsValid(FlyTo))
    return nullptr;
  return FlyTo->ProgressCurve;
}

void AGlobeAwareDefaultPawn::SetFlyToProgressCurve_DEPRECATED(
    UCurveFloat* NewValue) {
  UCesiumFlyToComponent* FlyTo =
      this->FindComponentByClass<UCesiumFlyToComponent>();
  if (!IsValid(FlyTo))
    return;
  FlyTo->ProgressCurve = NewValue;
}

UCurveFloat*
AGlobeAwareDefaultPawn::GetFlyToAltitudeProfileCurve_DEPRECATED() const {
  UCesiumFlyToComponent* FlyTo =
      this->FindComponentByClass<UCesiumFlyToComponent>();
  if (!IsValid(FlyTo))
    return nullptr;
  return FlyTo->HeightPercentageCurve;
}

void AGlobeAwareDefaultPawn::SetFlyToAltitudeProfileCurve_DEPRECATED(
    UCurveFloat* NewValue) {
  UCesiumFlyToComponent* FlyTo =
      this->FindComponentByClass<UCesiumFlyToComponent>();
  if (!IsValid(FlyTo))
    return;
  FlyTo->HeightPercentageCurve = NewValue;
}

UCurveFloat*
AGlobeAwareDefaultPawn::GetFlyToMaximumAltitudeCurve_DEPRECATED() const {
  UCesiumFlyToComponent* FlyTo =
      this->FindComponentByClass<UCesiumFlyToComponent>();
  if (!IsValid(FlyTo))
    return nullptr;
  return FlyTo->MaximumHeightByDistanceCurve;
}

void AGlobeAwareDefaultPawn::SetFlyToMaximumAltitudeCurve_DEPRECATED(
    UCurveFloat* NewValue) {
  UCesiumFlyToComponent* FlyTo =
      this->FindComponentByClass<UCesiumFlyToComponent>();
  if (!IsValid(FlyTo))
    return;
  FlyTo->MaximumHeightByDistanceCurve = NewValue;
}

float AGlobeAwareDefaultPawn::GetFlyToDuration_DEPRECATED() const {
  UCesiumFlyToComponent* FlyTo =
      this->FindComponentByClass<UCesiumFlyToComponent>();
  if (!IsValid(FlyTo))
    return 0.0f;
  return FlyTo->Duration;
}

void AGlobeAwareDefaultPawn::SetFlyToDuration_DEPRECATED(float NewValue) {
  UCesiumFlyToComponent* FlyTo =
      this->FindComponentByClass<UCesiumFlyToComponent>();
  if (!IsValid(FlyTo))
    return;
  FlyTo->Duration = NewValue;
}

void AGlobeAwareDefaultPawn::_moveAlongViewAxis(EAxis::Type axis, double Val) {
  if (Val == 0.0) {
    return;
  }

  FRotator worldRotation = this->GetViewRotation();
  this->_moveAlongVector(
      FRotationMatrix(worldRotation).GetScaledAxis(axis),
      Val);
}

void AGlobeAwareDefaultPawn::_moveAlongVector(
    const FVector& vector,
    double Val) {
  if (Val == 0.0) {
    return;
  }

  FRotator worldRotation = this->GetViewRotation();
  AddMovementInput(vector, Val);
}

void AGlobeAwareDefaultPawn::_onFlightComplete() {
  this->OnFlightComplete_DEPRECATED.Broadcast();
}

void AGlobeAwareDefaultPawn::_onFlightInterrupted() {
  this->OnFlightInterrupt_DEPRECATED.Broadcast();
}

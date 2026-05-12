// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumEllipsoid.h"
#include "CesiumEllipsoidFunctions.h"
#include "CesiumGeoreference.h"
#include "CesiumRuntime.h"
#include "VecMath.h"

#include "EngineUtils.h"
#include "MathUtil.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/ObjectSaveContext.h"

#if WITH_EDITOR
#include "Editor.h"
#include "LevelEditor.h"
#endif

#include <CesiumGeospatial/Ellipsoid.h>

using namespace CesiumGeospatial;

UCesiumEllipsoid* UCesiumEllipsoid::Create(const FVector& Radii) {
  UCesiumEllipsoid* pEllipsoid = NewObject<UCesiumEllipsoid>();
  pEllipsoid->SetRadii(Radii);
  return pEllipsoid;
}

FVector UCesiumEllipsoid::GetRadii() { return this->Radii; }

void UCesiumEllipsoid::SetRadii(const FVector& NewRadii) {
  this->Radii = NewRadii;
}

double UCesiumEllipsoid::GetMaximumRadius() { return this->Radii.X; }

double UCesiumEllipsoid::GetMinimumRadius() { return this->Radii.Z; }

FVector UCesiumEllipsoid::ScaleToGeodeticSurface(
    const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  return CesiumEllipsoidFunctions::ScaleToGeodeticSurface(
      this->GetNativeEllipsoid(),
      EllipsoidCenteredEllipsoidFixedPosition);
}

FVector UCesiumEllipsoid::GeodeticSurfaceNormal(
    const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  return CesiumEllipsoidFunctions::GeodeticSurfaceNormal(
      this->GetNativeEllipsoid(),
      EllipsoidCenteredEllipsoidFixedPosition);
}

FVector
UCesiumEllipsoid::LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
    const FVector& LongitudeLatitudeHeight) {
  return CesiumEllipsoidFunctions::
      LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
          this->GetNativeEllipsoid(),
          LongitudeLatitudeHeight);
}

FVector
UCesiumEllipsoid::EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(
    const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  return CesiumEllipsoidFunctions::
      EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(
          this->GetNativeEllipsoid(),
          EllipsoidCenteredEllipsoidFixedPosition);
}

FMatrix UCesiumEllipsoid::EastNorthUpToEllipsoidCenteredEllipsoidFixed(
    const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  return CesiumEllipsoidFunctions::EastNorthUpToEllipsoidCenteredEllipsoidFixed(
      this->GetNativeEllipsoid(),
      EllipsoidCenteredEllipsoidFixedPosition);
}

LocalHorizontalCoordinateSystem
UCesiumEllipsoid::CreateCoordinateSystem(const FVector& Center, double Scale) {
  {
    return LocalHorizontalCoordinateSystem(
        VecMath::createVector3D(Center),
        LocalDirection::East,
        LocalDirection::South,
        LocalDirection::Up,
        1.0 / Scale,
        this->GetNativeEllipsoid());
  }
}

const Ellipsoid& UCesiumEllipsoid::GetNativeEllipsoid() {
  const double MinRadiiValue = TMathUtilConstants<double>::Epsilon;

  if (!this->NativeEllipsoid.IsSet()) {
    // Radii of zero will throw Infs and NaNs into our calculations which will
    // cause Unreal to crash when the values reach a transform.
    if (this->Radii.X < MinRadiiValue || this->Radii.Y < MinRadiiValue ||
        this->Radii.Z < MinRadiiValue) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "Radii must be greater than 0 - clamping to minimum value to avoid crashes."));
    }

    this->NativeEllipsoid.Emplace(Ellipsoid(
        FMath::Max(this->Radii.X, MinRadiiValue),
        FMath::Max(this->Radii.Y, MinRadiiValue),
        FMath::Max(this->Radii.Z, MinRadiiValue)));
  }

  return *this->NativeEllipsoid;
}

#if WITH_EDITOR
void UCesiumEllipsoid::PostSaveRoot(
    FObjectPostSaveRootContext ObjectSaveContext) {
  if (!IsValid(GEditor)) {
    return;
  }

  GEditor->GetWorld();

  UWorld* World = GEditor->GetEditorWorldContext().World();
  if (!IsValid(World)) {
    return;
  }

  // Go through every georeference and update its ellipsoid if it's this
  // ellipsoid, since we might have modified values after saving.
  if (ObjectSaveContext.SaveSucceeded()) {
    for (TActorIterator<ACesiumGeoreference> It(World); It; ++It) {
      ACesiumGeoreference* Georeference = *It;
      if (Georeference->GetEllipsoid() == this) {
        Georeference->SetEllipsoid(this);
      }
    }
  }
}
#endif

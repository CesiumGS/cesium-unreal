
#include "CesiumCamera.h"
#include "CesiumRuntime.h"
#include "Math/UnrealMathUtility.h"

FCesiumCamera::FCesiumCamera()
    : ViewportSize(1.0f, 1.0f),
      Location(0.0f, 0.0f, 0.0f),
      Rotation(0.0f, 0.0f, 0.0f),
      FieldOfViewDegrees(0.0f),
      OverrideAspectRatio(0.0f) {}

FCesiumCamera::FCesiumCamera(
    const FVector2D& ViewportSize_,
    const FVector& Location_,
    const FRotator& Rotation_,
    float FieldOfViewDegrees_)
    : ViewportSize(ViewportSize_),
      Location(Location_),
      Rotation(Rotation_),
      FieldOfViewDegrees(FieldOfViewDegrees_),
      OverrideAspectRatio(0.0f) {}

FCesiumCamera::FCesiumCamera(
    const FVector2D& ViewportSize_,
    const FVector& Location_,
    const FRotator& Rotation_,
    float FieldOfViewDegrees_,
    float OverrideAspectRatio_)
    : ViewportSize(ViewportSize_),
      Location(Location_),
      Rotation(Rotation_),
      FieldOfViewDegrees(FieldOfViewDegrees_),
      OverrideAspectRatio(OverrideAspectRatio_) {}
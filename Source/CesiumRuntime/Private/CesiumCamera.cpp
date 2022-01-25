
#include "CesiumCamera.h"
#include "CesiumRuntime.h"
#include "Math/UnrealMathUtility.h"

FCesiumCamera::FCesiumCamera()
    : ViewportSize(1.0f, 1.0f),
      Location(0.0f, 0.0f, 0.0f),
      Rotation(0.0f, 0.0f, 0.0f),
      FieldOfViewDegrees(0.0f),
      AspectRatio(1.0f) {}

FCesiumCamera::FCesiumCamera(
    const FVector2D& ViewportSize_,
    const FVector& Location_,
    const FRotator& Rotation_,
    float FieldOfViewDegrees_)
    : ViewportSize(ViewportSize_),
      Location(Location_),
      Rotation(Rotation_),
      FieldOfViewDegrees(FieldOfViewDegrees_),
      AspectRatio(ViewportSize.X / ViewportSize.Y) {}

FCesiumCamera::FCesiumCamera(
    const FVector2D& ViewportSize_,
    const FVector& Location_,
    const FRotator& Rotation_,
    float FieldOfViewDegrees_,
    float AspectRatio_)
    : ViewportSize(ViewportSize_),
      Location(Location_),
      Rotation(Rotation_),
      FieldOfViewDegrees(FieldOfViewDegrees_),
      AspectRatio(AspectRatio_) {}

Cesium3DTilesSelection::ViewState
FCesiumCamera::createViewState(const glm::dmat4& unrealWorldToTileset) const {

  double horizontalFieldOfView =
      FMath::DegreesToRadians(this->FieldOfViewDegrees);

  double actualAspectRatio;
  glm::dvec2 size(this->ViewportSize.X, this->ViewportSize.Y);

  if (this->AspectRatio != 0.0f) {
    // Use aspect ratio and recompute effective viewport size after black bars
    // are added.
    actualAspectRatio = this->AspectRatio;
    double computedX = actualAspectRatio * this->ViewportSize.Y;
    double computedY = this->ViewportSize.Y / actualAspectRatio;

    double barWidth = this->ViewportSize.X - computedX;
    double barHeight = this->ViewportSize.Y - computedY;

    if (barWidth > 0.0 && barWidth > barHeight) {
      // Black bars on the sides
      size.x = computedX;
    } else if (barHeight > 0.0 && barHeight > barWidth) {
      // Black bars on the top and bottom
      size.y = computedY;
    }
  } else {
    actualAspectRatio = this->ViewportSize.X / this->ViewportSize.Y;
  }

  double verticalFieldOfView =
      atan(tan(horizontalFieldOfView * 0.5) / actualAspectRatio) * 2.0;

  FVector direction = this->Rotation.RotateVector(FVector(1.0f, 0.0f, 0.0f));
  FVector up = this->Rotation.RotateVector(FVector(0.0f, 0.0f, 1.0f));

  glm::dvec3 tilesetCameraLocation = glm::dvec3(
      unrealWorldToTileset *
      glm::dvec4(this->Location.X, this->Location.Y, this->Location.Z, 1.0));
  glm::dvec3 tilesetCameraFront = glm::normalize(glm::dvec3(
      unrealWorldToTileset *
      glm::dvec4(direction.X, direction.Y, direction.Z, 0.0)));
  glm::dvec3 tilesetCameraUp = glm::normalize(
      glm::dvec3(unrealWorldToTileset * glm::dvec4(up.X, up.Y, up.Z, 0.0)));

  return Cesium3DTilesSelection::ViewState::create(
      tilesetCameraLocation,
      tilesetCameraFront,
      tilesetCameraUp,
      size,
      horizontalFieldOfView,
      verticalFieldOfView);
}

#include "CesiumTile.h"
#include "CalcBounds.h"
#include "Components/PrimitiveComponent.h"
#include <glm/gtc/matrix_inverse.hpp>

FBoxSphereBounds UCesiumTile::GetBounds() const {
  return std::visit(
      CalcBoundsOperation{FTransform(), glm::affineInverse(_transform)},
      _pTile->getBoundingVolume());
}

bool UCesiumTile::IsIntersecting(UPrimitiveComponent* Other) const {
  if (Other) {
    FBoxSphereBounds Bounds = GetBounds();
    return FBoxSphereBounds::BoxesIntersect(Bounds, Other->Bounds) ||
           FBoxSphereBounds::SpheresIntersect(Bounds, Other->Bounds);
  } else {
    return false;
  }
}

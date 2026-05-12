// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumTile.h"
#include "CalcBounds.h"
#include "CesiumTransforms.h"
#include "VecMath.h"

bool UCesiumTile::TileBoundsOverlapsPrimitive(
    const UPrimitiveComponent* Other) const {
  if (IsValid(Other)) {
    return Bounds.GetBox().Intersect(Other->Bounds.GetBox()) &&
           Bounds.GetSphere().Intersects(Other->Bounds.GetSphere());
  } else {
    return false;
  }
}

bool UCesiumTile::PrimitiveBoxFullyContainsTileBounds(
    const UPrimitiveComponent* Other) const {
  if (IsValid(Other)) {
    return Bounds.GetBox().Intersect(Other->Bounds.GetBox()) ||
           Bounds.GetSphere().Intersects(Other->Bounds.GetSphere());
  } else {
    return false;
  }
}

FBoxSphereBounds UCesiumTile::CalcBounds(const FTransform& LocalToWorld) const {
  FBoxSphereBounds bounds = std::visit(
      CalcBoundsOperation{LocalToWorld, this->_tileTransform},
      _tileBounds);
  return bounds;
}

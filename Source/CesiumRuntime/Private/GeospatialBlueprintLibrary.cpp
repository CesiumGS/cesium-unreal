// Fill out your copyright notice in the Description page of Project Settings.


#include "GeospatialBlueprintLibrary.h"

#include "GeospatialLibrary.h"
#include <glm/mat3x3.hpp>

FMatrix UGeospatialBlueprintLibrary::InaccurateComputeEastNorthUpToUnreal(
    const FVector& Ue) {
  glm::dmat3 enuToUnreal =
    UGeospatialLibrary::ComputeEastNorthUpToUnreal(glm::dvec3(Ue.X, Ue.Y, Ue.Z));

  return FMatrix(
      FVector(enuToUnreal[0].x, enuToUnreal[0].y, enuToUnreal[0].z),
      FVector(enuToUnreal[1].x, enuToUnreal[1].y, enuToUnreal[1].z),
      FVector(enuToUnreal[2].x, enuToUnreal[2].y, enuToUnreal[2].z),
      FVector::ZeroVector);
}

FMatrix UGeospatialBlueprintLibrary::ComputeEastNorthUpToEcef(
    const FVector& Ecef) {
    glm::dmat3 enuToEcef =
        UGeospatialLibrary::ComputeEastNorthUpToEcef(glm::dvec3(Ecef.X, Ecef.Y, Ecef.Z));
  
    return FMatrix(
        FVector(enuToEcef[0].x, enuToEcef[0].y, enuToEcef[0].z),
        FVector(enuToEcef[1].x, enuToEcef[1].y, enuToEcef[1].z),
        FVector(enuToEcef[2].x, enuToEcef[2].y, enuToEcef[2].z),
        FVector::ZeroVector);
}


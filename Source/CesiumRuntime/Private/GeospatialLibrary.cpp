// Fill out your copyright notice in the Description page of Project Settings.


#include "GeospatialLibrary.h"

#include <CesiumGeospatial/Transforms.h>

glm::dmat3 UGeospatialLibrary::
ComputeEastNorthUpToEcef(const glm::dvec3& Ecef) {
  return glm::dmat3(
    CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(Ecef));
}

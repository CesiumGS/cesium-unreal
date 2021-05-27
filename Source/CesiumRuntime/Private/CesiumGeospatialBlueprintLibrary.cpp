// Fill out your copyright notice in the Description page of Project Settings.

#include "CesiumGeospatialBlueprintLibrary.h"

#include "CesiumGeospatialLibrary.h"
#include "CesiumRuntime.h"
#include "Engine/World.h"
#include "VecMath.h"

ACesiumGeoreference* UCesiumGeospatialBlueprintLibrary::GetDefaultGeoref() {
  // TODO
  return nullptr;
}

FVector UCesiumGeospatialBlueprintLibrary::TransformLongLatHeightToUnreal(
    const FVector& LongLatHeight,
    const ACesiumGeoreference* Georef) {

  const glm::dmat4& EcefToUeAbsoluteWorld =
      Georef->GetEllipsoidCenteredToUnrealWorldTransform();

  const glm::dvec3 originLocation =
      VecMath::createVector3D(Georef->GetWorld()->OriginLocation);

  glm::dvec3 ue = UCesiumGeospatialLibrary::TransformLongLatHeightToUnreal(
      VecMath::createVector3D(LongLatHeight),
      EcefToUeAbsoluteWorld,
      originLocation);

  return FVector(ue.x, ue.y, ue.z);
}

FVector UCesiumGeospatialBlueprintLibrary::
    TransformLongLatHeightToUnrealUsingDefaultGeoref(
        const FVector& LongLatHeight) {
  return TransformLongLatHeightToUnreal(LongLatHeight, GetDefaultGeoref());
}

FVector UCesiumGeospatialBlueprintLibrary::TransformUnrealToLongLatHeight(
    const FVector& Ue,
    const ACesiumGeoreference* Georef) {

  const glm::dmat4& UeAbsoluteWorldToEcef =
      Georef->GetUnrealWorldToEllipsoidCenteredTransform();

  const glm::dvec3 originLocation =
      VecMath::createVector3D(Georef->GetWorld()->OriginLocation);

  glm::dvec3 longLatHeight =
      UCesiumGeospatialLibrary::TransformUnrealToLongLatHeight(
          VecMath::createVector3D(Ue),
          UeAbsoluteWorldToEcef,
          originLocation);
  return FVector(longLatHeight.x, longLatHeight.y, longLatHeight.z);
}

FVector UCesiumGeospatialBlueprintLibrary::
    TransformUnrealToLongLatHeightUsingDefaultGeoref(const FVector& Ue) {
  return TransformUnrealToLongLatHeight(Ue, GetDefaultGeoref());
}

FVector UCesiumGeospatialBlueprintLibrary::TransformLongLatHeightToEcef(
    const FVector& LongLatHeight) {

  glm::dvec3 ecef = UCesiumGeospatialLibrary::TransformLongLatHeightToEcef(
      glm::dvec3(LongLatHeight.X, LongLatHeight.Y, LongLatHeight.Z));
  return FVector(ecef.x, ecef.y, ecef.z);
}

FVector UCesiumGeospatialBlueprintLibrary::TransformEcefToLongLatHeight(
    const FVector& Ecef) {
  glm::dvec3 llh = UCesiumGeospatialLibrary::TransformEcefToLongLatHeight(
      glm::dvec3(Ecef.X, Ecef.Y, Ecef.Z));
  return FVector(llh.x, llh.y, llh.z);
}

FRotator UCesiumGeospatialBlueprintLibrary::TransformRotatorEastNorthUpToUnreal(
    const FRotator& EnuRotator,
    const FVector& UeLocation,
    ACesiumGeoreference* Georef) {

  const glm::dmat4& ueAbsoluteWorldToEcef =
      Georef->GetUnrealWorldToEllipsoidCenteredTransform();

  const glm::dvec3 originLocation =
      VecMath::createVector3D(Georef->GetWorld()->OriginLocation);

  const glm::dmat4& ecefToGeoreferenced =
      Georef->GetEllipsoidCenteredToGeoreferencedTransform();

  const glm::dmat3 adjustedRotation =
      UCesiumGeospatialLibrary::TransformRotatorEastNorthUpToUnreal(
          VecMath::createRotationMatrix4D(EnuRotator),
          VecMath::createVector3D(UeLocation),
          ueAbsoluteWorldToEcef,
          originLocation,
          ecefToGeoreferenced);

  return VecMath::createRotator(adjustedRotation);
}

FRotator UCesiumGeospatialBlueprintLibrary::
    TransformRotatorEastNorthUpToUnrealUsingDefaultGeoref(
        const FRotator& EnuRotator,
        const FVector& UeLocation) {
  return TransformRotatorEastNorthUpToUnreal(
      EnuRotator,
      UeLocation,
      GetDefaultGeoref());
}

FRotator UCesiumGeospatialBlueprintLibrary::TransformRotatorUnrealToEastNorthUp(
    const FRotator& UeRotator,
    const FVector& UeLocation,
    ACesiumGeoreference* Georef) {

  if (!IsValid(Georef)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Georef invalid in TransformRotatorUnrealToEastNorthUp call"));
    return FRotator::ZeroRotator;
  }

  const glm::dmat4& ueAbsoluteWorldToEcef =
      Georef->GetUnrealWorldToEllipsoidCenteredTransform();

  const glm::dvec3& originLocation =
      VecMath::createVector3D(Georef->GetWorld()->OriginLocation);

  const glm::dmat4& ecefToGeoreferenced =
      Georef->GetEllipsoidCenteredToGeoreferencedTransform();

  const glm::dmat3& adjustedRotation =
      UCesiumGeospatialLibrary::TransformRotatorUnrealToEastNorthUp(
          VecMath::createRotationMatrix4D(UeRotator),
          VecMath::createVector3D(UeLocation),
          ueAbsoluteWorldToEcef,
          originLocation,
          ecefToGeoreferenced);

  return VecMath::createRotator(adjustedRotation);
}

FRotator UCesiumGeospatialBlueprintLibrary::
    TransformRotatorUnrealToEastNorthUpUsingDefaultGeoref(
        const FRotator& UeRotator,
        const FVector& UeLocation) {
  return TransformRotatorUnrealToEastNorthUp(
      UeRotator,
      UeLocation,
      GetDefaultGeoref());
}

FMatrix UCesiumGeospatialBlueprintLibrary::ComputeEastNorthUpToUnreal(
    const FVector& Ue,
    ACesiumGeoreference* Georef) {

  if (!IsValid(Georef)) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Georef invalid in ComputeEastNorthUpToUnreal call"));
    return FMatrix::Identity;
  }

  const glm::dmat4& ueAbsoluteWorldToEcef =
      Georef->GetUnrealWorldToEllipsoidCenteredTransform();

  const glm::dvec3& originLocation =
      VecMath::createVector3D(Georef->GetWorld()->OriginLocation);

  const glm::dmat3& ecefToGeoreferenced =
      glm::dmat3(Georef->GetEllipsoidCenteredToGeoreferencedTransform());

  const glm::dmat3& enuToUnreal =
      UCesiumGeospatialLibrary::ComputeEastNorthUpToUnreal(
          VecMath::createVector3D(Ue),
          ueAbsoluteWorldToEcef,
          originLocation,
          ecefToGeoreferenced);

  return VecMath::createMatrix(enuToUnreal);
}

FMatrix
UCesiumGeospatialBlueprintLibrary::ComputeEastNorthUpToUnrealUsingDefaultGeoref(
    const FVector& Ue) {
  return ComputeEastNorthUpToUnreal(Ue, GetDefaultGeoref());
}

FMatrix UCesiumGeospatialBlueprintLibrary::ComputeEastNorthUpToEcef(
    const FVector& Ecef) {
  glm::dmat3 enuToEcef = UCesiumGeospatialLibrary::ComputeEastNorthUpToEcef(
      glm::dvec3(Ecef.X, Ecef.Y, Ecef.Z));

  return VecMath::createMatrix(enuToEcef);
}

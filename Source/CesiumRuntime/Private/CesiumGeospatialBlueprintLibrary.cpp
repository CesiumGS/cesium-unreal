// Fill out your copyright notice in the Description page of Project Settings.

#include "CesiumGeospatialBlueprintLibrary.h"

#include "CesiumGeospatialLibrary.h"
#include "Engine/World.h"
#include <glm/fwd.hpp>
#include <glm/mat3x3.hpp>

ACesiumGeoreference* UCesiumGeospatialBlueprintLibrary::GetDefaultGeoref() {
  // TODO
  return nullptr;
}

FVector UCesiumGeospatialBlueprintLibrary::TransformLongLatHeightToUnreal(
    const FVector& LongLatHeight,
    const ACesiumGeoreference* Georef) {
  glm::dvec3 longLatHeight(LongLatHeight.X, LongLatHeight.Y, LongLatHeight.Z);

  const glm::dmat4& EcefToUeAbsoluteWorld =
      Georef->GetEllipsoidCenteredToUnrealWorldTransform();

  const FIntVector& originLocation = Georef->GetWorld()->OriginLocation;
  const glm::dvec3 originLocationGlm =
      glm::dvec3(originLocation.X, originLocation.Y, originLocation.Z);

  glm::dvec3 ue = UCesiumGeospatialLibrary::TransformLongLatHeightToUnreal(
      longLatHeight,
      EcefToUeAbsoluteWorld,
      originLocationGlm);
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

  glm::dvec3 ue(Ue.X, Ue.Y, Ue.Z);

  const glm::dmat4& UeAbsoluteWorldToEcef =
      Georef->GetUnrealWorldToEllipsoidCenteredTransform();

  const FIntVector& originLocation = Georef->GetWorld()->OriginLocation;
  const glm::dvec3 originLocationGlm =
      glm::dvec3(originLocation.X, originLocation.Y, originLocation.Z);

  glm::dvec3 longLatHeight =
      UCesiumGeospatialLibrary::TransformUnrealToLongLatHeight(
          ue,
          UeAbsoluteWorldToEcef,
          originLocationGlm);
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

  const FMatrix enu = FRotationMatrix::Make(EnuRotator);

  // clang-format off
  const glm::dmat3 enuRotation(
    enu.M[0][0], enu.M[0][1], enu.M[0][2],
    enu.M[1][0], enu.M[1][1], enu.M[1][2],
    enu.M[2][0], enu.M[2][1], enu.M[2][2]);
  // clang-format on

  glm::dvec3 ueLocation(UeLocation.X, UeLocation.Y, UeLocation.Z);

  const glm::dmat4& ueAbsoluteWorldToEcef =
      Georef->GetUnrealWorldToEllipsoidCenteredTransform();

  const FIntVector& originLocationInt = Georef->GetWorld()->OriginLocation;
  const glm::dvec3 originLocation =
      glm::dvec3(originLocationInt.X, originLocationInt.Y, originLocationInt.Z);

  const glm::dmat4& ecefToGeoreferenced =
      Georef->GetEllipsoidCenteredToGeoreferencedTransform();

  const glm::dmat3 adjustedRotation =
      UCesiumGeospatialLibrary::TransformRotatorEastNorthUpToUnreal(
          enuRotation,
          ueLocation,
          ueAbsoluteWorldToEcef,
          originLocation,
          ecefToGeoreferenced);

  return FRotator(); // TODO: convert from dmat3 back to FRotator here
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

  const FMatrix ue = FRotationMatrix::Make(UeRotator);

  // clang-format off
  const glm::dmat3 enuRotation(
    ue.M[0][0], ue.M[0][1], ue.M[0][2],
    ue.M[1][0], ue.M[1][1], ue.M[1][2],
    ue.M[2][0], ue.M[2][1], ue.M[2][2]);
  // clang-format on

  glm::dvec3 ueLocation(UeLocation.X, UeLocation.Y, UeLocation.Z);

  const glm::dmat4& ueAbsoluteWorldToEcef =
      Georef->GetUnrealWorldToEllipsoidCenteredTransform();

  const FIntVector& originLocationInt = Georef->GetWorld()->OriginLocation;
  const glm::dvec3 originLocation =
      glm::dvec3(originLocationInt.X, originLocationInt.Y, originLocationInt.Z);

  const glm::dmat4& ecefToGeoreferenced =
      Georef->GetEllipsoidCenteredToGeoreferencedTransform();

  const glm::dmat3 adjustedRotation =
      UCesiumGeospatialLibrary::TransformRotatorUnrealToEastNorthUp(
          enuRotation,
          ueLocation,
          ueAbsoluteWorldToEcef,
          originLocation,
          ecefToGeoreferenced);

  return FRotator(); // TODO: convert from dmat3 back to FRotator here
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
  glm::dvec3 ue(Ue.X, Ue.Y, Ue.Z);

  const glm::dmat4& ueAbsoluteWorldToEcef =
      Georef->GetUnrealWorldToEllipsoidCenteredTransform();

  const FIntVector& originLocationIntVector =
      Georef->GetWorld()->OriginLocation;
  const glm::dvec3 originLocation = glm::dvec3(
      originLocationIntVector.X,
      originLocationIntVector.Y,
      originLocationIntVector.Z);

  const glm::dmat3& ecefToGeoreferenced =
      glm::dmat3(Georef->GetEllipsoidCenteredToGeoreferencedTransform());

  glm::dmat3 enuToUnreal = UCesiumGeospatialLibrary::ComputeEastNorthUpToUnreal(
      ue,
      ueAbsoluteWorldToEcef,
      originLocation,
      ecefToGeoreferenced);

  return FMatrix(
      FVector(enuToUnreal[0].x, enuToUnreal[0].y, enuToUnreal[0].z),
      FVector(enuToUnreal[1].x, enuToUnreal[1].y, enuToUnreal[1].z),
      FVector(enuToUnreal[2].x, enuToUnreal[2].y, enuToUnreal[2].z),
      FVector::ZeroVector);
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

  return FMatrix(
      FVector(enuToEcef[0].x, enuToEcef[0].y, enuToEcef[0].z),
      FVector(enuToEcef[1].x, enuToEcef[1].y, enuToEcef[1].z),
      FVector(enuToEcef[2].x, enuToEcef[2].y, enuToEcef[2].z),
      FVector::ZeroVector);
}

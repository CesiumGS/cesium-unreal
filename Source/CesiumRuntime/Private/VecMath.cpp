// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "VecMath.h"

#include "CesiumUtility/Math.h"
#include "Math/Quat.h"
#include "Math/RotationMatrix.h"
#include <CesiumGeometry/Transforms.h>
#include <glm/gtc/quaternion.hpp>

glm::dmat4 VecMath::createMatrix4D(const FMatrix& m) noexcept {
  return glm::dmat4(
      m.M[0][0],
      m.M[0][1],
      m.M[0][2],
      m.M[0][3],
      m.M[1][0],
      m.M[1][1],
      m.M[1][2],
      m.M[1][3],
      m.M[2][0],
      m.M[2][1],
      m.M[2][2],
      m.M[2][3],
      m.M[3][0],
      m.M[3][1],
      m.M[3][2],
      m.M[3][3]);
}

glm::dmat4 VecMath::createMatrix4D(
    const FMatrix& m,
    const glm::dvec3& translation) noexcept {
  return VecMath::createMatrix4D(
      m,
      translation.x,
      translation.y,
      translation.z,
      1.0);
}

glm::dmat4 VecMath::createMatrix4D(
    const FMatrix& m,
    const glm::dvec4& translation) noexcept {
  return VecMath::createMatrix4D(
      m,
      translation.x,
      translation.y,
      translation.z,
      translation.w);
}

glm::dmat4 VecMath::createMatrix4D(
    const FMatrix& m,
    double tx,
    double ty,
    double tz,
    double tw) noexcept {
  return glm::dmat4(
      m.M[0][0],
      m.M[0][1],
      m.M[0][2],
      m.M[0][3],
      m.M[1][0],
      m.M[1][1],
      m.M[1][2],
      m.M[1][3],
      m.M[2][0],
      m.M[2][1],
      m.M[2][2],
      m.M[2][3],
      tx,
      ty,
      tz,
      tw);
}

glm::dmat4 VecMath::createTranslationMatrix4D(
    double tx,
    double ty,
    double tz,
    double tw) noexcept {
  return glm::dmat4(
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      1.0,
      0.0,
      0.0,
      0.0,
      0.0,
      1.0,
      0.0,
      tx,
      ty,
      tz,
      tw);
}

glm::dmat4 VecMath::createRotationMatrix4D(const FRotator& rot) noexcept {
  const FMatrix& m = FRotationMatrix::Make(rot);
  return createMatrix4D(m);
}

glm::dvec3 VecMath::createVector3D(const FVector& v) noexcept {
  return glm::dvec3(v.X, v.Y, v.Z);
}

glm::dvec3 VecMath::createVector3D(const FIntVector& v) noexcept {
  return glm::dvec3(v.X, v.Y, v.Z);
}

glm::dquat VecMath::createQuaternion(const FQuat& q) noexcept {
  return glm::dquat(q.W, q.X, q.Y, q.Z);
}

FMatrix VecMath::createMatrix(const glm::dmat4& m) noexcept {
  return FMatrix(
      FVector(m[0].x, m[0].y, m[0].z),
      FVector(m[1].x, m[1].y, m[1].z),
      FVector(m[2].x, m[2].y, m[2].z),
      FVector(m[3].x, m[3].y, m[3].z));
}

FTransform VecMath::createTransform(const glm::dmat4& m) noexcept {
  glm::dvec3 translation;
  glm::dquat rotation;
  glm::dvec3 scale;
  CesiumGeometry::Transforms::computeTranslationRotationScaleFromMatrix(
      m,
      &translation,
      &rotation,
      &scale);

  return FTransform(
      VecMath::createQuaternion(rotation),
      VecMath::createVector(translation),
      VecMath::createVector(scale));
}

FMatrix VecMath::createMatrix(const glm::dmat3& m) noexcept {
  return FMatrix(
      FVector(m[0].x, m[0].y, m[0].z),
      FVector(m[1].x, m[1].y, m[1].z),
      FVector(m[2].x, m[2].y, m[2].z),
      FVector::ZeroVector);
}

FMatrix VecMath::createMatrix(
    const glm::dvec3& column0,
    const glm::dvec3& column1,
    const glm::dvec3& column2) noexcept {
  return FMatrix(
      FVector(column0.x, column0.y, column0.z),
      FVector(column1.x, column1.y, column1.z),
      FVector(column2.x, column2.y, column2.z),
      FVector::ZeroVector);
}

FVector VecMath::createVector(const glm::dvec4& v) noexcept {
  return FVector(v.x, v.y, v.z);
}

FVector VecMath::createVector(const glm::dvec3& v) noexcept {
  return FVector(v.x, v.y, v.z);
}

FRotator VecMath::createRotator(const glm::dmat4& m) noexcept {
  // Avoid converting to Unreal single-precision types until the very end, so
  // that all intermediate conversions are performed in double-precision.
  return VecMath::createRotator(quat_cast(m));
}

FRotator VecMath::createRotator(const glm::dmat3& m) noexcept {
  return VecMath::createRotator(quat_cast(m));
}

FRotator VecMath::createRotator(const glm::dquat& q) noexcept {
  return FRotator(FQuat(q.x, q.y, q.z, q.w));
}

FQuat VecMath::createQuaternion(const glm::dquat& q) noexcept {
  return FQuat(q.x, q.y, q.z, q.w);
}

glm::dvec4 VecMath::add4D(const FVector& f, const FIntVector& i) noexcept {
  return glm::dvec4(VecMath::add3D(f, i), 1.0);
}

glm::dvec4 VecMath::add4D(const FIntVector& i, const FVector& f) noexcept {
  return glm::dvec4(VecMath::add3D(i, f), 1.0);
}

glm::dvec4 VecMath::add4D(const glm::dvec4& d, const FIntVector& i) noexcept {
  return glm::dvec4(VecMath::add3D(glm::dvec3(d), i), d.w);
}

glm::dvec3 VecMath::add3D(const FIntVector& i, const FVector& f) noexcept {
  return glm::dvec3(
      static_cast<double>(i.X) + f.X,
      static_cast<double>(i.Y) + f.Y,
      static_cast<double>(i.Z) + f.Z);
}

glm::dvec3 VecMath::add3D(const FVector& f, const FIntVector& i) noexcept {
  return glm::dvec3(
      static_cast<double>(f.X) + i.X,
      static_cast<double>(f.Y) + i.Y,
      static_cast<double>(f.Z) + i.Z);
}

glm::dvec3 VecMath::add3D(const glm::dvec3& f, const FIntVector& i) noexcept {
  return glm::dvec3(f.x + i.X, f.y + i.Y, f.z + i.Z);
}

glm::dvec4 VecMath::subtract4D(const FVector& f, const FIntVector& i) noexcept {
  return glm::dvec4(VecMath::subtract3D(f, i), 1.0);
}

glm::dvec4 VecMath::subtract4D(const FIntVector& i, const FVector& f) noexcept {
  return glm::dvec4(VecMath::subtract3D(i, f), 1.0);
}

glm::dvec3 VecMath::subtract3D(const FVector& f, const FIntVector& i) noexcept {
  return glm::dvec3(
      static_cast<double>(f.X) - i.X,
      static_cast<double>(f.Y) - i.Y,
      static_cast<double>(f.Z) - i.Z);
}

glm::dvec3 VecMath::subtract3D(const FIntVector& i, const FVector& f) noexcept {
  return glm::dvec3(
      static_cast<double>(i.X) - f.X,
      static_cast<double>(i.Y) - f.Y,
      static_cast<double>(i.Z) - f.Z);
}

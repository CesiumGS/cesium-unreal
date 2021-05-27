// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "VecMath.h"

#include <CesiumGeometry/AxisTransforms.h>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform.hpp>

inline glm::dmat4 VecMath::createMatrix4D(const FMatrix& m) noexcept {
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

inline glm::dmat4 VecMath::createMatrix4D(
    const FMatrix& m,
    const glm::dvec3& translation) noexcept {
  return VecMath::createMatrix4D(
      m,
      translation.x,
      translation.y,
      translation.z,
      1.0);
}

inline glm::dmat4 VecMath::createMatrix4D(
    const FMatrix& m,
    const glm::dvec4& translation) noexcept {
  return VecMath::createMatrix4D(
      m,
      translation.x,
      translation.y,
      translation.z,
      translation.w);
}

inline glm::dmat4 VecMath::createMatrix4D(
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

inline glm::dmat4 VecMath::createTranslationMatrix4D(
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

inline glm::dmat4 VecMath::createRotationMatrix4D(const FRotator& rot) {
  const FMatrix& m = FRotationMatrix::Make(rot);
  return createMatrix4D(m);
}

inline glm::dvec3 VecMath::createVector3D(const FVector& v) noexcept {
  return glm::dvec3(v.X, v.Y, v.Z);
}

inline glm::dvec3 VecMath::createVector3D(const FIntVector& v) noexcept {
  return glm::dvec3(v.X, v.Y, v.Z);
}

inline glm::dquat VecMath::createQuaternion(const FQuat& q) noexcept {
  return glm::dquat(q.W, q.X, q.Y, q.Z);
}

inline FMatrix VecMath::createMatrix(const glm::dmat4& m) noexcept {
  return FMatrix(
      FVector(m[0].x, m[0].y, m[0].z),
      FVector(m[1].x, m[1].y, m[1].z),
      FVector(m[2].x, m[2].y, m[2].z),
      FVector(m[3].x, m[3].y, m[3].z));
}

inline FMatrix VecMath::createMatrix(const glm::dmat3& m) noexcept {
  return FMatrix(
      FVector(m[0].x, m[0].y, m[0].z),
      FVector(m[1].x, m[1].y, m[1].z),
      FVector(m[2].x, m[2].y, m[2].z),
      FVector::ZeroVector);
}

inline FMatrix VecMath::createMatrix(
    const glm::dvec3& column0,
    const glm::dvec3& column1,
    const glm::dvec3& column2) noexcept {
  return FMatrix(
      FVector(column0.x, column0.y, column0.z),
      FVector(column1.x, column1.y, column1.z),
      FVector(column2.x, column2.y, column2.z),
      FVector::ZeroVector);
}

inline FRotator VecMath::createRotator(const glm::dmat4& m) noexcept {
  // Avoid converting to Unreal single-precision types until the very end, so
  // that all intermediate conversions are performed in double-precision.
  const glm::dquat& q = quat_cast(m);
  return FRotator(pitch(q), yaw(q), roll(q));
}

inline FRotator VecMath::createRotator(const glm::dmat3& m) noexcept {
  const glm::dquat& q = quat_cast(m);
  return FRotator(pitch(q), yaw(q), roll(q));
}

inline FRotator VecMath::createRotator(const glm::dquat& q) noexcept {
  return FRotator(pitch(q), yaw(q), roll(q));
}

inline FQuat VecMath::createQuaternion(const glm::dquat& q) noexcept {
  return FQuat(q.x, q.y, q.z, q.w);
}

inline glm::dvec4
VecMath::add4D(const FVector& f, const FIntVector& i) noexcept {
  return glm::dvec4(VecMath::add3D(f, i), 1.0);
}

inline glm::dvec4
VecMath::add4D(const FIntVector& i, const FVector& f) noexcept {
  return glm::dvec4(VecMath::add3D(i, f), 1.0);
}

inline glm::dvec3
VecMath::add3D(const FIntVector& i, const FVector& f) noexcept {
  return glm::dvec3(
      static_cast<double>(i.X) + f.X,
      static_cast<double>(i.Y) + f.Y,
      static_cast<double>(i.Z) + f.Z);
}

inline glm::dvec3
VecMath::add3D(const FVector& f, const FIntVector& i) noexcept {
  return glm::dvec3(
      static_cast<double>(f.X) + i.X,
      static_cast<double>(f.Y) + i.Y,
      static_cast<double>(f.Z) + i.Z);
}

inline glm::dvec3
VecMath::add3D(const glm::vec3& f, const FIntVector& i) noexcept {
  return glm::dvec3(f.x + i.X, f.y + i.Y, f.z + i.Z);
}

inline glm::dvec4
VecMath::subtract4D(const FVector& f, const FIntVector& i) noexcept {
  return glm::dvec4(VecMath::subtract3D(f, i), 1.0);
}

inline glm::dvec4
VecMath::subtract4D(const FIntVector& i, const FVector& f) noexcept {
  return glm::dvec4(VecMath::subtract3D(i, f), 1.0);
}

inline glm::dvec3
VecMath::subtract3D(const FVector& f, const FIntVector& i) noexcept {
  return glm::dvec3(
      static_cast<double>(f.X) - i.X,
      static_cast<double>(f.Y) - i.Y,
      static_cast<double>(f.Z) - i.Z);
}

inline glm::dvec3
VecMath::subtract3D(const FIntVector& i, const FVector& f) noexcept {
  return glm::dvec3(
      static_cast<double>(i.X) - f.X,
      static_cast<double>(i.Y) - f.Y,
      static_cast<double>(i.Z) - f.Z);
}

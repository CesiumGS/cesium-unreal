// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#include "CesiumGltfGaussianSplatComponent.h"

#include "CesiumGaussianSplatSubsystem.h"
#include "CesiumRuntime.h"
#include "VecMath.h"

#include "Engine/Engine.h"
#include "Math/Box.h"
#include "Misc/Optional.h"

#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/MeshPrimitive.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <fmt/format.h>

namespace {
int32 countShCoeffsOnPrimitive(const CesiumGltf::MeshPrimitive& primitive) {
  if (primitive.attributes.contains(
          "KHR_gaussian_splatting:SH_DEGREE_3_COEF_6")) {
    return 15;
  }

  if (primitive.attributes.contains(
          "KHR_gaussian_splatting:SH_DEGREE_2_COEF_4")) {
    return 8;
  }

  if (primitive.attributes.contains(
          "KHR_gaussian_splatting:SH_DEGREE_1_COEF_2")) {
    return 3;
  }

  return 0;
}

bool writeShCoeffs(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& meshPrimitive,
    TArray<float>& data,
    int32 stride,
    int32 offset,
    int32 degree) {
  const int32 numCoeffs = 3 + 2 * (degree - 1);
  for (int32 i = 0; i < numCoeffs; i++) {
    std::unordered_map<std::string, int32_t>::const_iterator accessorIt =
        meshPrimitive.attributes.find(fmt::format(
            "KHR_gaussian_splatting:SH_DEGREE_{}_COEF_{}",
            degree,
            i));
    if (accessorIt == meshPrimitive.attributes.end()) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Could not find spherical harmonic attribute for degree %d index %d on mesh primitive"),
          degree,
          i);
      return false;
    }

    CesiumGltf::AccessorView<glm::vec3> accessorView(model, accessorIt->second);
    if (accessorView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Accessor view for spherical harmonic attribute degree %d index %d on mesh primitive returned invalid status: %d"),
          degree,
          i,
          accessorView.status());
      return false;
    }

    for (int32 j = 0; j < accessorView.size(); j++) {
      data[j * stride + offset + i * 4] = accessorView[j].x;
      data[j * stride + offset + i * 4 + 1] = accessorView[j].y;
      data[j * stride + offset + i * 4 + 2] = accessorView[j].z;
      data[j * stride + offset + i * 4 + 3] = 0.0;
    }
  }

  return true;
}

template <typename T, typename ComponentT>
void writeConvertedAccessor4(
    const CesiumGltf::AccessorView<T>& accessorView,
    TArray<float>& data,
    int32 stride,
    TFunction<glm::vec4(glm::vec4)> convert) {
  for (int32 i = 0; i < accessorView.size(); i++) {
    glm::vec4 vec = glm::vec4(
        accessorView[i].x /
            static_cast<float>(TNumericLimits<ComponentT>::Max()),
        accessorView[i].y /
            static_cast<float>(TNumericLimits<ComponentT>::Max()),
        accessorView[i].z /
            static_cast<float>(TNumericLimits<ComponentT>::Max()),
        accessorView[i].w /
            static_cast<float>(TNumericLimits<ComponentT>::Max()));
    vec = convert(vec);
    data[i * stride] = vec.x;
    data[i * stride + 1] = vec.y;
    data[i * stride + 2] = vec.z;
    data[i * stride + 3] = vec.w;
  }
}

template <typename T, typename ComponentT>
void writeConvertedAccessor3(
    const CesiumGltf::AccessorView<T>& accessorView,
    TArray<float>& data,
    int32 stride,
    TFunction<glm::vec3(glm::vec3)> convert) {
  for (int32 i = 0; i < accessorView.size(); i++) {
    glm::vec3 vec = glm::vec3(
        accessorView[i].x /
            static_cast<float>(TNumericLimits<ComponentT>::Max()),
        accessorView[i].y /
            static_cast<float>(TNumericLimits<ComponentT>::Max()),
        accessorView[i].z /
            static_cast<float>(TNumericLimits<ComponentT>::Max()));
    vec = convert(vec);
    data[i * stride] = vec.x;
    data[i * stride + 1] = vec.y;
    data[i * stride + 2] = vec.z;
  }
}
} // namespace

void UCesiumGltfGaussianSplatComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  UCesiumGltfPrimitiveComponent::UpdateTransformFromCesium(
      CesiumToUnrealTransform);

  UCesiumGaussianSplatSubsystem* pSplatSubsystem =
      UCesiumGaussianSplatSubsystem::Get();
  ensure(pSplatSubsystem);
  pSplatSubsystem->RecomputeBounds();
}

void UCesiumGltfGaussianSplatComponent::OnUpdateTransform(
    EUpdateTransformFlags /*UpdateTransformFlags*/,
    ETeleportType /*Teleport*/) {
  UCesiumGaussianSplatSubsystem* pSplatSubsystem =
      UCesiumGaussianSplatSubsystem::Get();
  ensure(pSplatSubsystem);
  pSplatSubsystem->RecomputeBounds();
}

void UCesiumGltfGaussianSplatComponent::OnVisibilityChanged() {
  UCesiumGaussianSplatSubsystem* pSplatSubsystem =
      UCesiumGaussianSplatSubsystem::Get();
  ensure(pSplatSubsystem);
  pSplatSubsystem->RecomputeBounds();
}

glm::mat4x4 UCesiumGltfGaussianSplatComponent::GetMatrix() const {
  const FTransform& transform = this->GetComponentTransform();
  FMatrix matrix = transform.ToMatrixWithScale();
  return (glm::mat4x4)VecMath::createMatrix4D(matrix);
}

void UCesiumGltfGaussianSplatComponent::RegisterWithSubsystem() {
  UCesiumGaussianSplatSubsystem* pSplatSubsystem =
      UCesiumGaussianSplatSubsystem::Get();
  ensure(pSplatSubsystem);
  pSplatSubsystem->RegisterSplat(this);
}

void UCesiumGltfGaussianSplatComponent::BeginDestroy() {
  Super::BeginDestroy();

  UCesiumGaussianSplatSubsystem* pSplatSubsystem =
      UCesiumGaussianSplatSubsystem::Get();
  if (IsValid(pSplatSubsystem)) {
    pSplatSubsystem->UnregisterSplat(this);
  }
}

FCesiumGltfGaussianSplatData::FCesiumGltfGaussianSplatData(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& meshPrimitive) {
  const int32 numShCoeffs = countShCoeffsOnPrimitive(meshPrimitive);
  this->NumCoefficients = numShCoeffs;

  const std::unordered_map<std::string, int32_t>::const_iterator positionIt =
      meshPrimitive.attributes.find("POSITION");
  if (positionIt == meshPrimitive.attributes.end()) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Mesh primitive has no 'POSITION' attribute"));
    return;
  }

  CesiumGltf::AccessorView<glm::vec3> positionView(model, positionIt->second);
  if (positionView.status() != CesiumGltf::AccessorViewStatus::Valid) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "'POSITION' accessor view on mesh primitive returned invalid status: %d"),
        positionView.status());
    return;
  }

  this->Positions.SetNum(positionView.size() * 4, EAllowShrinking::Yes);
  this->NumSplats = positionView.size();

  TOptional<FBox> Bounds;
  for (int32 i = 0; i < positionView.size(); i++) {
    FVector Position(
        positionView[i].x * CesiumPrimitiveData::positionScaleFactor,
        positionView[i].y * -CesiumPrimitiveData::positionScaleFactor,
        positionView[i].z * CesiumPrimitiveData::positionScaleFactor);
    this->Positions[i * 4] = Position.X;
    this->Positions[i * 4 + 1] = Position.Y;
    this->Positions[i * 4 + 2] = Position.Z;
    // We need a W component because Unreal can only upload float2s or float4s
    // to the GPU, not float3s...
    this->Positions[i * 4 + 3] = 0.0;

    // Take this opportunity to update the bounds.
    if (Bounds) {
      Bounds->Min = FVector(
          std::min(Bounds->Min.X, Position.X),
          std::min(Bounds->Min.Y, Position.Y),
          std::min(Bounds->Min.Z, Position.Z));
      Bounds->Max = FVector(
          std::max(Bounds->Max.X, Position.X),
          std::max(Bounds->Max.Y, Position.Y),
          std::max(Bounds->Max.Z, Position.Z));
    } else {
      Bounds = FBox();
      Bounds->Min = FVector(Position.X, Position.Y, Position.Z);
      Bounds->Max = Bounds->Min;
    }
  }

  this->Bounds = Bounds.Get(FBox());

  const std::unordered_map<std::string, int32_t>::const_iterator scaleIt =
      meshPrimitive.attributes.find("KHR_gaussian_splatting:SCALE");
  if (scaleIt == meshPrimitive.attributes.end()) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Mesh primitive has no 'KHR_gaussian_splatting:SCALE' attribute"));
    return;
  }

  const CesiumGltf::Accessor& scaleAccessor = model.accessors[scaleIt->second];
  if (scaleAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::UNSIGNED_BYTE) {
    CesiumGltf::AccessorView<glm::u8vec3> scaleView(model, scaleIt->second);
    if (scaleView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "'KHR_gaussian_splatting:SCALE' accessor view on mesh primitive returned invalid status: %d"),
          scaleView.status());
      return;
    }

    this->Scales.SetNum(scaleView.size() * 4, EAllowShrinking::Yes);
    writeConvertedAccessor3<glm::u8vec3, uint8>(
        scaleView,
        this->Scales,
        4,
        [](glm::vec3 vec) {
          return glm::exp(vec) *
                 glm::vec3(CesiumPrimitiveData::positionScaleFactor);
        });
  } else if (
      scaleAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::UNSIGNED_SHORT) {
    CesiumGltf::AccessorView<glm::u16vec3> scaleView(model, scaleIt->second);
    if (scaleView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "'KHR_gaussian_splatting:SCALE' accessor view on mesh primitive returned invalid status: %d"),
          scaleView.status());
      return;
    }

    this->Scales.SetNum(scaleView.size() * 4, EAllowShrinking::Yes);
    writeConvertedAccessor3<glm::u16vec3, uint16>(
        scaleView,
        this->Scales,
        4,
        [](glm::vec3 vec) {
          return glm::exp(vec) *
                 glm::vec3(CesiumPrimitiveData::positionScaleFactor);
        });
  } else if (
      scaleAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::FLOAT) {
    CesiumGltf::AccessorView<glm::vec3> scaleView(model, scaleIt->second);
    if (scaleView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "'KHR_gaussian_splatting:SCALE' accessor view on mesh primitive returned invalid status: %d"),
          scaleView.status());
      return;
    }

    this->Scales.SetNum(scaleView.size() * 4, EAllowShrinking::Yes);
    for (int32 i = 0; i < scaleView.size(); i++) {
      glm::vec3 scale = glm::exp(scaleView[i]) *
                        glm::vec3(CesiumPrimitiveData::positionScaleFactor);
      this->Scales[i * 4] = scale.x;
      this->Scales[i * 4 + 1] = scale.y;
      this->Scales[i * 4 + 2] = scale.z;
      this->Scales[i * 4 + 3] = 0.0f;
    }
  } else {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Mesh primitive has 'KHR_gaussian_splatting:SCALE' attribute with unsupported component type %d"),
        scaleAccessor.componentType);
    return;
  }

  const std::unordered_map<std::string, int32_t>::const_iterator rotationIt =
      meshPrimitive.attributes.find("KHR_gaussian_splatting:ROTATION");
  if (rotationIt == meshPrimitive.attributes.end()) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Mesh primitive has no 'KHR_gaussian_splatting:ROTATION' attribute"));
    return;
  }

  const CesiumGltf::Accessor& rotationAccessor =
      model.accessors[rotationIt->second];
  if (rotationAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::BYTE) {
    CesiumGltf::AccessorView<glm::i8vec4> rotationView(
        model,
        rotationIt->second);
    if (rotationView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "'KHR_gaussian_splatting:ROTATION' accessor view on mesh primitive returned invalid status: %d"),
          rotationView.status());
      return;
    }

    this->Orientations.SetNum(rotationView.size() * 4, EAllowShrinking::Yes);
    writeConvertedAccessor4<glm::i8vec4, int8>(
        rotationView,
        this->Orientations,
        4,
        [](glm::vec4 vec) {
          return glm::normalize(vec * glm::vec4(1.f, -1.f, 1.f, 1.f));
        });
  } else if (
      rotationAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::SHORT) {
    CesiumGltf::AccessorView<glm::i16vec4> rotationView(
        model,
        rotationIt->second);
    if (rotationView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "'KHR_gaussian_splatting:ROTATION' accessor view on mesh primitive returned invalid status: %d"),
          rotationView.status());
      return;
    }

    this->Orientations.SetNum(rotationView.size() * 4, EAllowShrinking::Yes);
    writeConvertedAccessor4<glm::i16vec4, int16>(
        rotationView,
        this->Orientations,
        4,
        [](glm::vec4 vec) {
          return glm::normalize(vec * glm::vec4(1.f, -1.f, 1.f, 1.f));
        });
  } else if (
      rotationAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::FLOAT) {
    CesiumGltf::AccessorView<glm::vec4> rotationView(model, rotationIt->second);
    if (rotationView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "'KHR_gaussian_splatting:ROTATION' accessor view on mesh primitive returned invalid status: %d"),
          rotationView.status());
      return;
    }

    this->Orientations.SetNum(rotationView.size() * 4, EAllowShrinking::Yes);
    for (int32 i = 0; i < rotationView.size(); i++) {
      glm::vec4 rot =
          glm::normalize(rotationView[i] * glm::vec4(1.f, -1.f, 1.f, 1.f));
      this->Orientations[i * 4] = rot.x;
      this->Orientations[i * 4 + 1] = rot.y;
      this->Orientations[i * 4 + 2] = rot.z;
      this->Orientations[i * 4 + 3] = rot.w;
    }
  } else {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Mesh primitive has 'KHR_gaussian_splatting:ROTATION' attribute with unsupported component type %d"),
        rotationAccessor.componentType);
    return;
  }

  const std::unordered_map<std::string, int32_t>::const_iterator colorIt =
      meshPrimitive.attributes.find("COLOR_0");
  if (colorIt == meshPrimitive.attributes.end()) {
    UE_LOG(LogCesium, Error, TEXT("Mesh primitive has no 'COLOR_0' attribute"));
    return;
  }

  if (colorIt->second < 0 || colorIt->second >= model.accessors.size()) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Mesh primitive has invalid 'COLOR_0' accessor index %d"),
        colorIt->second);
    return;
  }

  const CesiumGltf::Accessor& colorAccessor = model.accessors[colorIt->second];
  if (colorAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::UNSIGNED_BYTE) {
    CesiumGltf::AccessorView<glm::u8vec4> accessorView(model, colorIt->second);
    if (accessorView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "'COLOR_0' accessor view on mesh primitive returned invalid status: %d"),
          accessorView.status());
      return;
    }

    this->Colors.SetNum(accessorView.size() * 4, EAllowShrinking::Yes);
    writeConvertedAccessor4<glm::u8vec4, uint8>(
        accessorView,
        this->Colors,
        4,
        [](glm::vec4 vec) { return vec; });
  } else if (
      colorAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::UNSIGNED_SHORT) {
    CesiumGltf::AccessorView<glm::u16vec4> accessorView(model, colorIt->second);
    if (accessorView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "'COLOR_0' accessor view on mesh primitive returned invalid status: %d"),
          accessorView.status());
      return;
    }

    this->Colors.SetNum(accessorView.size() * 4, EAllowShrinking::Yes);
    writeConvertedAccessor4<glm::u16vec4, uint16>(
        accessorView,
        this->Colors,
        4,
        [](glm::vec4 vec) { return vec; });
  } else if (
      colorAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::FLOAT) {
    CesiumGltf::AccessorView<glm::vec4> accessorView(model, colorIt->second);
    if (accessorView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "'COLOR_0' accessor view on mesh primitive returned invalid status: %d"),
          accessorView.status());
      return;
    }

    this->Colors.SetNum(accessorView.size() * 4, EAllowShrinking::Yes);
    for (int32 i = 0; i < accessorView.size(); i++) {
      this->Colors[i * 4] = accessorView[i].x;
      this->Colors[i * 4 + 1] = accessorView[i].y;
      this->Colors[i * 4 + 2] = accessorView[i].z;
      this->Colors[i * 4 + 3] = accessorView[i].w;
    }
  } else {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Invalid 'COLOR_0' componentType. Allowed values are UNSIGNED_BYTE, UNSIGNED_SHORT, and FLOAT."));
    return;
  }

  const int32 shStride = numShCoeffs * 4;
  this->SphericalHarmonics.SetNum(
      shStride * this->NumSplats,
      EAllowShrinking::Yes);
  if (numShCoeffs >= 3) {
    if (!writeShCoeffs(
            model,
            meshPrimitive,
            this->SphericalHarmonics,
            shStride,
            0,
            1)) {
      return;
    }
  }

  if (numShCoeffs >= 8) {
    if (!writeShCoeffs(
            model,
            meshPrimitive,
            this->SphericalHarmonics,
            shStride,
            3 * 4,
            2)) {
      return;
    }
  }

  if (numShCoeffs >= 15) {
    if (!writeShCoeffs(
            model,
            meshPrimitive,
            this->SphericalHarmonics,
            shStride,
            5 * 4 + 3 * 4,
            3)) {
      return;
    }
  }
}

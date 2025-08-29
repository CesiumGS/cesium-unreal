// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumRuntime.h"

#include "CesiumGltfGaussianSplatComponent.h"
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/MeshPrimitive.h>

#include <fmt/format.h>

namespace {
int32 countShCoeffsOnPrimitive(CesiumGltf::MeshPrimitive& primitive) {
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
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& meshPrimitive,
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
      data[j * stride + offset + i * 3] = accessorView[j].x;
      data[j * stride + offset + i * 3 + 1] = accessorView[j].y;
      data[j * stride + offset + i * 3 + 2] = accessorView[j].z;
    }
  }

  return true;
}

template <typename T, typename ComponentT>
void writeConvertedAccessor(
    CesiumGltf::AccessorView<T>& accessorView,
    TArray<float>& data,
    int32 stride,
    int32 offset) {
  for (int32 i = 0; i < accessorView.size(); i++) {
    data[i * stride + offset] =
        accessorView[i].x /
        static_cast<float>(TNumericLimits<ComponentT>::Max());
    data[i * stride + offset + 1] =
        accessorView[i].y /
        static_cast<float>(TNumericLimits<ComponentT>::Max());
    data[i * stride + offset + 2] =
        accessorView[i].z /
        static_cast<float>(TNumericLimits<ComponentT>::Max());
    data[i * stride + offset + 3] =
        accessorView[i].w /
        static_cast<float>(TNumericLimits<ComponentT>::Max());
  }
}
} // namespace

// Sets default values for this component's properties
UCesiumGltfGaussianSplatComponent::UCesiumGltfGaussianSplatComponent()
    : UsesAdditiveRefinement(false),
      GeometricError(0),
      Dimensions(glm::vec3(0)) {}

UCesiumGltfGaussianSplatComponent::~UCesiumGltfGaussianSplatComponent() {}

void UCesiumGltfGaussianSplatComponent::SetData(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& meshPrimitive) {
  const int32 numShCoeffs = countShCoeffsOnPrimitive(meshPrimitive);
  const int32 stride = 14 + numShCoeffs * 3;

  const std::unordered_map<std::string, int32_t>::const_iterator positionIt =
      meshPrimitive.attributes.find("POSITION");
  if (positionIt == meshPrimitive.attributes.end()) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("Mesh primitive has no 'POSITION' attribute"));
    return;
  }

  CesiumGltf::AccessorView<glm::vec3> positionView(model, positionIt->second);
  if (positionView.status() != CesiumGltf::AccessorViewStatus::Valid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "'POSITION' accessor view on mesh primitive returned invalid status: %d"),
        positionView.status());
    return;
  }

  this->Data.SetNum(stride * positionView.size(), true);
  for (int32 i = 0; i < positionView.size(); i++) {
    this->Data[i * stride] = positionView[i].x;
    this->Data[i * stride + 1] = positionView[i].y;
    this->Data[i * stride + 2] = positionView[i].z;
  }

  const std::unordered_map<std::string, int32_t>::const_iterator scaleIt =
      meshPrimitive.attributes.find("KHR_gaussian_splatting:SCALE");
  if (scaleIt == meshPrimitive.attributes.end()) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("Mesh primitive has no 'KHR_gaussian_splatting:SCALE' attribute"));
    return;
  }

  CesiumGltf::AccessorView<glm::vec3> scaleView(model, scaleIt->second);
  if (scaleView.status() != CesiumGltf::AccessorViewStatus::Valid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "'KHR_gaussian_splatting:SCALE' accessor view on mesh primitive returned invalid status: %d"),
        scaleView.status());
    return;
  }

  for (int32 i = 0; i < scaleView.size(); i++) {
    this->Data[i * stride + 3] = scaleView[i].x;
    this->Data[i * stride + 4] = scaleView[i].y;
    this->Data[i * stride + 5] = scaleView[i].z;
  }

  const std::unordered_map<std::string, int32_t>::const_iterator rotationIt =
      meshPrimitive.attributes.find("KHR_gaussian_splatting:ROTATION");
  if (rotationIt == meshPrimitive.attributes.end()) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Mesh primitive has no 'KHR_gaussian_splatting:ROTATION' attribute"));
    return;
  }

  CesiumGltf::AccessorView<glm::vec4> rotationView(model, rotationIt->second);
  if (rotationView.status() != CesiumGltf::AccessorViewStatus::Valid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "'KHR_gaussian_splatting:ROTATION' accessor view on mesh primitive returned invalid status: %d"),
        rotationView.status());
    return;
  }

  for (int32 i = 0; i < rotationView.size(); i++) {
    this->Data[i * stride + 6] = rotationView[i].x;
    this->Data[i * stride + 7] = rotationView[i].y;
    this->Data[i * stride + 8] = rotationView[i].z;
    this->Data[i * stride + 9] = rotationView[i].w;
  }

  const std::unordered_map<std::string, int32_t>::const_iterator colorIt =
      meshPrimitive.attributes.find("COLOR_0");
  if (colorIt == meshPrimitive.attributes.end()) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("Mesh primitive has no 'COLOR_0' attribute"));
    return;
  }

  if (colorIt->second < 0 || colorIt->second >= model.accessors.size()) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("Mesh primitive has invalid 'COLOR_0' accessor index %d"),
        colorIt->second);
    return;
  }

  CesiumGltf::Accessor& colorAccessor = model.accessors[colorIt->second];
  if (colorAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::UNSIGNED_BYTE) {
    CesiumGltf::AccessorView<glm::u8vec4> accessorView(model, colorIt->second);
    if (accessorView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "'COLOR_0' accessor view on mesh primitive returned invalid status: %d"),
          accessorView.status());
      return;
    }
    writeConvertedAccessor<glm::u8vec4, uint8>(
        accessorView,
        this->Data,
        stride,
        10);
  } else if (
      colorAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::UNSIGNED_SHORT) {
    CesiumGltf::AccessorView<glm::u16vec4> accessorView(model, colorIt->second);
    if (accessorView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "'COLOR_0' accessor view on mesh primitive returned invalid status: %d"),
          accessorView.status());
      return;
    }
    writeConvertedAccessor<glm::u16vec4, uint16>(
        accessorView,
        this->Data,
        stride,
        10);
  } else if (
      colorAccessor.componentType ==
      CesiumGltf::AccessorSpec::ComponentType::FLOAT) {
    CesiumGltf::AccessorView<glm::vec4> accessorView(model, colorIt->second);
    if (accessorView.status() != CesiumGltf::AccessorViewStatus::Valid) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "'COLOR_0' accessor view on mesh primitive returned invalid status: %d"),
          accessorView.status());
      return;
    }

    for (int32 i = 0; i < accessorView.size(); i++) {
      this->Data[i * stride + 10] = accessorView[i].x;
      this->Data[i * stride + 11] = accessorView[i].y;
      this->Data[i * stride + 12] = accessorView[i].z;
      this->Data[i * stride + 13] = accessorView[i].w;
    }
  } else {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Invalid 'COLOR_0' componentType. Allowed values are UNSIGNED_BYTE, UNSIGNED_SHORT, and FLOAT."));
    return;
  }

  if (numShCoeffs >= 3) {
    if (!writeShCoeffs(model, meshPrimitive, this->Data, stride, 14, 1)) {
      return;
    }
  }

  if (numShCoeffs >= 8) {
    if (!writeShCoeffs(
            model,
            meshPrimitive,
            this->Data,
            stride,
            14 + 3 * 3,
            2)) {
      return;
    }
  }

  if (numShCoeffs >= 15) {
    if (!writeShCoeffs(
            model,
            meshPrimitive,
            this->Data,
            stride,
            14 + 5 * 3 + 3 * 3,
            3)) {
      return;
    }
  }
}

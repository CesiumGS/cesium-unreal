#pragma once

#include "CesiumFeatureIdSpecUtility.h"

using namespace CesiumGltf;

int32_t AddBufferToPrimitive(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::string& type,
    const int32_t componentType,
    const std::vector<std::byte>&& values) {
  CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
  buffer.byteLength = values.size();
  buffer.cesium.data = std::move(values);

  CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
  bufferView.buffer = static_cast<int32_t>(model.buffers.size() - 1);
  bufferView.byteLength = buffer.byteLength;
  bufferView.byteOffset = 0;

  CesiumGltf::Accessor& accessor = model.accessors.emplace_back();
  accessor.bufferView = static_cast<int32_t>(model.bufferViews.size() - 1);
  accessor.type = type;
  accessor.componentType = componentType;

  const int64_t elementByteSize =
      Accessor::computeByteSizeOfComponent(componentType) *
      Accessor::computeNumberOfComponents(type);
  accessor.count = buffer.byteLength / elementByteSize;

  return static_cast<int32_t>(model.accessors.size() - 1);
}

void CreateAttributeForPrimitive(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::string& attributeName,
    const std::string& type,
    const int32_t componentType,
    const std::vector<std::byte>&& values) {
  const int32_t accessor = AddBufferToPrimitive(
      model,
      primitive,
      type,
      componentType,
      std::move(values));
  primitive.attributes.insert({attributeName, accessor});
}

void CreateIndicesForPrimitive(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::string& type,
    const int32_t componentType,
    const std::vector<uint8_t>& indices) {
  std::vector<std::byte> values(indices.size());
  std::memcpy(values.data(), indices.data(), values.size());

  const int32_t accessor = AddBufferToPrimitive(
      model,
      primitive,
      type,
      componentType,
      std::move(values));
  primitive.indices = accessor;
}

FeatureId& AddFeatureIDsAsAttributeToModel(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::vector<uint8_t>& featureIDs,
    const int64_t featureCount,
    const int64_t attributeIndex) {
  std::vector<std::byte> values(featureIDs.size() * sizeof(uint8_t));
  std::memcpy(values.data(), featureIDs.data(), values.size());

  CreateAttributeForPrimitive(
      model,
      primitive,
      "_FEATURE_ID_" + std::to_string(attributeIndex),
      CesiumGltf::AccessorSpec::Type::SCALAR,
      CesiumGltf::AccessorSpec::ComponentType::UNSIGNED_BYTE,
      std::move(values));

  ExtensionExtMeshFeatures* pExtension =
      primitive.getExtension<ExtensionExtMeshFeatures>();
  if (pExtension == nullptr) {
    pExtension = &primitive.addExtension<ExtensionExtMeshFeatures>();
  }

  FeatureId& featureID = pExtension->featureIds.emplace_back();
  featureID.featureCount = featureCount;
  featureID.attribute = attributeIndex;

  return featureID;
}

FeatureId& AddFeatureIDsAsTextureToModel(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::vector<uint8_t>& featureIDs,
    const int64_t featureCount,
    const int32_t imageWidth,
    const int32_t imageHeight,
    const std::vector<glm::vec2>& texCoords,
    const int64_t texcoordSetIndex) {
  CesiumGltf::Image& image = model.images.emplace_back();
  image.cesium.bytesPerChannel = 1;
  image.cesium.channels = 1;
  image.cesium.width = imageWidth;
  image.cesium.height = imageHeight;

  std::vector<std::byte>& data = image.cesium.pixelData;
  data.resize(imageWidth * imageHeight);
  std::memcpy(data.data(), featureIDs.data(), data.size());

  model.samplers.emplace_back();

  CesiumGltf::Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  std::vector<std::byte> values(texCoords.size() * sizeof(glm::vec2));
  std::memcpy(values.data(), texCoords.data(), values.size());
  CreateAttributeForPrimitive(
      model,
      primitive,
      "TEXCOORD_" + std::to_string(texcoordSetIndex),
      CesiumGltf::AccessorSpec::Type::VEC2,
      CesiumGltf::AccessorSpec::ComponentType::FLOAT,
      std::move(values));

  ExtensionExtMeshFeatures* pExtension =
      primitive.getExtension<ExtensionExtMeshFeatures>();
  if (pExtension == nullptr) {
    pExtension = &primitive.addExtension<ExtensionExtMeshFeatures>();
  }

  FeatureId& featureID = pExtension->featureIds.emplace_back();
  featureID.featureCount = featureCount;

  FeatureIdTexture featureIDTexture;
  featureIDTexture.channels = {0};
  featureIDTexture.index = 0;
  featureIDTexture.texCoord = texcoordSetIndex;

  featureID.texture = featureIDTexture;

  return featureID;
}
#pragma once

#include "CesiumGltf/Model.h"
#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include <glm/glm.hpp>
#include <vector>

void CreateAttributeForPrimitive(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::string& attributeName,
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
  accessor.count =
      buffer.byteLength / Accessor::computeByteSizeOfComponent(componentType);

  primitive.attributes.insert(
      {attributeName, static_cast<int32_t>(model.accessors.size() - 1)});
}

ExtensionExtMeshFeaturesFeatureId& AddFeatureIDsAsAttributeToModel(
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

  ExtensionExtMeshFeatures* pExtension;
  if (primitive.hasExtension<ExtensionExtMeshFeatures>()) {
    pExtension = primitive.getExtension<ExtensionExtMeshFeatures>();
  } else {
    pExtension = &primitive.addExtension<ExtensionExtMeshFeatures>();
  }

  ExtensionExtMeshFeaturesFeatureId& featureID =
      pExtension->featureIds.emplace_back();
  featureID.featureCount = featureCount;
  featureID.attribute = attributeIndex;

  return featureID;
}

ExtensionExtMeshFeaturesFeatureId& AddFeatureIDsAsTextureToModel(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::vector<uint8_t>& featureIDs,
    const int64_t featureCount,
    const int32_t imageWidth,
    const int32_t imageHeight,
    const std::vector<glm::vec2>& texCoords) {
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
      "TEXCOORD_0",
      CesiumGltf::AccessorSpec::Type::VEC2,
      CesiumGltf::AccessorSpec::ComponentType::FLOAT,
      std::move(values));

  ExtensionExtMeshFeatures* pExtension;
  if (primitive.hasExtension<ExtensionExtMeshFeatures>()) {
    pExtension = primitive.getExtension<ExtensionExtMeshFeatures>();
  } else {
    pExtension = &primitive.addExtension<ExtensionExtMeshFeatures>();
  }

  ExtensionExtMeshFeaturesFeatureId& featureID =
      pExtension->featureIds.emplace_back();
  featureID.featureCount = featureCount;

  ExtensionExtMeshFeaturesFeatureIdTexture featureIDTexture;
  featureIDTexture.channels = {0};
  featureIDTexture.index = 0;
  featureIDTexture.texCoord = 0;

  featureID.texture = featureIDTexture;

  return featureID;
}

#include "CesiumGltfSpecUtility.h"

using namespace CesiumGltf;

int32_t AddBufferToModel(
    CesiumGltf::Model& model,
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

CesiumGltf::FeatureId& AddFeatureIDsAsAttributeToModel(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::vector<uint8_t>& featureIDs,
    const int64_t featureCount,
    const int64_t setIndex) {
  std::vector<std::byte> values = GetValuesAsBytes(featureIDs);

  CreateAttributeForPrimitive(
      model,
      primitive,
      "_FEATURE_ID_" + std::to_string(setIndex),
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
  featureID.attribute = setIndex;

  return featureID;
}

CesiumGltf::FeatureId& AddFeatureIDsAsTextureToModel(
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

  Sampler& sampler = model.samplers.emplace_back();
  sampler.wrapS = Sampler::WrapS::CLAMP_TO_EDGE;
  sampler.wrapT = Sampler::WrapT::CLAMP_TO_EDGE;

  CesiumGltf::Texture& texture = model.textures.emplace_back();
  texture.sampler = 0;
  texture.source = 0;

  std::vector<std::byte> values = GetValuesAsBytes(texCoords);
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

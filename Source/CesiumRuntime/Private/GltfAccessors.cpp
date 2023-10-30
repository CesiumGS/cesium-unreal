#include "GltfAccessors.h"

CesiumTexCoordAccessorType GetTexCoordAccessorView(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    int32 textureCoordinateSetIndex) {
  const std::string texCoordName =
      "TEXCOORD_" + std::to_string(textureCoordinateSetIndex);
  auto texCoord = primitive.attributes.find(texCoordName);
  if (texCoord == primitive.attributes.end()) {
    return CesiumTexCoordAccessorType();
  }

  const CesiumGltf::Accessor* pAccessor = model.getSafe<CesiumGltf::Accessor>(
      &model.accessors,
      primitive.attributes.at(texCoordName));
  if (!pAccessor || pAccessor->type != CesiumGltf::Accessor::Type::VEC2) {
    return CesiumTexCoordAccessorType();
  }

  switch (pAccessor->componentType) {
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
    if (pAccessor->normalized) {
      // Unsigned byte texcoords must be normalized.
      return CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<uint8_t>>(
          model,
          *pAccessor);
    }
    [[fallthrough]];
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
    if (pAccessor->normalized) {
      // Unsigned short texcoords must be normalized.
      return CesiumGltf::AccessorView<
          CesiumGltf::AccessorTypes::VEC2<uint16_t>>(model, *pAccessor);
    }
    [[fallthrough]];
  case CesiumGltf::Accessor::ComponentType::FLOAT:
    return CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
        model,
        *pAccessor);
  default:
    return CesiumTexCoordAccessorType();
  }
}

#pragma once

#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltf/Model.h"
#include <glm/glm.hpp>
#include <vector>

/**
 * @brief Adds the buffer to the given primitive, creating a buffer view and
 * accessor in the process.
 * @returns The index of the accessor.
 */
int32_t AddBufferToPrimitive(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::string& type,
    const int32_t componentType,
    const std::vector<std::byte>&& values);

/**
 * @brief Creates an attribute on the given primitive, including a buffer,
 * buffer view, and accessor for the given values.
 */
void CreateAttributeForPrimitive(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::string& attributeName,
    const std::string& type,
    const int32_t componentType,
    const std::vector<std::byte>&& values);

/**
 * @brief Creates indices for the given primitive, including a buffer,
 * buffer view, and accessor for the given values.
 */
void CreateIndicesForPrimitive(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::string& type,
    const int32_t componentType,
    const std::vector<uint8_t>& indices);

/**
 * @brief Adds the feature IDs to the given primitive as a feature ID attribute
 * in EXT_mesh_features. If the primitive doesn't already contain
 * EXT_mesh_features, this function adds it.
 *
 * @returns The newly created feature ID in the primitive extension.
 */
FeatureId& AddFeatureIDsAsAttributeToModel(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::vector<uint8_t>& featureIDs,
    const int64_t featureCount,
    const int64_t attributeIndex);

/**
 * @brief Adds the feature IDs to the given primitive as a feature ID texture
 * in EXT_mesh_features. This also adds the given texcoords to the primitive as
 * a TEXCOORD attribute. If the primitive doesn't already contain
 * EXT_mesh_features, this function adds it.
 *
 * @returns The newly created feature ID in the primitive extension.
 */
FeatureId& AddFeatureIDsAsTextureToModel(
    CesiumGltf::Model& model,
    CesiumGltf::MeshPrimitive& primitive,
    const std::vector<uint8_t>& featureIDs,
    const int64_t featureCount,
    const int32_t imageWidth,
    const int32_t imageHeight,
    const std::vector<glm::vec2>& texCoords,
    const int64_t texcoordSetIndex);

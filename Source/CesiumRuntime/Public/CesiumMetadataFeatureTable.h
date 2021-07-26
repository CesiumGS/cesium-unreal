// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/AccessorView.h"
#include "CesiumMetadataGenericValue.h"
#include "CesiumMetadataProperty.h"
#include "CesiumMetadataFeatureTable.generated.h"

namespace CesiumGltf {
struct Model;
struct Accessor;
struct FeatureTable;
} // namespace CesiumGltf

/**
 * Provide a wrapper for a metadata feature table. Feature table is a
 * collection of properties and stores the value as a struct of array.
 * This struct is provided to make it work with blueprint.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataFeatureTable {
  GENERATED_USTRUCT_BODY()

  using AccesorViewType = std::variant<
      std::monostate,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int8_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint8_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int16_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint16_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint32_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<float>>>;

public:
  /**
   * Construct an empty feature table.
   */
  FCesiumMetadataFeatureTable(){};

  /**
   * Construct a feature table from a Gltf Feature Table.
   *
   * @param model The model that stores EXT_feature_metadata
   * @param accessor The accessor of feature ID
   * @param featureTable The feature table that is paired with feature ID
   */
  FCesiumMetadataFeatureTable(
      const CesiumGltf::Model& model,
      const CesiumGltf::Accessor& accessor,
      const CesiumGltf::FeatureTable& featureTable);

  /**
   * Query the number of features in the feature table.
   *
   * @return The number of features in the feature table
   */
  size_t GetNumOfFeatures() const;

  /**
   * Query the feature ID based on a vertex.
   *
   * @return The feature ID of the vertex
   */
  int64 GetFeatureIDForVertex(uint32 vertexIdx) const;

  /**
   * Return the map of a feature that maps feature's property name to value.
   *
   * @return The property map of a feature
   */
  TMap<FString, FCesiumMetadataGenericValue>
  GetValuesForFeatureID(size_t featureID) const;

  /**
   * Return the map of a feature that maps feature's property name to value as
   * string.
   *
   * @return The property map of a feature
   */
  TMap<FString, FString> GetValuesAsStringsForFeatureID(size_t featureID) const;

  /**
   * Get all the properties of a feature table.
   *
   * @return All the properties of a feature table
   */
  const TMap<FString, FCesiumMetadataProperty>& GetProperties() const;

private:
  AccesorViewType _featureIDAccessor;
  TMap<FString, FCesiumMetadataProperty> _properties;
};

// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Misc/Optional.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataEnum.generated.h"

namespace CesiumGltf {
struct Enum;
struct Schema;
struct Model;
} // namespace CesiumGltf

/**
 * @brief Stores information on the values of an enum and the corresponding
 * names of those values.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataEnum {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * @brief Constructs an empty enum definition with no values.
   */
  FCesiumMetadataEnum() : _valueNames() {}
  /**
   * @brief Constructs an enum definition from a @ref CesiumGltf::Enum.
   */
  FCesiumMetadataEnum(const CesiumGltf::Enum& Enum);
  /**
   * @brief Constructs an enum definition from an Unreal UENUM type.
   *
   * This can be useful when creating @ref FCesiumMetadataValue items directly
   * from code, to avoid having to create a @ref CesiumGltf::Enum from scratch
   * to create this enum definition from. It can also be useful for testing
   * purposes.
   *
   * To call this method with an Unreal UENUM from C++, you can use the
   * `StaticEnum` method like so:
   * ```
   * FCesiumMetadataEnum enumDefinition(StaticEnum<EExampleEnumType>());
   * ```
   */
  FCesiumMetadataEnum(const UEnum* UnrealEnum);

  /**
   * @brief Attempts to return the name of a given value from this enum
   * definition, returning the name if the value was found or an empty optional
   * if not.
   *
   * @param Value The value to lookup in this enum definition.
   */
  TOptional<FString> GetName(int64_t Value) const;

private:
  TMap<int64_t, FString> _valueNames;
};

/**
 * @brief Contains a set of enum definitions obtained from a @ref
 * CesiumGltf::Schema.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumMetadataEnumCollection {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * @brief Creates a new @ref FCesiumMetadataEnumCollection without any enum
   * definitions.
   */
  FCesiumMetadataEnumCollection() : _enumDefinitions() {}
  /**
   * @brief Creates a new @ref FCesiumMetadataEnumCollection with definitions
   * obtained from the `enums` property of the given @ref CesiumGltf::Schema.
   *
   * In most cases, @ref FCesiumMetadataEnumCollection::GetOrCreateFromSchema
   * should be used instead.
   */
  FCesiumMetadataEnumCollection(const CesiumGltf::Schema& Schema);

  /**
   * @brief Obtains the @ref FCesiumMetadataEnum corresponding to the given key.
   *
   * The input key should be one of the keys in the `enums` of the @ref
   * CesiumGltf::Schema that this collection is based on.
   */
  TSharedPtr<FCesiumMetadataEnum> Get(const FString& InKey) const;

  /**
   * @brief Attempts to obtain a @ref FCesiumMetadataEnumCollection from the
   * given schema if one has already been created, or creates one if not.
   *
   * This method utilizes a custom extension on the @ref CesiumGltf::Schema to
   * prevent duplicate enum collections from being created for the same schema,
   * avoiding unnecessary allocations.
   *
   * @param Schema The schema to create an enum collection from.
   */
  static TSharedRef<FCesiumMetadataEnumCollection>
  GetOrCreateFromSchema(CesiumGltf::Schema& Schema);

  /**
   * @brief Attempts to obtain a @ref FCesiumMetadataEnumCollection from a
   * schema attached to the given model, or returns an invalid `TSharedPtr` if
   * no schema is present.
   *
   * This method is equivalent to looking up the schema from the model yourself
   * and calling @ref FCesiumMetadataEnumCollection::GetOrCreateFromSchema.
   *
   * @param Model The model to lookup the schema from.
   */
  static TSharedPtr<FCesiumMetadataEnumCollection>
  GetOrCreateFromModel(const CesiumGltf::Model& Model);

private:
  TMap<FString, TSharedRef<FCesiumMetadataEnum>> _enumDefinitions;
};

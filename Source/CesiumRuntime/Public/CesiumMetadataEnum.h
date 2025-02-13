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
 * A Blueprint-accessible wrapper for a glTF structural metadata enum
 * definition.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataEnum {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumMetadataEnum() : _name(), _valueNames() {}
  FCesiumMetadataEnum(const CesiumGltf::Enum& Enum);
  FCesiumMetadataEnum(const UEnum* UnrealEnum);

  TOptional<FString> GetName(int64_t Value) const;

private:
  FString _name;
  TMap<int64_t, FString> _valueNames;
};

USTRUCT()
struct CESIUMRUNTIME_API FCesiumMetadataEnumCollection {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumMetadataEnumCollection() : _enumDefinitions() {}
  FCesiumMetadataEnumCollection(const CesiumGltf::Schema& Schema);

  TSharedPtr<FCesiumMetadataEnum> Get(const FString& InName) const;

  static TSharedRef<FCesiumMetadataEnumCollection>
  GetOrCreateFromSchema(CesiumGltf::Schema& Schema);
  static TSharedPtr<FCesiumMetadataEnumCollection>
  GetOrCreateFromModel(const CesiumGltf::Model& Model);

private:
  TMap<FString, TSharedRef<FCesiumMetadataEnum>> _enumDefinitions;
};

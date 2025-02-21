// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumMetadataEnum.h"

#include "Containers/UnrealString.h"
#include "Misc/Optional.h"
#include <CesiumGltf/Enum.h>
#include <CesiumGltf/EnumValue.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/Schema.h>

struct ExtensionUnrealMetadataEnumCollection {
  static inline constexpr const char* TypeName =
      "ExtensionUnrealMetadataEnumCollection";
  static inline constexpr const char* ExtensionName =
      "PRIVATE_MetadataEnumCollection_Unreal";

  TSharedRef<FCesiumMetadataEnumCollection> EnumCollection;

  ExtensionUnrealMetadataEnumCollection(const CesiumGltf::Schema& Schema)
      : EnumCollection(MakeShared<FCesiumMetadataEnumCollection>(Schema)) {}
};

FCesiumMetadataEnum::FCesiumMetadataEnum(const CesiumGltf::Enum& Enum)
    : _name(UTF8_TO_TCHAR(Enum.name.value_or("").c_str())) {
  this->_valueNames.Reserve(Enum.values.size());
  for (const CesiumGltf::EnumValue& enumValue : Enum.values) {
    const FString Name(UTF8_TO_TCHAR(enumValue.name.c_str()));
    this->_valueNames.Add(enumValue.value, Name);
  }
}

FCesiumMetadataEnum::FCesiumMetadataEnum(const UEnum* UnrealEnum)
    : _name(), _valueNames() {
  if (UnrealEnum != nullptr) {
    const int32 NumEntries = UnrealEnum->NumEnums();
    UnrealEnum->GetName(_name);
    _valueNames.Reserve(NumEntries);

    for (int32 i = 0; i < NumEntries; i++) {
      _valueNames.Emplace(
          UnrealEnum->GetValueByIndex(i),
          UnrealEnum->GetNameStringByIndex(i));
    }
  }
}

TOptional<FString> FCesiumMetadataEnum::GetName(int64_t Value) const {
  const FString* FoundName = this->_valueNames.Find(Value);
  if (FoundName == nullptr) {
    return {};
  }

  return *FoundName;
}

FCesiumMetadataEnumCollection::FCesiumMetadataEnumCollection(
    const CesiumGltf::Schema& Schema) {
  this->_enumDefinitions.Reserve(Schema.enums.size());
  for (const auto& [name, enumDefinition] : Schema.enums) {
    this->_enumDefinitions.Emplace(
        FString(UTF8_TO_TCHAR(name.c_str())),
        MakeShared<FCesiumMetadataEnum>(enumDefinition));
  }
}

TSharedPtr<FCesiumMetadataEnum>
FCesiumMetadataEnumCollection::Get(const FString& InName) const {
  const TSharedRef<FCesiumMetadataEnum>* MaybeDefinition =
      this->_enumDefinitions.Find(InName);
  if (MaybeDefinition == nullptr) {
    return TSharedPtr<FCesiumMetadataEnum>();
  }

  return MaybeDefinition->ToSharedPtr();
}

TSharedRef<FCesiumMetadataEnumCollection>
FCesiumMetadataEnumCollection::GetOrCreateFromSchema(
    CesiumGltf::Schema& Schema) {
  const ExtensionUnrealMetadataEnumCollection* ExtensionPtr =
      Schema.getExtension<ExtensionUnrealMetadataEnumCollection>();
  if (ExtensionPtr != nullptr) {
    return ExtensionPtr->EnumCollection;
  }

  ExtensionUnrealMetadataEnumCollection& NewExtension =
      Schema.addExtension<ExtensionUnrealMetadataEnumCollection>(Schema);
  return NewExtension.EnumCollection;
}

TSharedPtr<FCesiumMetadataEnumCollection>
FCesiumMetadataEnumCollection::GetOrCreateFromModel(
    const CesiumGltf::Model& Model) {
  const CesiumGltf::ExtensionModelExtStructuralMetadata* ExtensionPtr =
      Model.getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();
  if (ExtensionPtr == nullptr || ExtensionPtr->schema == nullptr) {
    return {};
  }

  return FCesiumMetadataEnumCollection::GetOrCreateFromSchema(
      *ExtensionPtr->schema);
}

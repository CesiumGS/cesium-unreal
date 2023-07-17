// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPropertyArrayBlueprintLibrary.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"

ECesiumMetadataBlueprintType
UCesiumPropertyArrayBlueprintLibrary::GetElementBlueprintType(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return CesiumMetadataValueTypeToBlueprintType(array._elementType);
}

FCesiumMetadataValueType
UCesiumPropertyArrayBlueprintLibrary::GetElementValueType(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return array._elementType;
}

int64 UCesiumPropertyArrayBlueprintLibrary::GetSize(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return std::visit([](const auto& view) { return view.size(); }, array._value);
}

FCesiumMetadataValue UCesiumPropertyArrayBlueprintLibrary::GetValue(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index) {
  return std::visit(
      [index](const auto& v) -> FCesiumMetadataValue {
        if (index < 0 || index >= v.size()) {
          FFrame::KismetExecutionMessage(
              *FString::Printf(
                  TEXT(
                      "Attempted to access index %d from CesiumPropertyArray of length %d!"),
                  index,
                  v.size()),
              ELogVerbosity::Warning,
              FName("CesiumPropertyArrayOutOfBoundsWarning"));
          return FCesiumMetadataValue();
        }
        return FCesiumMetadataValue(v[index]);
      },
      array._value);
}

// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumVectorData/VectorDocument.h"
#include "CesiumVectorNode.h"

#include <optional>

#include "CesiumVectorDocument.generated.h"

USTRUCT(BlueprintType)
struct FCesiumVectorDocument {
  GENERATED_BODY()

  FCesiumVectorDocument() : _document(CesiumVectorData::VectorNode{}) {}

  FCesiumVectorDocument(CesiumVectorData::VectorDocument&& document)
      : _document(std::move(document)) {}

private:
  CesiumVectorData::VectorDocument _document;

  friend class UCesiumVectorDocumentBlueprintLibrary;
};

UCLASS()
class UCesiumVectorDocumentBlueprintLibrary : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|Vector",
      meta = (DisplayName = "Load GeoJSON Document From String"))
  static UPARAM(DisplayName = "Success") bool LoadGeoJsonFromString(
      const FString& InString,
      FCesiumVectorDocument& OutVectorDocument);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector",
      meta = (DisplayName = "Get Root Node"))
  static FCesiumVectorNode
  GetRootNode(const FCesiumVectorDocument& InVectorDocument);
};

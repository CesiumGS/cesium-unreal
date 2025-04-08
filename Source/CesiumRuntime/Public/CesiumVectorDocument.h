// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumVectorData/VectorDocument.h"
#include "CesiumVectorNode.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"

#include <optional>

#include "CesiumVectorDocument.generated.h"

/**
 * @brief A vector document containing a tree of `FCesiumVectorNode` values.
 */
USTRUCT(BlueprintType)
struct FCesiumVectorDocument {
  GENERATED_BODY()

  /**
   * @brief Creates an empty `FCesiumVectorDocument`.
   */
  FCesiumVectorDocument() : _document(CesiumVectorData::VectorNode{}, {}) {}

  /**
   * @brief Creates a `FCesiumVectorDocument` wrapping the provided
   * `CesiumVectorData::VectorDocument`.
   */
  FCesiumVectorDocument(CesiumVectorData::VectorDocument&& document)
      : _document(std::move(document)) {}

private:
  CesiumVectorData::VectorDocument _document;

  friend class UCesiumVectorDocumentBlueprintLibrary;
};

/**
 * @brief A Blueprint Function Library providing functions for interacting with
 * a `FCesiumVectorDocument`.
 */
UCLASS()
class UCesiumVectorDocumentBlueprintLibrary : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Attempts to load a `FCesiumVectorDocument` from a string containing
   * GeoJSON data.
   *
   * If loading fails, this function will return false and `OutVectorDocument`
   * will be empty.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|Vector|Document",
      meta = (DisplayName = "Load GeoJSON Document From String"))
  static UPARAM(DisplayName = "Success") bool LoadGeoJsonFromString(
      const FString& InString,
      FCesiumVectorDocument& OutVectorDocument);

  /**
   * @brief Obtains the root node of the provided vector document.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Document",
      meta = (DisplayName = "Get Root Node"))
  static FCesiumVectorNode
  GetRootNode(const FCesiumVectorDocument& InVectorDocument);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCesiumVectorDocumentIonLoadDelegate,
    bool,
    Success,
    FCesiumVectorDocument,
    Document);

UCLASS()
class CESIUMRUNTIME_API UCesiumLoadVectorDocumentFromIonAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  /**
   * @brief Attempts to load a vector document from a Cesium ion asset.
   *
   * If successful, `Success` will be true and `Document` will contain the
   * loaded document.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|Vector|Document",
      meta =
          (BlueprintInternalUseOnly = true,
           DisplayName = "Load Vector Document from Cesium ion"))
  static UCesiumLoadVectorDocumentFromIonAsyncAction* LoadFromIon(
      int64 AssetId,
      const FString& IonAccessToken,
      const FString& IonAssetEndpointUrl = "https://api.cesium.com/");

  UPROPERTY(BlueprintAssignable)
  FCesiumVectorDocumentIonLoadDelegate OnLoadResult;

  virtual void Activate() override;

  int64 AssetId;
  FString IonAccessToken;
  FString IonAssetEndpointUrl;
};

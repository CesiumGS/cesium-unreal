// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoJsonObject.h"
#include "CesiumUtility/IntrusivePointer.h"
#include "CesiumVectorData/GeoJsonDocument.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SharedPointer.h"
#include "UObject/ObjectMacros.h"

#include <optional>

#include "CesiumGeoJsonDocument.generated.h"

/**
 * @brief A vector document containing a tree of `FCesiumGeoJsonObject` values.
 */
USTRUCT(BlueprintType)
struct FCesiumGeoJsonDocument {
  GENERATED_BODY()

  /**
   * @brief Creates an empty `FCesiumGeoJsonDocument`.
   */
  FCesiumGeoJsonDocument() : _document(nullptr) {}

  /**
   * @brief Creates a `FCesiumGeoJsonDocument` wrapping the provided
   * `CesiumVectorData::GeoJsonDocument`.
   */
  FCesiumGeoJsonDocument(
      CesiumUtility::IntrusivePointer<CesiumVectorData::GeoJsonDocument>&&
          document)
      : _document(std::move(document)) {}

  /**
   * @brief Checks if this FCesiumVectorDocument is valid (document is not
   * nullptr).
   */
  bool IsValid() const { return this->_document != nullptr; }

  /**
   * @brief Returns the `CesiumVectorData::VectorDocument` this wraps.
   */
  const CesiumUtility::IntrusivePointer<CesiumVectorData::GeoJsonDocument>&
  GetDocument() const {
    return this->_document;
  }

private:
  CesiumUtility::IntrusivePointer<CesiumVectorData::GeoJsonDocument> _document;

  friend class UCesiumGeoJsonDocumentBlueprintLibrary;
};

/**
 * @brief A Blueprint Function Library providing functions for interacting with
 * a `FCesiumVectorDocument`.
 */
UCLASS()
class UCesiumGeoJsonDocumentBlueprintLibrary : public UBlueprintFunctionLibrary {
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
      FCesiumGeoJsonDocument& OutVectorDocument);

  /**
   * @brief Obtains the root node of the provided vector document.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Document",
      meta = (DisplayName = "Get Root Node"))
  static FCesiumGeoJsonObject
  GetRootObject(const FCesiumGeoJsonDocument& InVectorDocument);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCesiumVectorDocumentIonLoadDelegate,
    bool,
    Success,
    FCesiumGeoJsonDocument,
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

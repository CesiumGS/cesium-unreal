// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoJsonObject.h"
#include "CesiumUtility/IntrusivePointer.h"
#include "CesiumVectorData/GeoJsonDocument.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SharedPointer.h"
#include "UObject/ObjectMacros.h"

#include <memory>
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
  FCesiumGeoJsonDocument();

  /**
   * @brief Creates a `FCesiumGeoJsonDocument` wrapping the provided
   * `CesiumVectorData::GeoJsonDocument`.
   */
  FCesiumGeoJsonDocument(
      std::shared_ptr<CesiumVectorData::GeoJsonDocument>&& document);

  /**
   * @brief Checks if this FCesiumGeoJsonDocument is valid (document is not
   * nullptr).
   */
  bool IsValid() const;

  /**
   * @brief Returns the `CesiumVectorData::GeoJsonDocument` this wraps.
   */
  const std::shared_ptr<CesiumVectorData::GeoJsonDocument>& GetDocument() const;

private:
  std::shared_ptr<CesiumVectorData::GeoJsonDocument> _pDocument;

  friend class UCesiumGeoJsonDocumentBlueprintLibrary;
};

/**
 * @brief A Blueprint Function Library providing functions for interacting with
 * a `FCesiumGeoJsonDocument`.
 */
UCLASS()
class UCesiumGeoJsonDocumentBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Attempts to load a `FCesiumGeoJsonDocument` from a string containing
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
    FCesiumVectorDocumentAsyncLoadDelegate,
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
   * If the provided `IonAccessToken` is an empty string, the
   * `DefaultIonAccessToken` from the provided `CesiumIonServer` will be used
   * instead.
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
      const UCesiumIonServer* CesiumIonServer,
      const FString& IonAccessToken);

  UPROPERTY(BlueprintAssignable)
  FCesiumVectorDocumentAsyncLoadDelegate OnLoadResult;

  virtual void Activate() override;

  int64 AssetId;
  FString IonAccessToken;

  UPROPERTY()
  const UCesiumIonServer* CesiumIonServer;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumLoadVectorDocumentFromUrlAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  /**
   * @brief Attempts to load a vector document from a URL.
   *
   * If successful, `Success` will be true and `Document` will contain the
   * loaded document.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|Vector|Document",
      meta =
          (BlueprintInternalUseOnly = true,
           DisplayName = "Load Vector Document from URL"))
  static UCesiumLoadVectorDocumentFromUrlAsyncAction*
  LoadFromUrl(const FString& Url, const TMap<FString, FString>& Headers);

  UPROPERTY(BlueprintAssignable)
  FCesiumVectorDocumentAsyncLoadDelegate OnLoadResult;

  virtual void Activate() override;

  FString Url;
  TMap<FString, FString> Headers;
};

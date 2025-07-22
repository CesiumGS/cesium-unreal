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
 * @brief A GeoJSON document containing a tree of `FCesiumGeoJsonObject` values.
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
   * If loading fails, this function will return false and `OutGeoJsonDocument`
   * will be empty.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|Vector|Document",
      meta = (DisplayName = "Load GeoJSON Document From String"))
  static UPARAM(DisplayName = "Success") bool LoadGeoJsonFromString(
      const FString& InString,
      FCesiumGeoJsonDocument& OutGeoJsonDocument);

  /**
   * @brief Obtains the root node of the provided GeoJSON document.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Document",
      meta = (DisplayName = "Get Root Node"))
  static FCesiumGeoJsonObject
  GetRootObject(const FCesiumGeoJsonDocument& InGeoJsonDocument);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCesiumGeoJsonDocumentAsyncLoadDelegate,
    bool,
    Success,
    FCesiumGeoJsonDocument,
    Document);

UCLASS()
class CESIUMRUNTIME_API UCesiumLoadGeoJsonDocumentFromIonAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  /**
   * @brief Attempts to load a GeoJSON document from a Cesium ion asset.
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
           DisplayName = "Load GeoJSON Document from Cesium ion"))
  static UCesiumLoadGeoJsonDocumentFromIonAsyncAction* LoadFromIon(
      int64 AssetId,
      const UCesiumIonServer* CesiumIonServer,
      const FString& IonAccessToken);

  UPROPERTY(BlueprintAssignable)
  FCesiumGeoJsonDocumentAsyncLoadDelegate OnLoadResult;

  virtual void Activate() override;

  int64 AssetId;
  FString IonAccessToken;

  UPROPERTY()
  const UCesiumIonServer* CesiumIonServer;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumLoadGeoJsonDocumentFromUrlAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  /**
   * @brief Attempts to load a GeoJSON document from a URL.
   *
   * If successful, `Success` will be true and `Document` will contain the
   * loaded document.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|Vector|Document",
      meta =
          (BlueprintInternalUseOnly = true,
           DisplayName = "Load GeoJSON Document from URL"))
  static UCesiumLoadGeoJsonDocumentFromUrlAsyncAction*
  LoadFromUrl(const FString& Url, const TMap<FString, FString>& Headers);

  UPROPERTY(BlueprintAssignable)
  FCesiumGeoJsonDocumentAsyncLoadDelegate OnLoadResult;

  virtual void Activate() override;

  FString Url;
  TMap<FString, FString> Headers;
};

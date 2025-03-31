// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTileset.h"
#include "CesiumGeoreference.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Math/MathFwd.h"
#include "Misc/Optional.h"
#include "Templates/SharedPointer.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

THIRD_PARTY_INCLUDES_START
#include <CesiumIonClient/Connection.h>
THIRD_PARTY_INCLUDES_END

#include <CesiumGeospatial/Cartographic.h>
#include <CesiumGeospatial/GlobeRectangle.h>
#include <CesiumIonClient/Geocoder.h>
#include <CesiumUtility/Math.h>

#include <optional>

#include "CesiumGeocoderServiceBlueprintLibrary.generated.h"

/**
 * @brief The supported providers that can be accessed through ion's geocoder
 * API.
 */
UENUM(BlueprintType)
enum class ECesiumIonGeocoderProviderType : uint8 {
  /**
   * @brief Google geocoder, for use with Google data.
   */
  Google = CesiumIonClient::GeocoderProviderType::Google,
  /**
   * @brief Bing geocoder, for use with Bing data.
   */
  Bing = CesiumIonClient::GeocoderProviderType::Bing,
  /**
   * @brief Use the default geocoder as set on the server. Used when neither
   * Bing or Google data is used.
   */
  Default = CesiumIonClient::GeocoderProviderType::Default
};

/**
 * @brief The supported types of requests to geocoding API.
 */
UENUM(BlueprintType)
enum class ECesiumIonGeocoderRequestType : uint8 {
  /**
   * @brief Perform a full search from a complete query.
   */
  Search = CesiumIonClient::GeocoderRequestType::Search,
  /**
   * @brief Perform a quick search based on partial input, such as while a user
   * is typing.
   *
   * The search results may be less accurate or exhaustive than using
   * `ECesiumIonGeocoderRequestType::Search`.
   */
  Autocomplete = CesiumIonClient::GeocoderRequestType::Autocomplete
};

/**
 * @brief Attribution information for a query to a geocoder service.
 */
USTRUCT(BlueprintType)
struct FCesiumGeocoderServiceAttribution {
  GENERATED_BODY()
public:
  FCesiumGeocoderServiceAttribution() {}
  FCesiumGeocoderServiceAttribution(
      const CesiumIonClient::GeocoderAttribution& attribution);

  /**
   * @brief An HTML string containing the necessary attribution information.
   */
  UPROPERTY(BlueprintReadOnly)
  FString Html;

  /**
   * @brief If true, the credit should be visible in the main credit container.
   * Otherwise, it can appear in a popover.
   */
  UPROPERTY(BlueprintReadOnly)
  bool bShowOnScreen;
};

/**
 * @brief A single feature (a location or region) obtained from a geocoder
 * service.
 */
UCLASS(BlueprintType)
class UCesiumGeocoderServiceFeature : public UObject {
  GENERATED_BODY()
public:
  UCesiumGeocoderServiceFeature();

  /**
   * @brief Returns a position in Longitude, Latitude, and Height representing
   * this feature.
   *
   * If the geocoder service returned a bounding box for this result, this will
   * return the center of the bounding box. If the geocoder service returned a
   * coordinate for this result, this will return the coordinate.
   */
  UFUNCTION(BlueprintPure)
  FVector GetCartographic() const;

  /**
   * @brief Returns an FBox representing this feature. `FBox::Min` will hold the
   * southwest corner while `FBox::Max` will hold the northwest corner.
   *
   * If the geocoder service returned a bounding box for this result, this will
   * return the bounding box. If the geocoder service returned a coordinate for
   * this result, this will return a zero-width rectangle at that coordinate.
   */
  UFUNCTION(BlueprintPure)
  FBox GetGlobeRectangle() const;

  /**
   * @brief The user-friendly display name of this feature.
   */
  UFUNCTION(BlueprintPure)
  FString GetDisplayName() const;

  void SetFeature(CesiumIonClient::GeocoderFeature&& feature);

private:
  std::optional<CesiumIonClient::GeocoderFeature> _feature;
};

/**
 * @brief The result of making a request to a geocoder service.
 */
UCLASS(BlueprintType)
class UCesiumGeocoderServiceResult : public UObject {
  GENERATED_BODY()
public:
  UCesiumGeocoderServiceResult() {}

  /**
   * @brief Any necessary attributions for this geocoder result.
   */
  UPROPERTY(BlueprintReadOnly)
  TArray<FCesiumGeocoderServiceAttribution> Attributions;

  /**
   * @brief The features obtained from this geocoder service, if any.
   */
  UPROPERTY(BlueprintReadOnly)
  TArray<UCesiumGeocoderServiceFeature*> Features;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCesiumGeocoderServiceDelegate,
    bool,
    Success,
    UCesiumGeocoderServiceResult*,
    Result,
    FString,
    Error);

UCLASS()
class CESIUMRUNTIME_API UCesiumGeocoderServiceIonGeocoderAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()

public:
  /**
   * @brief Queries the Cesium ion Geocoder service.
   *
   * @param IonAccessToken The access token to use for Cesium ion. This token
   * must have the `geocode` scope.
   * @param CesiumIonServer Information on the Cesium ion server to perform this
   * request against.
   * @param ProviderType The provider to obtain a geocoding result from.
   * @param RequestType The type of geocoding request to make.
   * @param Query The query string.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      meta =
          (BlueprintInternalUseOnly = true,
           DisplayName = "Query Cesium ion Geocoder"))
  static UCesiumGeocoderServiceIonGeocoderAsyncAction* Geocode(
      const FString& IonAccessToken,
      const UCesiumIonServer* CesiumIonServer,
      ECesiumIonGeocoderProviderType ProviderType,
      ECesiumIonGeocoderRequestType RequestType,
      const FString& Query);

  UPROPERTY(BlueprintAssignable)
  FCesiumGeocoderServiceDelegate OnGeocodeRequestComplete;

  virtual void Activate() override;

private:
  FString _ionAccessToken;

  UPROPERTY()
  const UCesiumIonServer* _cesiumIonServer;

  ECesiumIonGeocoderProviderType _providerType;
  ECesiumIonGeocoderRequestType _requestType;
  FString _query;
};

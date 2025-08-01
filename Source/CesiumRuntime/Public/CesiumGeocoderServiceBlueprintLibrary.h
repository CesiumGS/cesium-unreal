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
  FCesiumGeocoderServiceAttribution() = default;
  FCesiumGeocoderServiceAttribution(
      const CesiumIonClient::GeocoderAttribution& attribution);

  /**
   * @brief An HTML string containing the necessary attribution information.
   */
  UPROPERTY(BlueprintReadOnly, Category = "Cesium|Geocoder")
  FString Html;

  /**
   * @brief If true, the credit should be visible in the main credit container.
   * Otherwise, it can appear in a popover.
   */
  UPROPERTY(BlueprintReadOnly, Category = "Cesium|Geocoder")
  bool bShowOnScreen = false;
};

/**
 * @brief A single feature (a location or region) obtained from a geocoder
 * service.
 */
USTRUCT(BlueprintType)
struct FCesiumGeocoderServiceFeature {
  GENERATED_BODY()
public:
  FCesiumGeocoderServiceFeature() = default;
  FCesiumGeocoderServiceFeature(
      const CesiumIonClient::GeocoderFeature& feature);

  /**
   * @brief The position of the feature expressed as longitude in degrees (X),
   * latitude in degrees (Y), and height in meters above the ellipsoid (Z).
   *
   * Do not confuse the ellipsoid height with a geoid height or height above
   * mean sea level, which can be tens of meters higher or lower depending on
   * where in the world the object is located.
   *
   * The height may be 0.0, indicating that the geocoder did not provide a
   * height for the feature.
   *
   * If the geocoder service returned a bounding box for this result, this will
   * return the center of the bounding box. If the geocoder service returned a
   * coordinate for this result, this will return the coordinate.
   */
  UPROPERTY(
      BlueprintReadOnly,
      Category = "Cesium|Geocoder",
      meta = (AllowPrivateAccess))
  FVector LongitudeLatitudeHeight = FVector::Zero();

  /**
   * @brief The globe rectangle that bounds the feature. The box's `Min.X` is
   * the Westernmost longitude in degrees, `Min.Y` is the Southernmost latitude
   * in degrees, `Max.X` is the Easternmost longitude in degrees, and `Max.Y` is
   * the Northernmost latitude in degrees.
   *
   * If the geocoder service returned a bounding box for this result, this will
   * return the bounding box. If the geocoder service returned a coordinate for
   * this result, this will return a zero-width rectangle at that coordinate.
   */
  UPROPERTY(
      BlueprintReadOnly,
      Category = "Cesium|Geocoder",
      meta = (AllowPrivateAccess))
  FBox GlobeRectangle;

  /**
   * @brief The user-friendly display name of this feature.
   */
  UPROPERTY(
      BlueprintReadOnly,
      Category = "Cesium|Geocoder",
      meta = (AllowPrivateAccess))
  FString DisplayName;
};

/**
 * @brief The result of making a request to a geocoder service.
 */
UCLASS(BlueprintType)
class UCesiumGeocoderServiceResult : public UObject {
  GENERATED_BODY()
public:
  UCesiumGeocoderServiceResult() = default;

  /**
   * @brief Any necessary attributions for this geocoder result.
   */
  UPROPERTY(BlueprintReadOnly, Category = "Cesium|Geocoder")
  TArray<FCesiumGeocoderServiceAttribution> Attributions;

  /**
   * @brief The features obtained from this geocoder service, if any.
   */
  UPROPERTY(BlueprintReadOnly, Category = "Cesium|Geocoder")
  TArray<FCesiumGeocoderServiceFeature> Features;
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
      Category = "Cesium|Geocoder",
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

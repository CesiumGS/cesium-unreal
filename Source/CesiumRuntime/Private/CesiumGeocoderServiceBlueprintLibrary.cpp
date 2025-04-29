#include "CesiumGeocoderServiceBlueprintLibrary.h"
#include "CesiumRuntime.h"

FCesiumGeocoderServiceAttribution::FCesiumGeocoderServiceAttribution(
    const CesiumIonClient::GeocoderAttribution& attribution)
    : Html(UTF8_TO_TCHAR(attribution.html.c_str())),
      bShowOnScreen(attribution.showOnScreen) {}

FVector UCesiumGeocoderServiceFeature::GetCartographic() const {
  if (!this->_feature) {
    return FVector::Zero();
  }

  const CesiumGeospatial::Cartographic cartographic =
      this->_feature->getCartographic();
  return FVector(
      CesiumUtility::Math::radiansToDegrees(cartographic.longitude),
      CesiumUtility::Math::radiansToDegrees(cartographic.latitude),
      cartographic.height);
}

FBox UCesiumGeocoderServiceFeature::GetGlobeRectangle() const {
  if (!this->_feature) {
    return FBox();
  }

  const CesiumGeospatial::GlobeRectangle rect =
      this->_feature->getGlobeRectangle();
  const CesiumGeospatial::Cartographic southWest = rect.getSouthwest();
  const CesiumGeospatial::Cartographic northEast = rect.getNortheast();
  return FBox(
      FVector(
          CesiumUtility::Math::radiansToDegrees(southWest.longitude),
          CesiumUtility::Math::radiansToDegrees(southWest.latitude),
          southWest.height),
      FVector(
          CesiumUtility::Math::radiansToDegrees(northEast.longitude),
          CesiumUtility::Math::radiansToDegrees(northEast.latitude),
          northEast.height));
}

FString UCesiumGeocoderServiceFeature::GetDisplayName() const {
  if (!this->_feature) {
    return FString();
  }

  return UTF8_TO_TCHAR(this->_feature->displayName.c_str());
}

void UCesiumGeocoderServiceFeature::SetFeature(
    CesiumIonClient::GeocoderFeature&& feature) {
  this->_feature = std::move(feature);
}

UCesiumGeocoderServiceIonGeocoderAsyncAction*
UCesiumGeocoderServiceIonGeocoderAsyncAction::Geocode(
    const FString& IonAccessToken,
    const UCesiumIonServer* CesiumIonServer,
    ECesiumIonGeocoderProviderType ProviderType,
    ECesiumIonGeocoderRequestType RequestType,
    const FString& Query) {
  UCesiumGeocoderServiceIonGeocoderAsyncAction* pAction =
      NewObject<UCesiumGeocoderServiceIonGeocoderAsyncAction>();
  pAction->_ionAccessToken = IonAccessToken;
  pAction->_cesiumIonServer = CesiumIonServer;
  pAction->_providerType = ProviderType;
  pAction->_requestType = RequestType;
  pAction->_query = Query;
  return pAction;
}

void UCesiumGeocoderServiceIonGeocoderAsyncAction::Activate() {
  CesiumIonClient::Connection::appData(
      getAsyncSystem(),
      getAssetAccessor(),
      TCHAR_TO_UTF8(*this->_cesiumIonServer->ApiUrl))
      .thenInMainThread([this](CesiumIonClient::Response<
                               CesiumIonClient::ApplicationData>&& response) {
        if (!response.value) {
          this->OnGeocodeRequestComplete.Broadcast(
              false,
              nullptr,
              FString::Printf(
                  TEXT("App data request failed, error code %s, message %s"),
                  UTF8_TO_TCHAR(response.errorCode.c_str()),
                  UTF8_TO_TCHAR(response.errorMessage.c_str())));
          return;
        }

        CesiumIonClient::Connection connection(
            getAsyncSystem(),
            getAssetAccessor(),
            TCHAR_TO_UTF8(*this->_ionAccessToken),
            *response.value,
            TCHAR_TO_UTF8(*this->_cesiumIonServer->ApiUrl));
        connection
            .geocode(
                (CesiumIonClient::GeocoderProviderType)this->_providerType,
                (CesiumIonClient::GeocoderRequestType)this->_requestType,
                TCHAR_TO_UTF8(*this->_query))
            .thenInMainThread([this](CesiumIonClient::Response<
                                     CesiumIonClient::GeocoderResult>&&
                                         response) {
              if (!response.value) {
                this->OnGeocodeRequestComplete.Broadcast(
                    false,
                    nullptr,
                    FString::Printf(
                        TEXT(
                            "Geocode request failed, error code %s, message %s"),
                        UTF8_TO_TCHAR(response.errorCode.c_str()),
                        UTF8_TO_TCHAR(response.errorMessage.c_str())));
                return;
              }

              UCesiumGeocoderServiceResult* pResult =
                  NewObject<UCesiumGeocoderServiceResult>();

              pResult->Attributions.Reserve(
                  response.value->attributions.size());
              for (const CesiumIonClient::GeocoderAttribution& attr :
                   response.value->attributions) {
                pResult->Attributions.Emplace(attr);
              }

              pResult->Features.Reserve(response.value->features.size());
              for (CesiumIonClient::GeocoderFeature& feature :
                   response.value->features) {
                UCesiumGeocoderServiceFeature* pFeature =
                    NewObject<UCesiumGeocoderServiceFeature>();
                pFeature->SetFeature(std::move(feature));
                pResult->Features.Emplace(pFeature);
              }

              this->OnGeocodeRequestComplete.Broadcast(
                  true,
                  pResult,
                  FString());
            });
      });
}

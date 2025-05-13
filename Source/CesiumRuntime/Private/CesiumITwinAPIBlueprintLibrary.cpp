#include "CesiumITwinAPIBlueprintLibrary.h"
#include "Async/Async.h"
#include "CesiumITwinCesiumCuratedContentRasterOverlay.h"
#include "CesiumRuntime.h"
#include <CesiumUtility/Result.h>

UCesiumITwinAPIAuthorizeAsyncAction*
UCesiumITwinAPIAuthorizeAsyncAction::Authorize(const FString& ClientID) {
  UCesiumITwinAPIAuthorizeAsyncAction* pAsyncAction =
      NewObject<UCesiumITwinAPIAuthorizeAsyncAction>();
  pAsyncAction->_clientId = ClientID;
  return pAsyncAction;
}

void UCesiumITwinAPIAuthorizeAsyncAction::Activate() {
  CesiumITwinClient::Connection::authorize(
      getAsyncSystem(),
      getAssetAccessor(),
      "Cesium for Unreal",
      TCHAR_TO_UTF8(*this->_clientId),
      "/itwin/auth/redirect",
      5081,
      std::vector<std::string>{"itwin-platform", "offline_access"},
      [&Callback = this->OnAuthorizationEvent](const std::string& url) {
        Callback.Broadcast(
            ECesiumITwinAuthorizationDelegateType::OpenUrl,
            UTF8_TO_TCHAR(url.c_str()),
            nullptr,
            TArray<FString>());
      })
      .thenInMainThread([this](CesiumUtility::Result<
                               CesiumUtility::IntrusivePointer<
                                   CesiumITwinClient::Connection>>&&
                                   connection) {
        if (!IsValid(this)) {
          UE_LOG(
              LogCesium,
              Warning,
              TEXT(
                  "Authorization finished but authorize async action is no longer valid."));
          return;
        }

        if (!connection.pValue) {
          TArray<FString> Errors;
          for (const std::string& error : connection.errors.errors) {
            Errors.Emplace(UTF8_TO_TCHAR(error.c_str()));
          }

          for (const std::string& warning : connection.errors.warnings) {
            Errors.Emplace(UTF8_TO_TCHAR(warning.c_str()));
          }

          this->OnAuthorizationEvent.Broadcast(
              ECesiumITwinAuthorizationDelegateType::Failure,
              FString(),
              nullptr,
              Errors);
        } else {
          UCesiumITwinConnection* pConnection =
              NewObject<UCesiumITwinConnection>();
          pConnection->SetConnection(connection.pValue);
          this->OnAuthorizationEvent.Broadcast(
              ECesiumITwinAuthorizationDelegateType::Success,
              FString(),
              pConnection,
              TArray<FString>());
          this->SetReadyToDestroy();
        }
      });
}

UCesiumITwinAPIGetProfileAsyncAction*
UCesiumITwinAPIGetProfileAsyncAction::GetProfile(
    UCesiumITwinConnection* pConnection) {
  UCesiumITwinAPIGetProfileAsyncAction* pAsyncAction =
      NewObject<UCesiumITwinAPIGetProfileAsyncAction>();
  pAsyncAction->pConnection = pConnection->pConnection;
  return pAsyncAction;
}

void UCesiumITwinAPIGetProfileAsyncAction::Activate() {
  if (!this->pConnection) {
    TArray<FString> Errors;
    Errors.Push("No connection to iTwin.");
    OnProfileResult.Broadcast(nullptr, Errors);
    return;
  }

  this->pConnection->me().thenInMainThread([this](CesiumUtility::Result<
                                                  CesiumITwinClient::
                                                      UserProfile>&& result) {
    if (!IsValid(this)) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Authorization finished but authorize async action is no longer valid."));
      return;
    }

    if (!result.value) {
      TArray<FString> Errors;
      for (const std::string& error : result.errors.errors) {
        Errors.Emplace(UTF8_TO_TCHAR(error.c_str()));
      }

      for (const std::string& warning : result.errors.warnings) {
        Errors.Emplace(UTF8_TO_TCHAR(warning.c_str()));
      }

      OnProfileResult.Broadcast(nullptr, Errors);
    } else {
      UCesiumITwinUserProfile* pProfile = NewObject<UCesiumITwinUserProfile>();
      pProfile->SetProfile(std::move(*result.value));
      OnProfileResult.Broadcast(pProfile, TArray<FString>());
    }
  });
}

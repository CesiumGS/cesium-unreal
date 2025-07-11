// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumITwinSession.h"
#include "CesiumEditor.h"
#include "CesiumEditorSettings.h"
#include "CesiumIonServer.h"
#include "CesiumRuntimeSettings.h"
#include "CesiumSourceControl.h"
THIRD_PARTY_INCLUDES_START
#include "CesiumUtility/Uri.h"
THIRD_PARTY_INCLUDES_END
#include "CesiumUtility/Result.h"
#include "CesiumUtility/joinToString.h"
#include "FileHelpers.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"

#include <string>

// THIS IS A TEMPORARY TESTING CLIENT ID!
// It will be deleted eventually. DO NOT MERGE UNTIL THIS IS CHANGED!
static const std::string CESIUM_FOR_UNREAL_CLIENT_ID =
    "native-xS7Mkz7y4jZ3K6RMENGpSQfRd";

using namespace CesiumAsync;
using namespace CesiumITwinClient;

namespace {
template<typename T>
void logResponseErrors(const CesiumUtility::Result<T>& result) {
  if (result.errors.hasErrors()) {
    std::string errorMessage =
        "Response errors:\n -" +
        CesiumUtility::joinToString(result.errors.errors, "\n- ");
    UE_LOG(LogCesiumEditor, Error, TEXT("%s"), UTF8_TO_TCHAR(errorMessage.c_str()));
  }
}

void logResponseException(const std::exception& exception) {
  UE_LOG(
      LogCesiumEditor,
      Error,
      TEXT("Exception: %s"),
      UTF8_TO_TCHAR(exception.what()));
}

} // namespace

CesiumITwinSession::CesiumITwinSession(
    CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor)
    : _asyncSystem(asyncSystem),
      _pAssetAccessor(pAssetAccessor),
      _connection(std::nullopt),
      _profile(std::nullopt),
      _isConnecting(false),
      _isResuming(false),
      _isLoadingProfile(false),
      _loadProfileQueued(false),
      _authorizeUrl() {}

void CesiumITwinSession::connect() {
  if (this->isConnecting() || this->isConnected()) {
    return;
  }

  this->_isConnecting = true;

  std::shared_ptr<CesiumITwinSession> thiz = this->shared_from_this();

  CesiumITwinClient::Connection::authorize(
      thiz->_asyncSystem,
      thiz->_pAssetAccessor,
      "Cesium for Unreal",
      CESIUM_FOR_UNREAL_CLIENT_ID,
      "/itwin/auth/redirect",
    5081,
      {"itwin-platform", "offline_access"},
      [thiz](const std::string& url) {
        thiz->_authorizeUrl = url;

        thiz->_redirectUrl =
            CesiumUtility::Uri::getQueryValue(url, "redirect_uri");

        FPlatformProcess::LaunchURL(
            UTF8_TO_TCHAR(thiz->_authorizeUrl.c_str()),
            NULL,
            NULL);
      })
      .thenInMainThread([ thiz](CesiumUtility::Result<CesiumITwinClient::Connection>&&
                     connectionResult) {
        thiz->_isConnecting = false;
        if (connectionResult.errors.hasErrors()) {
          thiz->_connection = std::nullopt;
          logResponseErrors(connectionResult);
          thiz->ConnectionUpdated.Broadcast();
        } else {
          thiz->_connection = std::move(connectionResult.value);
          thiz->ConnectionUpdated.Broadcast();
          thiz->startQueuedLoads();
        }
      })
      .catchInMainThread(
          [thiz](std::exception&& e) {
            UE_LOG(
                LogCesiumEditor,
                Error,
                TEXT("Error connecting: %s"),
                UTF8_TO_TCHAR(e.what()));
            thiz->_isConnecting = false;
            thiz->_connection = std::nullopt;
            thiz->ConnectionUpdated.Broadcast();
          });
}

void CesiumITwinSession::disconnect() {
  this->_connection.reset();
  this->_profile.reset();

  this->ConnectionUpdated.Broadcast();
  this->ProfileUpdated.Broadcast();
}

void CesiumITwinSession::refreshProfile() {
  if (!this->_connection || this->_isLoadingProfile) {
    this->_loadProfileQueued = true;
    return;
  }

  this->_isLoadingProfile = true;
  this->_loadProfileQueued = false;

  std::shared_ptr<CesiumITwinSession> thiz = this->shared_from_this();

  this->_connection->me()
      .thenInMainThread([thiz](CesiumUtility::Result<CesiumITwinClient::UserProfile>&& profile) {
        logResponseErrors(profile);
        thiz->_isLoadingProfile = false;
        thiz->_profile = std::move(profile.value);
        thiz->ProfileUpdated.Broadcast();
            if (thiz->_loadProfileQueued) {
              thiz->refreshProfile();
            }
      })
      .catchInMainThread([thiz](std::exception&& e) {
        logResponseException(e);
        thiz->_isLoadingProfile = false;
        thiz->_profile = std::nullopt;
        thiz->ProfileUpdated.Broadcast();
        if (thiz->_loadProfileQueued) {
          thiz->refreshProfile();
        }
      });
}

const std::optional<CesiumITwinClient::Connection>&
CesiumITwinSession::getConnection() const {
  return this->_connection;
}

const CesiumITwinClient::UserProfile& CesiumITwinSession::getProfile() {
  static const CesiumITwinClient::UserProfile empty{};
  if (this->_profile) {
    return *this->_profile;
  } else {
    this->refreshProfile();
    return empty;
  }
}

bool CesiumITwinSession::refreshProfileIfNeeded() {
  if (this->_loadProfileQueued || !this->_profile.has_value()) {
    this->refreshProfile();
  }
  return this->isProfileLoaded();
}

void CesiumITwinSession::startQueuedLoads() {
  if (this->_loadProfileQueued) {
    this->refreshProfile();
  }
}

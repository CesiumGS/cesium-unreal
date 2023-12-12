// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonSession.h"
#include "CesiumEditor.h"
#include "CesiumEditorSettings.h"
#include "CesiumIonServer.h"
#include "CesiumRuntimeSettings.h"
#include "CesiumSourceControl.h"
#include "CesiumUtility/Uri.h"
#include "FileHelpers.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"

using namespace CesiumAsync;
using namespace CesiumIonClient;

namespace {

template <typename T> void logResponseErrors(const Response<T>& response) {
  if (!response.errorCode.empty() && !response.errorMessage.empty()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT("%s (Code %s)"),
        UTF8_TO_TCHAR(response.errorMessage.c_str()),
        UTF8_TO_TCHAR(response.errorCode.c_str()));
  } else if (!response.errorCode.empty()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT("Code %s"),
        UTF8_TO_TCHAR(response.errorCode.c_str()));
  } else if (!response.errorMessage.empty()) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT("%s"),
        UTF8_TO_TCHAR(response.errorMessage.c_str()));
  }
}

void logResponseErrors(const std::exception& exception) {
  UE_LOG(
      LogCesiumEditor,
      Error,
      TEXT("Exception: %s"),
      UTF8_TO_TCHAR(exception.what()));
}

} // namespace

CesiumIonSession::CesiumIonSession(
    CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
    TWeakObjectPtr<UCesiumIonServer> pServer)
    : _asyncSystem(asyncSystem),
      _pAssetAccessor(pAssetAccessor),
      _pServer(pServer),
      _connection(std::nullopt),
      _profile(std::nullopt),
      _assets(std::nullopt),
      _tokens(std::nullopt),
      _defaults(std::nullopt),
      _isConnecting(false),
      _isResuming(false),
      _isLoadingProfile(false),
      _isLoadingAssets(false),
      _isLoadingTokens(false),
      _isLoadingDefaults(false),
      _loadProfileQueued(false),
      _loadAssetsQueued(false),
      _loadTokensQueued(false),
      _loadDefaultsQueued(false),
      _authorizeUrl() {}

void CesiumIonSession::connect() {
  if (!this->_pServer.IsValid() || this->isConnecting() ||
      this->isConnected() || this->isResuming()) {
    return;
  }

  UCesiumIonServer* pServer = this->_pServer.Get();

  this->_isConnecting = true;

  std::string ionServerUrl = TCHAR_TO_UTF8(*pServer->ServerUrl);

  Future<std::optional<std::string>> futureApiUrl =
      !pServer->ApiUrl.IsEmpty()
          ? this->_asyncSystem.createResolvedFuture<std::optional<std::string>>(
                TCHAR_TO_UTF8(*pServer->ApiUrl))
          : Connection::getApiUrl(
                this->_asyncSystem,
                this->_pAssetAccessor,
                ionServerUrl);

  std::shared_ptr<CesiumIonSession> thiz = this->shared_from_this();

  std::move(futureApiUrl)
      .thenInMainThread([ionServerUrl, thiz, pServer = this->_pServer](
                            std::optional<std::string>&& ionApiUrl) {
        if (!pServer.IsValid()) {
          thiz->_isConnecting = false;
          thiz->_connection = std::nullopt;
          thiz->ConnectionUpdated.Broadcast();
          return;
        }

        if (!ionApiUrl) {
          thiz->_isConnecting = false;
          thiz->_connection = std::nullopt;
          thiz->ConnectionUpdated.Broadcast();
          UE_LOG(
              LogCesiumEditor,
              Error,
              TEXT(
                  "Failed to retrieve API URL from the config.json file at the specified Ion server URL: %s"),
              UTF8_TO_TCHAR(ionServerUrl.c_str()));
          return;
        }

        if (pServer->ApiUrl.IsEmpty()) {
          pServer->ApiUrl = UTF8_TO_TCHAR(ionApiUrl->c_str());
          pServer->Modify();
          UEditorLoadingAndSavingUtils::SavePackages(
              {pServer->GetPackage()},
              true);
        }

        int64_t clientID = pServer->OAuth2ApplicationID;

        Connection::authorize(
            thiz->_asyncSystem,
            thiz->_pAssetAccessor,
            "Cesium for Unreal",
            clientID,
            "/cesium-for-unreal/oauth2/callback",
            {"assets:list",
             "assets:read",
             "profile:read",
             "tokens:read",
             "tokens:write",
             "geocode"},
            [thiz](const std::string& url) {
              thiz->_authorizeUrl = url;

              thiz->_redirectUrl =
                  CesiumUtility::Uri::getQueryValue(url, "redirect_uri");

              FPlatformProcess::LaunchURL(
                  UTF8_TO_TCHAR(thiz->_authorizeUrl.c_str()),
                  NULL,
                  NULL);
            },
            *ionApiUrl,
            CesiumUtility::Uri::resolve(ionServerUrl, "oauth"))
            .thenInMainThread([thiz](CesiumIonClient::Connection&& connection) {
              thiz->_isConnecting = false;
              thiz->_connection = std::move(connection);

              UCesiumEditorSettings* pSettings =
                  GetMutableDefault<UCesiumEditorSettings>();
              pSettings->UserAccessTokenMap.Add(
                  thiz->_pServer.Get(),
                  UTF8_TO_TCHAR(
                      thiz->_connection.value().getAccessToken().c_str()));
              pSettings->Save();

              thiz->ConnectionUpdated.Broadcast();

              thiz->startQueuedLoads();
            })
            .catchInMainThread([thiz](std::exception&& e) {
              thiz->_isConnecting = false;
              thiz->_connection = std::nullopt;
              thiz->ConnectionUpdated.Broadcast();
            });
      });
}

void CesiumIonSession::resume() {
  if (!this->_pServer.IsValid() || this->isConnecting() ||
      this->isConnected() || this->isResuming()) {
    return;
  }

  const UCesiumEditorSettings* pSettings = GetDefault<UCesiumEditorSettings>();
  const FString* pUserAccessToken =
      pSettings->UserAccessTokenMap.Find(this->_pServer.Get());

  if (!pUserAccessToken || pUserAccessToken->IsEmpty()) {
    // No existing session to resume.
    return;
  }

  this->_isResuming = true;

  std::shared_ptr<Connection> pConnection = std::make_shared<Connection>(
      this->_asyncSystem,
      this->_pAssetAccessor,
      TCHAR_TO_UTF8(**pUserAccessToken),
      TCHAR_TO_UTF8(*this->_pServer->ApiUrl));

  std::shared_ptr<CesiumIonSession> thiz = this->shared_from_this();

  // Verify that the connection actually works.
  pConnection->me()
      .thenInMainThread([thiz, pConnection](Response<Profile>&& response) {
        logResponseErrors(response);
        if (response.value.has_value()) {
          thiz->_connection = std::move(*pConnection);
        }
        thiz->_isResuming = false;
        thiz->ConnectionUpdated.Broadcast();

        thiz->startQueuedLoads();
      })
      .catchInMainThread([thiz](std::exception&& e) {
        logResponseErrors(e);
        thiz->_isResuming = false;
      });
}

void CesiumIonSession::disconnect() {
  this->_connection.reset();
  this->_profile.reset();
  this->_assets.reset();
  this->_tokens.reset();
  this->_defaults.reset();

  UCesiumEditorSettings* pSettings = GetMutableDefault<UCesiumEditorSettings>();
  pSettings->UserAccessTokenMap.Remove(this->_pServer.Get());
  pSettings->Save();

  this->ConnectionUpdated.Broadcast();
  this->ProfileUpdated.Broadcast();
  this->AssetsUpdated.Broadcast();
  this->TokensUpdated.Broadcast();
  this->DefaultsUpdated.Broadcast();
}

void CesiumIonSession::refreshProfile() {
  if (!this->_connection || this->_isLoadingProfile) {
    this->_loadProfileQueued = true;
    return;
  }

  this->_isLoadingProfile = true;
  this->_loadProfileQueued = false;

  std::shared_ptr<CesiumIonSession> thiz = this->shared_from_this();

  this->_connection->me()
      .thenInMainThread([thiz](Response<Profile>&& profile) {
        logResponseErrors(profile);
        thiz->_isLoadingProfile = false;
        thiz->_profile = std::move(profile.value);
        thiz->ProfileUpdated.Broadcast();
        if (thiz->_loadProfileQueued)
          thiz->refreshProfile();
      })
      .catchInMainThread([thiz](std::exception&& e) {
        logResponseErrors(e);
        thiz->_isLoadingProfile = false;
        thiz->_profile = std::nullopt;
        thiz->ProfileUpdated.Broadcast();
        if (thiz->_loadProfileQueued)
          thiz->refreshProfile();
      });
}

void CesiumIonSession::refreshAssets() {
  if (!this->_connection || this->_isLoadingAssets) {
    this->_loadAssetsQueued = true;
    return;
  }

  this->_isLoadingAssets = true;
  this->_loadAssetsQueued = false;

  std::shared_ptr<CesiumIonSession> thiz = this->shared_from_this();

  this->_connection->assets()
      .thenInMainThread([thiz](Response<Assets>&& assets) {
        logResponseErrors(assets);
        thiz->_isLoadingAssets = false;
        thiz->_assets = std::move(assets.value);
        thiz->AssetsUpdated.Broadcast();
        if (thiz->_loadAssetsQueued)
          thiz->refreshAssets();
      })
      .catchInMainThread([thiz](std::exception&& e) {
        logResponseErrors(e);
        thiz->_isLoadingAssets = false;
        thiz->_assets = std::nullopt;
        thiz->AssetsUpdated.Broadcast();
        if (thiz->_loadAssetsQueued)
          thiz->refreshAssets();
      });
}

void CesiumIonSession::refreshTokens() {
  if (!this->_connection || this->_isLoadingTokens) {
    this->_loadTokensQueued = true;
    return;
  }

  this->_isLoadingTokens = true;
  this->_loadTokensQueued = false;

  std::shared_ptr<CesiumIonSession> thiz = this->shared_from_this();

  this->_connection->tokens()
      .thenInMainThread([thiz](Response<TokenList>&& tokens) {
        logResponseErrors(tokens);
        thiz->_isLoadingTokens = false;
        thiz->_tokens = tokens.value
                            ? std::make_optional(std::move(tokens.value->items))
                            : std::nullopt;
        thiz->TokensUpdated.Broadcast();
        if (thiz->_loadTokensQueued)
          thiz->refreshTokens();
      })
      .catchInMainThread([thiz](std::exception&& e) {
        logResponseErrors(e);
        thiz->_isLoadingTokens = false;
        thiz->_tokens = std::nullopt;
        thiz->TokensUpdated.Broadcast();
        if (thiz->_loadTokensQueued)
          thiz->refreshTokens();
      });
}

void CesiumIonSession::refreshDefaults() {
  if (!this->_connection || this->_isLoadingDefaults) {
    this->_loadDefaultsQueued = true;
    return;
  }

  this->_isLoadingDefaults = true;
  this->_loadDefaultsQueued = false;

  std::shared_ptr<CesiumIonSession> thiz = this->shared_from_this();

  this->_connection->defaults()
      .thenInMainThread([thiz](Response<Defaults>&& defaults) {
        logResponseErrors(defaults);
        thiz->_isLoadingDefaults = false;
        thiz->_defaults = std::move(defaults.value);
        thiz->DefaultsUpdated.Broadcast();
        if (thiz->_loadDefaultsQueued)
          thiz->refreshDefaults();
      })
      .catchInMainThread([thiz](std::exception&& e) {
        logResponseErrors(e);
        thiz->_isLoadingDefaults = false;
        thiz->_defaults = std::nullopt;
        thiz->DefaultsUpdated.Broadcast();
        if (thiz->_loadDefaultsQueued)
          thiz->refreshDefaults();
      });
}

const std::optional<CesiumIonClient::Connection>&
CesiumIonSession::getConnection() const {
  return this->_connection;
}

const CesiumIonClient::Profile& CesiumIonSession::getProfile() {
  static const CesiumIonClient::Profile empty{};
  if (this->_profile) {
    return *this->_profile;
  } else {
    this->refreshProfile();
    return empty;
  }
}

const CesiumIonClient::Assets& CesiumIonSession::getAssets() {
  static const CesiumIonClient::Assets empty;
  if (this->_assets) {
    return *this->_assets;
  } else {
    this->refreshAssets();
    return empty;
  }
}

const std::vector<CesiumIonClient::Token>& CesiumIonSession::getTokens() {
  static const std::vector<CesiumIonClient::Token> empty;
  if (this->_tokens) {
    return *this->_tokens;
  } else {
    this->refreshTokens();
    return empty;
  }
}

const CesiumIonClient::Defaults& CesiumIonSession::getDefaults() {
  static const CesiumIonClient::Defaults empty;
  if (this->_defaults) {
    return *this->_defaults;
  } else {
    this->refreshDefaults();
    return empty;
  }
}

bool CesiumIonSession::refreshProfileIfNeeded() {
  if (this->_loadProfileQueued || !this->_profile.has_value()) {
    this->refreshProfile();
  }
  return this->isProfileLoaded();
}

bool CesiumIonSession::refreshAssetsIfNeeded() {
  if (this->_loadAssetsQueued || !this->_assets.has_value()) {
    this->refreshAssets();
  }
  return this->isAssetListLoaded();
}

bool CesiumIonSession::refreshTokensIfNeeded() {
  if (this->_loadTokensQueued || !this->_tokens.has_value()) {
    this->refreshTokens();
  }
  return this->isTokenListLoaded();
}

bool CesiumIonSession::refreshDefaultsIfNeeded() {
  if (this->_loadDefaultsQueued || !this->_defaults.has_value()) {
    this->refreshDefaults();
  }
  return this->isDefaultsLoaded();
}

Future<Response<Token>>
CesiumIonSession::findToken(const FString& token) const {
  if (!this->_connection) {
    return this->getAsyncSystem().createResolvedFuture(
        Response<Token>(0, "NOTCONNECTED", "Not connected to Cesium ion."));
  }

  std::string tokenString = TCHAR_TO_UTF8(*token);
  std::optional<std::string> maybeTokenID =
      Connection::getIdFromToken(tokenString);

  if (!maybeTokenID) {
    return this->getAsyncSystem().createResolvedFuture(
        Response<Token>(0, "INVALIDTOKEN", "The token is not valid."));
  }

  return this->_connection->token(*maybeTokenID);
}

namespace {

Token tokenFromServer(UCesiumIonServer* pServer) {
  Token result;

  if (pServer) {
    result.token = TCHAR_TO_UTF8(*pServer->DefaultIonAccessToken);
  }

  return result;
}

Future<Token> getTokenFuture(const CesiumIonSession& session) {
  std::shared_ptr<const CesiumIonSession> pSession = session.shared_from_this();
  TWeakObjectPtr<UCesiumIonServer> pServer = session.getServer();

  if (pServer.IsValid() && !pServer->DefaultIonAccessTokenId.IsEmpty()) {
    return session.getConnection()
        ->token(TCHAR_TO_UTF8(*pServer->DefaultIonAccessTokenId))
        .thenImmediately([pServer](Response<Token>&& tokenResponse) {
          if (tokenResponse.value) {
            return *tokenResponse.value;
          } else {
            return tokenFromServer(pServer.Get());
          }
        });
  } else if (!pServer->DefaultIonAccessToken.IsEmpty()) {
    return session.findToken(pServer->DefaultIonAccessToken)
        .thenImmediately([pServer](Response<Token>&& response) {
          if (response.value) {
            return *response.value;
          } else {
            return tokenFromServer(pServer.Get());
          }
        });
  } else {
    return session.getAsyncSystem().createResolvedFuture(
        tokenFromServer(pServer.Get()));
  }
}

} // namespace

SharedFuture<Token> CesiumIonSession::getProjectDefaultTokenDetails() {
  if (this->_projectDefaultTokenDetailsFuture) {
    // If the future is resolved but its token doesn't match the designated
    // default token, do the request again because the user probably specified a
    // new token.
    if (this->_projectDefaultTokenDetailsFuture->isReady() &&
        this->_projectDefaultTokenDetailsFuture->wait().token !=
            TCHAR_TO_UTF8(*this->_pServer->DefaultIonAccessToken)) {
      this->_projectDefaultTokenDetailsFuture.reset();
    } else {
      return *this->_projectDefaultTokenDetailsFuture;
    }
  }

  if (!this->isConnected()) {
    return this->getAsyncSystem()
        .createResolvedFuture(tokenFromServer(this->_pServer.Get()))
        .share();
  }

  this->_projectDefaultTokenDetailsFuture = getTokenFuture(*this).share();
  return *this->_projectDefaultTokenDetailsFuture;
}

void CesiumIonSession::invalidateProjectDefaultTokenDetails() {
  this->_projectDefaultTokenDetailsFuture.reset();
}

void CesiumIonSession::startQueuedLoads() {
  if (this->_loadProfileQueued)
    this->refreshProfile();
  if (this->_loadAssetsQueued)
    this->refreshAssets();
  if (this->_loadTokensQueued)
    this->refreshTokens();
  if (this->_loadDefaultsQueued)
    this->refreshDefaults();
}

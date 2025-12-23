// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumIonSession.h"
#include "CesiumEditor.h"
#include "CesiumEditorSettings.h"
#include "CesiumIonServer.h"
#include "CesiumRuntimeSettings.h"
#include "CesiumSourceControl.h"
THIRD_PARTY_INCLUDES_START
#include "CesiumUtility/Uri.h"
THIRD_PARTY_INCLUDES_END
#include "FileHelpers.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"

using namespace CesiumAsync;
using namespace CesiumIonClient;

const char* OAUTH2_REDIRECT_URI = "/cesium-for-unreal/oauth2/callback";

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

template <typename T>
bool disconnectOnNoValidToken(
    CesiumIonSession& session,
    const Response<T>& response) {
  if (response.errorCode == "NoValidToken") {
    session.disconnect();
    return true;
  } else {
    return false;
  }
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
      _appData(std::nullopt),
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

bool CesiumIonSession::isAuthenticationRequired() const {
  return this->_appData.has_value() ? this->_appData->needsOauthAuthentication()
                                    : true;
}

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
        CesiumAsync::Promise<bool> promise =
            thiz->_asyncSystem.createPromise<bool>();

        if (!pServer.IsValid()) {
          promise.reject(
              std::runtime_error("CesiumIonServer unexpectedly nullptr"));
          return promise.getFuture();
        }

        if (!ionApiUrl) {
          promise.reject(std::runtime_error(
              "Failed to retrieve API URL from the config.json file at the "
              "specified Ion server URL: " +
              ionServerUrl));
          return promise.getFuture();
        }

        if (pServer->ApiUrl.IsEmpty()) {
          pServer->ApiUrl = UTF8_TO_TCHAR(ionApiUrl->c_str());
          pServer->Modify();
          UEditorLoadingAndSavingUtils::SavePackages(
              {pServer->GetPackage()},
              true);
        }

        // Make request to /appData to learn the server's authentication mode
        return thiz->ensureAppDataLoaded();
      })
      .thenInMainThread([ionServerUrl, thiz, pServer = this->_pServer](
                            bool loadedAppData) {
        if (!loadedAppData || !thiz->_appData.has_value()) {
          Promise<Connection> promise =
              thiz->_asyncSystem.createPromise<Connection>();
          promise.reject(std::runtime_error(
              "Errors connecting: failed to obtain _appData, can't create connection"));
          return promise.getFuture();
        }

        if (thiz->_appData->needsOauthAuthentication()) {
          int64_t clientID = pServer->OAuth2ApplicationID;
          return CesiumIonClient::Connection::authorize(
                     thiz->_asyncSystem,
                     thiz->_pAssetAccessor,
                     "Cesium for Unreal",
                     clientID,
                     OAUTH2_REDIRECT_URI,
                     {"assets:list",
                      "assets:read",
                      "profile:read",
                      "tokens:read",
                      "tokens:write",
                      "geocode"},
                     [thiz](const std::string& url) {
                       thiz->_authorizeUrl = url;

                       thiz->_redirectUrl = CesiumUtility::Uri::getQueryValue(
                           url,
                           "redirect_uri");

                       FPlatformProcess::LaunchURL(
                           UTF8_TO_TCHAR(thiz->_authorizeUrl.c_str()),
                           NULL,
                           NULL);
                     },
                     thiz->_appData.value(),
                     std::string(TCHAR_TO_UTF8(*pServer->ApiUrl)),
                     CesiumUtility::Uri::resolve(ionServerUrl, "oauth"))
              .thenInMainThread(
                  [thiz](CesiumUtility::Result<Connection>&& result) {
                    Promise<Connection> promise =
                        thiz->_asyncSystem.createPromise<Connection>();
                    if (!result.value) {
                      promise.reject(std::runtime_error(result.errors.format(
                          "Errors connecting to Cesium ion:")));
                    } else {
                      promise.resolve(std::move(*result.value));
                    }

                    return promise.getFuture();
                  });
        }

        return thiz->_asyncSystem
            .createResolvedFuture<CesiumIonClient::Connection>(
                CesiumIonClient::Connection(
                    thiz->_asyncSystem,
                    thiz->_pAssetAccessor,
                    thiz->_appData.value(),
                    std::string(TCHAR_TO_UTF8(*pServer->ApiUrl))));
      })
      .thenInMainThread([ionServerUrl, thiz, pServer = this->_pServer](
                            CesiumIonClient::Connection&& connection) {
        thiz->_isConnecting = false;
        thiz->_connection = std::move(connection);

        UCesiumEditorSettings* pSettings =
            GetMutableDefault<UCesiumEditorSettings>();
        pSettings->UserAccessTokenMap.Add(
            thiz->_pServer.Get(),
            UTF8_TO_TCHAR(thiz->_connection.value().getAccessToken().c_str()));
        pSettings->UserRefreshTokenMap.Add(
            thiz->_pServer.Get(),
            UTF8_TO_TCHAR(thiz->_connection.value().getRefreshToken().c_str()));
        pSettings->Save();

        thiz->ConnectionUpdated.Broadcast();

        thiz->startQueuedLoads();
      })
      .catchInMainThread(
          [ionServerUrl, thiz, pServer = this->_pServer](std::exception&& e) {
            UE_LOG(LogCesiumEditor, Error, TEXT("%s"), UTF8_TO_TCHAR(e.what()));
            thiz->_isConnecting = false;
            thiz->_connection = std::nullopt;
            thiz->ConnectionUpdated.Broadcast();
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
  const FString* pUserRefreshToken =
      pSettings->UserRefreshTokenMap.Find(this->_pServer.Get());

  // We can resume a session with an access token or with a refresh token, or
  // both.
  if ((!pUserAccessToken || pUserAccessToken->IsEmpty()) &&
      (!pUserRefreshToken || pUserRefreshToken->IsEmpty())) {
    // No existing session to resume.
    return;
  }

  FString userAccessToken = pUserAccessToken ? *pUserAccessToken : "";
  FString userRefreshToken = pUserRefreshToken ? *pUserRefreshToken : "";

  this->_isResuming = true;

  std::shared_ptr<CesiumIonSession> thiz = this->shared_from_this();

  // Verify that the connection actually works.
  this->ensureAppDataLoaded()
      .thenInMainThread([thiz, userAccessToken, userRefreshToken](
                            bool loadedAppData) {
        if (!loadedAppData || !thiz->_appData.has_value()) {
          Promise<void> promise = thiz->_asyncSystem.createPromise<void>();

          promise.reject(std::runtime_error(
              "Failed to obtain _appData, can't resume connection"));
          return promise.getFuture();
        }

        CesiumUtility::Result<LoginToken> tokenResult =
            userAccessToken.IsEmpty()
                ? LoginToken("", -1)
                : LoginToken::parse(TCHAR_TO_UTF8(*userAccessToken));
        if (!tokenResult.value) {
          Promise<void> promise = thiz->_asyncSystem.createPromise<void>();
          promise.reject(std::runtime_error(
              tokenResult.errors.format("Failed to parse access token:")));
          return promise.getFuture();
        }

        std::shared_ptr<Connection> pConnection = std::make_shared<Connection>(
            thiz->_asyncSystem,
            thiz->_pAssetAccessor,
            std::move(*tokenResult.value),
            TCHAR_TO_UTF8(*userRefreshToken),
            thiz->_pServer->OAuth2ApplicationID,
            OAUTH2_REDIRECT_URI,
            *thiz->_appData,
            TCHAR_TO_UTF8(*thiz->_pServer->ApiUrl));

        return pConnection->me().thenInMainThread(
            [thiz, pConnection](Response<Profile>&& response) {
              logResponseErrors(response);
              if (response.value.has_value()) {
                thiz->_connection = std::move(*pConnection);
              }
              thiz->_isResuming = false;
              thiz->ConnectionUpdated.Broadcast();

              thiz->startQueuedLoads();
            });
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
  this->_appData.reset();

  this->_loadProfileQueued = false;
  this->_loadAssetsQueued = false;
  this->_loadTokensQueued = false;
  this->_loadDefaultsQueued = false;

  UCesiumEditorSettings* pSettings = GetMutableDefault<UCesiumEditorSettings>();
  pSettings->UserAccessTokenMap.Remove(this->_pServer.Get());
  pSettings->UserRefreshTokenMap.Remove(this->_pServer.Get());
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
        thiz->_isLoadingProfile = false;
        logResponseErrors(profile);
        if (disconnectOnNoValidToken(*thiz, profile))
          return;
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
        thiz->_isLoadingAssets = false;
        logResponseErrors(assets);
        if (disconnectOnNoValidToken(*thiz, assets))
          return;
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
        thiz->_isLoadingTokens = false;
        logResponseErrors(tokens);
        if (disconnectOnNoValidToken(*thiz, tokens))
          return;
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
        thiz->_isLoadingDefaults = false;
        logResponseErrors(defaults);
        if (disconnectOnNoValidToken(*thiz, defaults))
          return;
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

const CesiumIonClient::ApplicationData& CesiumIonSession::getAppData() {
  static const CesiumIonClient::ApplicationData empty{};
  if (this->_appData) {
    return *this->_appData;
  }
  return empty;
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

CesiumAsync::Future<bool> CesiumIonSession::ensureAppDataLoaded() {
  UCesiumIonServer* pServer = this->_pServer.Get();
  std::shared_ptr<CesiumIonSession> thiz = this->shared_from_this();

  return CesiumIonClient::Connection::appData(
             thiz->_asyncSystem,
             thiz->_pAssetAccessor,
             std::string(TCHAR_TO_UTF8(*pServer->ApiUrl)))
      .thenInMainThread(
          [thiz, pServer = this->_pServer](
              CesiumIonClient::Response<CesiumIonClient::ApplicationData>&&
                  appData) {
            CesiumAsync::Promise<bool> promise =
                thiz->_asyncSystem.createPromise<bool>();

            thiz->_appData = appData.value;
            if (!appData.value.has_value()) {
              UE_LOG(
                  LogCesiumEditor,
                  Error,
                  TEXT("Failed to obtain ion server application data: %s"),
                  UTF8_TO_TCHAR(appData.errorMessage.c_str()));
              promise.resolve(false);
            } else {
              promise.resolve(true);
            }

            return promise.getFuture();
          })
      .catchInMainThread([thiz, pServer = this->_pServer](std::exception&& e) {
        UE_LOG(
            LogCesiumEditor,
            Error,
            TEXT("Error obtaining appData: %s"),
            UTF8_TO_TCHAR(e.what()));
        return thiz->_asyncSystem.createResolvedFuture(false);
      });
}

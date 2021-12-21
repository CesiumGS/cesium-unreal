// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonSession.h"
#include "CesiumEditorSettings.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"

using namespace CesiumAsync;
using namespace CesiumIonClient;

CesiumIonSession::CesiumIonSession(
    CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor)
    : _asyncSystem(asyncSystem),
      _pAssetAccessor(pAssetAccessor),
      _connection(std::nullopt),
      _profile(std::nullopt),
      _assets(std::nullopt),
      _tokens(std::nullopt),
      _projectDefaultToken(std::nullopt),
      _isConnecting(false),
      _isResuming(false),
      _isLoadingProfile(false),
      _isLoadingAssets(false),
      _isLoadingTokens(false),
      _isLoadingProjectDefaultToken(false),
      _loadProfileQueued(false),
      _loadAssetsQueued(false),
      _loadTokensQueued(false),
      _authorizeUrl() {}

void CesiumIonSession::connect() {
  if (this->isConnecting() || this->isConnected() || this->isResuming()) {
    return;
  }

  this->_isConnecting = true;

  Connection::authorize(
      this->_asyncSystem,
      this->_pAssetAccessor,
      "Cesium for Unreal",
      190,
      "/cesium-for-unreal/oauth2/callback",
      {"assets:list",
       "assets:read",
       "profile:read",
       "tokens:read",
       "tokens:write",
       "geocode"},
      [this](const std::string& url) {
        this->_authorizeUrl = url;
        FPlatformProcess::LaunchURL(
            UTF8_TO_TCHAR(this->_authorizeUrl.c_str()),
            NULL,
            NULL);
      })
      .thenInMainThread([this](CesiumIonClient::Connection&& connection) {
        this->_isConnecting = false;
        this->_connection = std::move(connection);

        UCesiumEditorSettings* pSettings =
            GetMutableDefault<UCesiumEditorSettings>();
        pSettings->UserAccessToken =
            UTF8_TO_TCHAR(this->_connection.value().getAccessToken().c_str());
        pSettings->SaveConfig();

        this->ConnectionUpdated.Broadcast();
      })
      .catchInMainThread([this](std::exception&& e) {
        this->_isConnecting = false;
        this->_connection = std::nullopt;
        this->ConnectionUpdated.Broadcast();
      });
}

void CesiumIonSession::resume() {
  if (this->isConnecting() || this->isConnected() || this->isResuming()) {
    return;
  }

  const UCesiumEditorSettings* pSettings = GetDefault<UCesiumEditorSettings>();
  if (pSettings->UserAccessToken.IsEmpty()) {
    // No existing session to resume.
    return;
  }

  this->_isResuming = true;

  this->_connection = Connection(
      this->_asyncSystem,
      this->_pAssetAccessor,
      TCHAR_TO_UTF8(*GetDefault<UCesiumEditorSettings>()->UserAccessToken));

  // Verify that the connection actually works.
  this->_connection.value()
      .me()
      .thenInMainThread([this](Response<Profile>&& response) {
        if (!response.value.has_value()) {
          this->_connection.reset();
        }
        this->_isResuming = false;
        this->ConnectionUpdated.Broadcast();
      })
      .catchInMainThread([this](std::exception&& e) {
        this->_isResuming = false;
        this->_connection.reset();
      });
}

void CesiumIonSession::disconnect() {
  this->_connection.reset();
  this->_profile.reset();
  this->_assets.reset();
  this->_tokens.reset();
  this->_projectDefaultToken.reset();

  UCesiumEditorSettings* pSettings = GetMutableDefault<UCesiumEditorSettings>();
  pSettings->UserAccessToken.Empty();
  pSettings->SaveConfig();

  this->ConnectionUpdated.Broadcast();
  this->ProfileUpdated.Broadcast();
  this->AssetsUpdated.Broadcast();
  this->TokensUpdated.Broadcast();
  this->ProjectDefaultTokenUpdated.Broadcast();
}

void CesiumIonSession::refreshProfile() {
  if (!this->_connection || this->_isLoadingProfile) {
    this->_loadProfileQueued = true;
    return;
  }

  this->_isLoadingProfile = true;
  this->_loadProfileQueued = false;

  this->_connection->me()
      .thenInMainThread([this](Response<Profile>&& profile) {
        this->_isLoadingProfile = false;
        this->_profile = std::move(profile.value);
        this->ProfileUpdated.Broadcast();
        this->refreshProfileIfNeeded();
      })
      .catchInMainThread([this](std::exception&& e) {
        this->_isLoadingProfile = false;
        this->_profile = std::nullopt;
        this->ProfileUpdated.Broadcast();
        this->refreshProfileIfNeeded();
      });
}

void CesiumIonSession::refreshAssets() {
  if (!this->_connection || this->_isLoadingAssets) {
    return;
  }

  this->_isLoadingAssets = true;
  this->_loadAssetsQueued = false;

  this->_connection->assets()
      .thenInMainThread([this](Response<Assets>&& assets) {
        this->_isLoadingAssets = false;
        this->_assets = std::move(assets.value);
        this->AssetsUpdated.Broadcast();
        this->refreshAssetsIfNeeded();
      })
      .catchInMainThread([this](std::exception&& e) {
        this->_isLoadingAssets = false;
        this->_assets = std::nullopt;
        this->AssetsUpdated.Broadcast();
        this->refreshAssetsIfNeeded();
      });
}

void CesiumIonSession::refreshTokens() {
  if (!this->_connection || this->_isLoadingTokens) {
    return;
  }

  this->_isLoadingTokens = true;
  this->_loadTokensQueued = false;

  this->_connection->tokens()
      .thenInMainThread([this](Response<TokenList>&& tokens) {
        this->_isLoadingTokens = false;
        this->_tokens = tokens.value
                            ? std::make_optional(std::move(tokens.value->items))
                            : std::nullopt;
        this->TokensUpdated.Broadcast();
        this->refreshTokensIfNeeded();
        this->refreshProjectDefaultTokenIfNeeded();
      })
      .catchInMainThread([this](std::exception&& e) {
        this->_isLoadingTokens = false;
        this->_tokens = std::nullopt;
        this->TokensUpdated.Broadcast();
        this->refreshTokensIfNeeded();
      });
}

void CesiumIonSession::refreshProjectDefaultToken() {
  if (this->_isLoadingProjectDefaultToken) {
    return;
  }

  if (!this->_connection || !this->isTokenListLoaded()) {
    this->_loadAssetAccessTokenQueued = true;
    this->refreshTokens();
    return;
  }

  this->_isLoadingProjectDefaultToken = true;
  this->_loadAssetAccessTokenQueued = false;

  std::string tokenName = TCHAR_TO_UTF8(FApp::GetProjectName());
  tokenName += " (Created by Cesium for Unreal)";

  // TODO: rather than find a token by name, it would be better to store the
  // token ID in the UE project somewhere.
  const std::vector<Token>& tokenList = this->getTokens();
  const Token* pFound = nullptr;

  for (auto& token : tokenList) {
    if (token.name == tokenName) {
      pFound = &token;
    }
  }

  CesiumAsync::Future<Token> futureToken =
      pFound ? this->_asyncSystem.createResolvedFuture<Token>(Token(*pFound))
             : this->_connection
                   ->createToken(tokenName, {"assets:read"}, std::nullopt)
                   .thenInMainThread(
                       [this](CesiumIonClient::Response<Token>&& token) {
                         if (token.value) {
                           return token.value.value();
                         } else {
                           throw std::runtime_error("Failed to create token.");
                         }
                       });

  std::move(futureToken)
      .thenInMainThread([this](CesiumIonClient::Token&& token) {
        this->_projectDefaultToken = std::move(token);
        this->_isLoadingProjectDefaultToken = false;
        this->ProjectDefaultTokenUpdated.Broadcast();
      })
      .catchInMainThread([this](std::exception&& e) {
        this->_projectDefaultToken = std::nullopt;
        this->_isLoadingProjectDefaultToken = false;
        this->ProjectDefaultTokenUpdated.Broadcast();
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

const CesiumIonClient::Token& CesiumIonSession::getProjectDefaultToken() {
  static const CesiumIonClient::Token empty{};
  if (this->_projectDefaultToken) {
    return *this->_projectDefaultToken;
  } else {
    this->refreshProjectDefaultToken();
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

bool CesiumIonSession::refreshProjectDefaultTokenIfNeeded() {
  if (this->_loadAssetAccessTokenQueued ||
      !this->_projectDefaultToken.has_value()) {
    this->refreshProjectDefaultToken();
  }
  return this->isProjectDefaultTokenLoaded();
}

Future<std::optional<Token>>
CesiumIonSession::findToken(const FString& token) const {
  if (!this->_connection) {
    return this->getAsyncSystem().createResolvedFuture<std::optional<Token>>(
        std::nullopt);
  }

  std::string tokenString = TCHAR_TO_UTF8(*token);

  // We need a recursive lambda to handle pagination. It's awkward.
  auto pProcessPage = std::make_shared<
      std::function<Future<std::optional<Token>>(Response<TokenList> &&)>>();

  *pProcessPage =
      [pProcessPage,
       asyncSystem = this->getAsyncSystem(),
       tokenString,
       connection = *this->_connection](
          Response<TokenList>&& page) -> Future<std::optional<Token>> {
    if (!page.value) {
      return asyncSystem.createResolvedFuture<std::optional<Token>>(
          std::nullopt);
    }

    for (const Token& token : page.value->items) {
      if (token.token == tokenString) {
        return asyncSystem.createResolvedFuture<std::optional<Token>>(token);
      }
    }

    // Not on this page, try the next
    if (page.nextPageUrl) {
      return connection.nextPage(page).thenInMainThread(*pProcessPage);
    }

    // Token not found
    return asyncSystem.createResolvedFuture<std::optional<Token>>(std::nullopt);
  };

  return this->_connection->tokens().thenInMainThread(*pProcessPage);
}

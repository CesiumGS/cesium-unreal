#include "CesiumIonSession.h"
#include "UnrealConversions.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"

using namespace CesiumIonClient;

CesiumIonSession::CesiumIonSession(
    CesiumAsync::AsyncSystem& asyncSystem,
    const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor
) :
    _asyncSystem(asyncSystem),
    _pAssetAccessor(pAssetAccessor),
    _connection(std::nullopt),
    _profile(std::nullopt),
    _assets(std::nullopt),
    _tokens(std::nullopt),
    _assetAccessToken(std::nullopt),
    _isConnecting(false),
    _isLoadingProfile(false),
    _isLoadingAssets(false),
    _isLoadingTokens(false),
    _isLoadingAssetAccessToken(false),
    _loadProfileQueued(false),
    _loadAssetsQueued(false),
    _loadTokensQueued(false),
    _authorizeUrl()
{
}

void CesiumIonSession::connect() {
    if (this->isConnecting() || this->isConnected()) {
        return;
    }

    this->_isConnecting = true;

    Connection::authorize(
        this->_asyncSystem,
        this->_pAssetAccessor,
        "Cesium for Unreal",
        190,
        "/cesium-for-unreal/oauth2/callback",
        {
            "assets:list",
            "assets:read",
            "profile:read",
            "tokens:read",
            "tokens:write",
            "geocode"
        },
        [this](const std::string& url) {
            this->_authorizeUrl = url;
            FPlatformProcess::LaunchURL(*utf8_to_wstr(this->_authorizeUrl), NULL, NULL);
        }
    ).thenInMainThread([this](CesiumIonClient::Connection&& connection) {
        this->_isConnecting = false;
        this->_connection = std::move(connection);
        this->ConnectionUpdated.Broadcast();
    }).catchInMainThread([this](std::exception&& e) {
        this->_isConnecting = false;
        this->_connection = std::nullopt;
        this->ConnectionUpdated.Broadcast();
    });
}

void CesiumIonSession::disconnect() {
    this->_connection.reset();
    this->_profile.reset();
    this->_assets.reset();
    this->_tokens.reset();
    this->_assetAccessToken.reset();

    this->ConnectionUpdated.Broadcast();
    this->ProfileUpdated.Broadcast();
    this->AssetsUpdated.Broadcast();
    this->TokensUpdated.Broadcast();
}

void CesiumIonSession::refreshProfile() {
    if (!this->_connection || this->_isLoadingProfile) {
        this->_loadProfileQueued = true;
        return;
    }

    this->_isLoadingProfile = true;
    this->_loadProfileQueued = false;

    this->_connection->me().thenInMainThread([this](
        Response<Profile>&& profile
    ) {
        this->_isLoadingProfile = false;
        this->_profile = std::move(profile.value);
        this->ProfileUpdated.Broadcast();
        this->refreshProfileIfNeeded();
    }).catchInMainThread([this](std::exception&& e) {
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

    this->_connection->assets().thenInMainThread([this](
        Response<Assets>&& assets
    ) {
        this->_isLoadingAssets = false;
        this->_assets = std::move(assets.value);
        this->AssetsUpdated.Broadcast();
        this->refreshAssetsIfNeeded();
    }).catchInMainThread([this](std::exception&& e) {
        this->_isLoadingAssets = false;
        this->_assets = std::nullopt;
        this->AssetsUpdated.Broadcast();
        this->refreshAssetsIfNeeded();
    });
}

void CesiumIonSession::refreshTokens() {
    if (!this->_connection || this->_isLoadingAssets) {
        return;
    }

    this->_isLoadingTokens = true;
    this->_loadTokensQueued = false;

    this->_connection->tokens().thenInMainThread([this](
        Response<std::vector<Token>>&& tokens
    ) {
        this->_isLoadingTokens = false;
        this->_tokens = std::move(tokens.value);
        this->TokensUpdated.Broadcast();
        this->refreshTokensIfNeeded();
        this->refreshAssetAccessTokenIfNeeded();
    }).catchInMainThread([this](std::exception&& e) {
        this->_isLoadingTokens = false;
        this->_tokens = std::nullopt;
        this->TokensUpdated.Broadcast();
        this->refreshTokensIfNeeded();
    });
}

void CesiumIonSession::refreshAssetAccessToken() {
    if (this->_isLoadingAssetAccessToken) {
        return;
    }

    if (!this->_connection || !this->isTokenListLoaded()) {
        this->_loadAssetAccessTokenQueued = true;
        this->refreshTokens();
        return;
    }

    this->_isLoadingAssetAccessToken = true;
    this->_loadAssetAccessTokenQueued = false;

    std::string tokenName = wstr_to_utf8(FApp::GetProjectName()) + " (Created by Cesium for Unreal)";

    // TODO: rather than find a token by name, it would be better to store the token ID in the UE project somewhere.
    const std::vector<Token>& tokenList = this->getTokens();
    const Token* pFound = nullptr;

    for (auto& token : tokenList) {
        if (token.name == tokenName) {
            pFound = &token;
        }
    }

    CesiumAsync::Future<Token> futureToken = pFound
        ? this->_asyncSystem.createResolvedFuture<Token>(Token(*pFound))
        : this->_connection->createToken(
            tokenName,
            { "assets:read" },
            std::nullopt
        ).thenInMainThread([this](CesiumIonClient::Response<Token>&& token) {
            if (token.value) {
                return token.value.value();
            } else {
                throw std::runtime_error("Failed to create token.");
            }
        });

    std::move(futureToken).thenInMainThread([this](CesiumIonClient::Token&& token) {
        this->_assetAccessToken = std::move(token);
        this->_isLoadingAssetAccessToken = false;
    }).catchInMainThread([this](std::exception&& e) {
        this->_assetAccessToken = std::nullopt;
        this->_isLoadingAssetAccessToken = false;
    });
}

const std::optional<CesiumIonClient::Connection>& CesiumIonSession::getConnection() const {
    return this->_connection;
}

const CesiumIonClient::Profile& CesiumIonSession::getProfile() {
    static const CesiumIonClient::Profile empty;
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

const CesiumIonClient::Token& CesiumIonSession::getAssetAccessToken() {
    static const CesiumIonClient::Token empty;
    if (this->_assetAccessToken) {
        return *this->_assetAccessToken;
    } else {
        this->refreshAssetAccessToken();
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

bool CesiumIonSession::refreshAssetAccessTokenIfNeeded() {
    if (this->_loadAssetAccessTokenQueued || !this->_assetAccessToken.has_value()) {
        this->refreshAssetAccessToken();
    }
    return this->isAssetAccessTokenLoaded();
}

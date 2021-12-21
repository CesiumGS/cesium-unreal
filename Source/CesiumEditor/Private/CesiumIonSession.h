// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumIonClient/Connection.h"
#include <memory>

DECLARE_MULTICAST_DELEGATE(FIonUpdated);

class CesiumIonSession {
public:
  CesiumIonSession(
      CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor);

  const std::shared_ptr<CesiumAsync::IAssetAccessor>& getAssetAccessor() const {
    return this->_pAssetAccessor;
  }
  const CesiumAsync::AsyncSystem& getAsyncSystem() const {
    return this->_asyncSystem;
  }
  CesiumAsync::AsyncSystem& getAsyncSystem() { return this->_asyncSystem; }

  bool isConnected() const { return this->_connection.has_value(); }
  bool isConnecting() const { return this->_isConnecting; }
  bool isResuming() const { return this->_isResuming; }

  bool isProfileLoaded() const { return this->_profile.has_value(); }
  bool isLoadingProfile() const { return this->_isLoadingProfile; }

  bool isAssetListLoaded() const { return this->_assets.has_value(); }
  bool isLoadingAssetList() const { return this->_isLoadingAssets; }

  bool isTokenListLoaded() const { return this->_tokens.has_value(); }
  bool isLoadingTokenList() const { return this->_isLoadingTokens; }

  bool isProjectDefaultTokenLoaded() const {
    return this->_projectDefaultToken.has_value();
  }
  bool isLoadingProjectDefaultToken() const {
    return this->_isLoadingProjectDefaultToken;
  }

  void connect();
  void resume();
  void disconnect();

  void refreshProfile();
  void refreshAssets();
  void refreshTokens();
  void refreshProjectDefaultToken();

  FIonUpdated ConnectionUpdated;
  FIonUpdated ProfileUpdated;
  FIonUpdated AssetsUpdated;
  FIonUpdated TokensUpdated;
  FIonUpdated ProjectDefaultTokenUpdated;

  const std::optional<CesiumIonClient::Connection>& getConnection() const;
  const CesiumIonClient::Profile& getProfile();
  const CesiumIonClient::Assets& getAssets();
  const std::vector<CesiumIonClient::Token>& getTokens();
  const CesiumIonClient::Token& getProjectDefaultToken();

  const std::string& getAuthorizeUrl() const { return this->_authorizeUrl; }

  bool refreshProfileIfNeeded();
  bool refreshAssetsIfNeeded();
  bool refreshTokensIfNeeded();
  bool refreshProjectDefaultTokenIfNeeded();

  /**
   * Finds the details of the specified token in the user's account.
   *
   * If this session is not connected, returns std::nullopt.
   *
   * Even if the list of tokens is already loaded, this method does a new query
   * in order get the most up-to-date information about the token.
   *
   * @param token The token.
   * @return The details of the token, or std::nullopt if the token does not
   * exist in the signed-in user account.
   */
  CesiumAsync::Future<std::optional<CesiumIonClient::Token>>
  findToken(const FString& token) const;

private:
  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;

  std::optional<CesiumIonClient::Connection> _connection;
  std::optional<CesiumIonClient::Profile> _profile;
  std::optional<CesiumIonClient::Assets> _assets;
  std::optional<std::vector<CesiumIonClient::Token>> _tokens;
  std::optional<CesiumIonClient::Token> _projectDefaultToken;

  bool _isConnecting;
  bool _isResuming;
  bool _isLoadingProfile;
  bool _isLoadingAssets;
  bool _isLoadingTokens;
  bool _isLoadingProjectDefaultToken;

  bool _loadProfileQueued;
  bool _loadAssetsQueued;
  bool _loadTokensQueued;
  bool _loadAssetAccessTokenQueued;

  std::string _authorizeUrl;
};

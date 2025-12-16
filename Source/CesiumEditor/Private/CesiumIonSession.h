// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/SharedFuture.h"
#include "CesiumIonClient/Connection.h"
#include "Delegates/Delegate.h"
#include <memory>

DECLARE_MULTICAST_DELEGATE(FIonUpdated);

class UCesiumIonServer;

class CesiumIonSession : public std::enable_shared_from_this<CesiumIonSession> {
public:
  CesiumIonSession(
      CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      TWeakObjectPtr<UCesiumIonServer> pServer);

  const std::shared_ptr<CesiumAsync::IAssetAccessor>& getAssetAccessor() const {
    return this->_pAssetAccessor;
  }
  const CesiumAsync::AsyncSystem& getAsyncSystem() const {
    return this->_asyncSystem;
  }
  CesiumAsync::AsyncSystem& getAsyncSystem() { return this->_asyncSystem; }

  TWeakObjectPtr<UCesiumIonServer> getServer() const { return this->_pServer; }

  bool isConnected() const { return this->_connection.has_value(); }
  bool isConnecting() const { return this->_isConnecting; }
  bool isResuming() const { return this->_isResuming; }

  bool isProfileLoaded() const { return this->_profile.has_value(); }
  bool isLoadingProfile() const { return this->_isLoadingProfile; }

  bool isAssetListLoaded() const { return this->_assets.has_value(); }
  bool isLoadingAssetList() const { return this->_isLoadingAssets; }

  bool isTokenListLoaded() const { return this->_tokens.has_value(); }
  bool isLoadingTokenList() const { return this->_isLoadingTokens; }

  bool isDefaultsLoaded() const { return this->_defaults.has_value(); }
  bool isLoadingDefaults() const { return this->_isLoadingDefaults; }

  bool isAuthenticationRequired() const;

  void connect();
  void resume();
  void disconnect();

  void refreshProfile();
  void refreshAssets();
  void refreshTokens();
  void refreshDefaults();

  FIonUpdated ConnectionUpdated;
  FIonUpdated ProfileUpdated;
  FIonUpdated AssetsUpdated;
  FIonUpdated TokensUpdated;
  FIonUpdated DefaultsUpdated;

  std::optional<CesiumIonClient::Connection>& getConnection() const;
  const CesiumIonClient::Profile& getProfile();
  const CesiumIonClient::Assets& getAssets();
  const std::vector<CesiumIonClient::Token>& getTokens();
  const CesiumIonClient::Defaults& getDefaults();
  const CesiumIonClient::ApplicationData& getAppData();

  const std::string& getAuthorizeUrl() const { return this->_authorizeUrl; }
  const std::string& getRedirectUrl() const { return this->_redirectUrl; }

  bool refreshProfileIfNeeded();
  bool refreshAssetsIfNeeded();
  bool refreshTokensIfNeeded();
  bool refreshDefaultsIfNeeded();

  /**
   * Finds the details of the specified token in the user's account.
   *
   * If this session is not connected, returns std::nullopt.
   *
   * Even if the list of tokens is already loaded, this method does a new query
   * in order get the most up-to-date information about the token.
   *
   * @param token The token.
   * @return The details of the token, or an error response if the token does
   * not exist in the signed-in user account.
   */
  CesiumAsync::Future<CesiumIonClient::Response<CesiumIonClient::Token>>
  findToken(const FString& token) const;

  /**
   * Gets the project default token.
   *
   * If the project default token exists in the signed-in user's account, full
   * token details will be included. Otherwise, only the token value itself
   * (i.e. the `token` property`) will be included, and it may or may not be
   * valid. In the latter case, the `id` property will be an empty string.
   *
   * @return A future that resolves to the project default token.
   */
  CesiumAsync::SharedFuture<CesiumIonClient::Token>
  getProjectDefaultTokenDetails();

  void invalidateProjectDefaultTokenDetails();

private:
  void startQueuedLoads();

  /**
   * If the {@link _appData} field has no value, this method will request the
   * ion server's /appData endpoint to obtain its data.
   * @returns A future that resolves to true if _appData is present or false if
   * it couldn't be fetched.
   */
  CesiumAsync::Future<bool> ensureAppDataLoaded();

  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;
  TWeakObjectPtr<UCesiumIonServer> _pServer;

  mutable std::optional<CesiumIonClient::Connection> _connection;
  std::optional<CesiumIonClient::Profile> _profile;
  std::optional<CesiumIonClient::Assets> _assets;
  std::optional<std::vector<CesiumIonClient::Token>> _tokens;
  std::optional<CesiumIonClient::Defaults> _defaults;
  std::optional<CesiumIonClient::ApplicationData> _appData;

  std::optional<CesiumAsync::SharedFuture<CesiumIonClient::Token>>
      _projectDefaultTokenDetailsFuture;

  bool _isConnecting;
  bool _isResuming;
  bool _isLoadingProfile;
  bool _isLoadingAssets;
  bool _isLoadingTokens;
  bool _isLoadingDefaults;

  bool _loadProfileQueued;
  bool _loadAssetsQueued;
  bool _loadTokensQueued;
  bool _loadDefaultsQueued;

  std::string _authorizeUrl;
  std::string _redirectUrl;
};

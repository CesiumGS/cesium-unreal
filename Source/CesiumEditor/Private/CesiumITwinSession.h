// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumAsync/IAssetAccessor.h"
#include "CesiumAsync/SharedFuture.h"
#include "CesiumITwinClient/Connection.h"
#include "CesiumITwinClient/Profile.h"
#include "Delegates/Delegate.h"
#include <memory>

DECLARE_MULTICAST_DELEGATE(FITwinUpdated);

class CesiumITwinSession : public std::enable_shared_from_this<CesiumITwinSession> {
public:
  CesiumITwinSession(
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

  bool isProfileLoaded() const { return this->_profile.has_value(); }
  bool isLoadingProfile() const { return this->_isLoadingProfile; }

  void connect();
  void disconnect();

  void refreshProfile();

  FITwinUpdated ConnectionUpdated;
  FITwinUpdated ProfileUpdated;

  const std::optional<CesiumITwinClient::Connection>& getConnection() const;
  const CesiumITwinClient::UserProfile& getProfile();

  const std::string& getAuthorizeUrl() const { return this->_authorizeUrl; }
  const std::string& getRedirectUrl() const { return this->_redirectUrl; }

  bool refreshProfileIfNeeded();

private:
  void startQueuedLoads();

  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<CesiumAsync::IAssetAccessor> _pAssetAccessor;

  std::optional<CesiumITwinClient::Connection> _connection;
  std::optional<CesiumITwinClient::UserProfile> _profile;

  bool _isConnecting;
  bool _isResuming;
  bool _isLoadingProfile;

  bool _loadProfileQueued;

  std::string _authorizeUrl;
  std::string _redirectUrl;
};

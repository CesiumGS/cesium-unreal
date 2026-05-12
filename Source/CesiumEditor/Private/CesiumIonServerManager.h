// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumIonServer.h"
#include "Delegates/DelegateCombinations.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include <memory>

class CesiumIonSession;

DECLARE_MULTICAST_DELEGATE(FCesiumIonServerChanged);

class CESIUMEDITOR_API CesiumIonServerManager {
public:
  CesiumIonServerManager() noexcept;
  ~CesiumIonServerManager() noexcept;

  void Initialize();
  void ResumeAll();

  std::shared_ptr<CesiumIonSession> GetSession(UCesiumIonServer* Server);
  std::shared_ptr<CesiumIonSession> GetCurrentSession();

  const TArray<TWeakObjectPtr<UCesiumIonServer>>& GetServerList();
  void RefreshServerList();

  UCesiumIonServer* GetCurrentServer();
  void SetCurrentServer(UCesiumIonServer* pServer);

  FCesiumIonServerChanged ServerListChanged;
  FCesiumIonServerChanged CurrentServerChanged;

private:
  void OnAssetAdded(const FAssetData& asset);
  void OnAssetRemoved(const FAssetData& asset);
  void OnAssetUpdated(const FAssetData& asset);

  struct ServerSession {
    TWeakObjectPtr<UCesiumIonServer> Server;
    std::shared_ptr<CesiumIonSession> Session;
  };

  TArray<ServerSession> _sessions;
  TArray<TWeakObjectPtr<UCesiumIonServer>> _servers;
};

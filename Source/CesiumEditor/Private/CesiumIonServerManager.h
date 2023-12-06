// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "UObject/WeakObjectPtrTemplates.h"
#include <memory>

class UCesiumIonServer;
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

  const TArray<TObjectPtr<UCesiumIonServer>>& GetServerList();
  void RefreshServerList();

  UCesiumIonServer* GetCurrentServer();
  void SetCurrentServer(UCesiumIonServer* pServer);

  FCesiumIonServerChanged ServerListChanged;
  FCesiumIonServerChanged CurrentServerChanged;

private:
  void OnAssetAdded(const FAssetData& asset);
  void OnAssetRemoved(const FAssetData& asset);

  struct ServerSession {
    TWeakObjectPtr<UCesiumIonServer> Server;
    std::shared_ptr<CesiumIonSession> Session;
  };

  TArray<ServerSession> _sessions;
  TArray<TObjectPtr<UCesiumIonServer>> _servers;
};

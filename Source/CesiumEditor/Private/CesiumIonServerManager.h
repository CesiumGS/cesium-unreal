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

  std::shared_ptr<CesiumIonSession> GetSession(UCesiumIonServer* Server);
  std::shared_ptr<CesiumIonSession> GetCurrentSession();

  const TArray<TObjectPtr<UCesiumIonServer>>& GetServerList();
  void RefreshServerList();

  UCesiumIonServer* GetCurrent();
  void SetCurrent(UCesiumIonServer* pServer);

  FCesiumIonServerChanged ServerListChanged;
  FCesiumIonServerChanged CurrentChanged;

private:
  void OnAssetAddedOrRemoved(const FAssetData& asset);

  struct ServerSession {
    TWeakObjectPtr<UCesiumIonServer> Server;
    std::shared_ptr<CesiumIonSession> Session;
  };

  TArray<ServerSession> _sessions;
  TArray<TObjectPtr<UCesiumIonServer>> _servers;
};

// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "UObject/WeakObjectPtrTemplates.h"
#include <memory>

class UCesiumIonServer;
class CesiumIonSession;

DECLARE_MULTICAST_DELEGATE(FCesiumIonServerChanged);

class CESIUMEDITOR_API CesiumIonServerManager {
public:
  std::shared_ptr<CesiumIonSession> GetSession(UCesiumIonServer* Server);

  const TArray<TObjectPtr<UCesiumIonServer>>& GetServerList();

  UCesiumIonServer* GetCurrent();
  void SetCurrent(UCesiumIonServer* pServer);

  FCesiumIonServerChanged ServerListChanged;
  FCesiumIonServerChanged CurrentChanged;

private:
  struct ServerSession {
    TWeakObjectPtr<UCesiumIonServer> Server;
    std::shared_ptr<CesiumIonSession> Session;
  };

  TArray<ServerSession> _sessions;
  TArray<TObjectPtr<UCesiumIonServer>> _servers;
};

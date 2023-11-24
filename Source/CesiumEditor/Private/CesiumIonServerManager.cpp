// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumIonServerManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CesiumEditorSettings.h"
#include "CesiumIonServer.h"
#include "CesiumIonSession.h"
#include "CesiumRuntime.h"

std::shared_ptr<CesiumIonSession>
CesiumIonServerManager::GetSession(UCesiumIonServer* Server) {
  if (Server == nullptr)
    return nullptr;

  ServerSession* Found = this->_sessions.FindByPredicate(
      [Server](const ServerSession& ServerSession) {
        return ServerSession.Server == Server;
      });

  if (!Found) {
    std::shared_ptr<CesiumIonSession> pSession =
        std::make_shared<CesiumIonSession>(
            getAsyncSystem(),
            getAssetAccessor(),
            TWeakObjectPtr<UCesiumIonServer>(Server));
    int32 index = this->_sessions.Add(
        ServerSession{TWeakObjectPtr<UCesiumIonServer>(Server), pSession});
    Found = &this->_sessions[index];
  }

  return Found->Session;
}

const TArray<TObjectPtr<UCesiumIonServer>>&
CesiumIonServerManager::GetServerList() {
  this->_servers.Empty();

  TArray<FAssetData> CesiumIonServers;
  FAssetRegistryModule& AssetRegistryModule =
      FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
  AssetRegistryModule.Get().GetAssetsByClass(
      UCesiumIonServer::StaticClass()->GetFName(),
      CesiumIonServers);

  for (const FAssetData& ServerAsset : CesiumIonServers) {
    this->_servers.Add(Cast<UCesiumIonServer>(ServerAsset.GetAsset()));
  }

  return this->_servers;
}

UCesiumIonServer* CesiumIonServerManager::GetCurrent() {
  UCesiumEditorSettings* pSettings = GetMutableDefault<UCesiumEditorSettings>();

  if (!pSettings) {
    return UCesiumIonServer::GetOrCreateDefault();
  }

  UCesiumIonServer* pServer =
      pSettings->CurrentCesiumIonServer.LoadSynchronous();
  if (pServer == nullptr) {
    pServer = UCesiumIonServer::GetOrCreateDefault();
    pSettings->CurrentCesiumIonServer = pServer;
    pSettings->Save();
  }

  return pServer;
}

void CesiumIonServerManager::SetCurrent(UCesiumIonServer* pServer) {
  UCesiumEditorSettings* pSettings = GetMutableDefault<UCesiumEditorSettings>();
  if (pSettings) {
    pSettings->CurrentCesiumIonServer = pServer;
    CurrentChanged.Broadcast();
  }
}

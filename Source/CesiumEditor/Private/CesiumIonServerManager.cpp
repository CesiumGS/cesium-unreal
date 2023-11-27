// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumIonServerManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "CesiumEditorSettings.h"
#include "CesiumIonServer.h"
#include "CesiumIonSession.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"
#include "CesiumSourceControl.h"

CesiumIonServerManager::CesiumIonServerManager() noexcept {
  FAssetRegistryModule& AssetRegistryModule =
      FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
  AssetRegistryModule.GetRegistry().OnAssetAdded().AddRaw(
      this,
      &CesiumIonServerManager::OnAssetAddedOrRemoved);
  AssetRegistryModule.GetRegistry().OnAssetRemoved().AddRaw(
      this,
      &CesiumIonServerManager::OnAssetAddedOrRemoved);
}

CesiumIonServerManager::~CesiumIonServerManager() noexcept {
  FAssetRegistryModule* pAssetRegistryModule =
      FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry");
  if (pAssetRegistryModule) {
    pAssetRegistryModule->GetRegistry().OnAssetAdded().RemoveAll(this);
    pAssetRegistryModule->GetRegistry().OnAssetRemoved().RemoveAll(this);
  }
}

void CesiumIonServerManager::Initialize() {
  UCesiumRuntimeSettings* pSettings =
      GetMutableDefault<UCesiumRuntimeSettings>();
  if (pSettings) {
    if (!pSettings->DefaultIonAccessTokenId_DEPRECATED.IsEmpty() ||
        !pSettings->DefaultIonAccessToken_DEPRECATED.IsEmpty()) {
      UCesiumIonServer* pServer = UCesiumIonServer::GetOrCreateDefault();
      pServer->Modify();

      pServer->DefaultIonAccessTokenId =
          std::move(pSettings->DefaultIonAccessTokenId_DEPRECATED);
      pSettings->DefaultIonAccessTokenId_DEPRECATED.Empty();

      pServer->DefaultIonAccessToken =
          std::move(pSettings->DefaultIonAccessToken_DEPRECATED);
      pSettings->DefaultIonAccessToken_DEPRECATED.Empty();

      CesiumSourceControl::PromptToCheckoutConfigFile(
          pSettings->GetDefaultConfigFilename());

      pSettings->Modify();
      pSettings->TryUpdateDefaultConfigFile();
    }
  }
}

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

std::shared_ptr<CesiumIonSession> CesiumIonServerManager::GetCurrentSession() {
  return this->GetSession(this->GetCurrent());
}

const TArray<TObjectPtr<UCesiumIonServer>>&
CesiumIonServerManager::GetServerList() {
  this->RefreshServerList();
  return this->_servers;
}

void CesiumIonServerManager::RefreshServerList() {
  this->_servers.Empty();

  TArray<FAssetData> CesiumIonServers;
  FAssetRegistryModule& AssetRegistryModule =
      FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
  AssetRegistryModule.Get().GetAssetsByClass(
      UCesiumIonServer::StaticClass()->GetClassPathName(),
      CesiumIonServers);

  for (const FAssetData& ServerAsset : CesiumIonServers) {
    this->_servers.Add(Cast<UCesiumIonServer>(ServerAsset.GetAsset()));
  }
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

void CesiumIonServerManager::OnAssetAddedOrRemoved(const FAssetData& asset) {
  this->RefreshServerList();
  this->ServerListChanged.Broadcast();
}

// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumIonServerManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Cesium3DTileset.h"
#include "CesiumEditorSettings.h"
#include "CesiumIonRasterOverlay.h"
#include "CesiumIonServer.h"
#include "CesiumIonSession.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"
#include "CesiumSourceControl.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"

CesiumIonServerManager::CesiumIonServerManager() noexcept {
  FAssetRegistryModule& AssetRegistryModule =
      FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
  AssetRegistryModule.GetRegistry().OnAssetAdded().AddRaw(
      this,
      &CesiumIonServerManager::OnAssetAdded);
  AssetRegistryModule.GetRegistry().OnAssetRemoved().AddRaw(
      this,
      &CesiumIonServerManager::OnAssetRemoved);
  AssetRegistryModule.GetRegistry().OnAssetUpdated().AddRaw(
      this,
      &CesiumIonServerManager::OnAssetUpdated);
}

CesiumIonServerManager::~CesiumIonServerManager() noexcept {
  FAssetRegistryModule* pAssetRegistryModule =
      FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry");
  if (pAssetRegistryModule) {
    pAssetRegistryModule->GetRegistry().OnAssetAdded().RemoveAll(this);
    pAssetRegistryModule->GetRegistry().OnAssetRemoved().RemoveAll(this);
    pAssetRegistryModule->GetRegistry().OnAssetUpdated().RemoveAll(this);
  }
}

void CesiumIonServerManager::Initialize() {
  UCesiumRuntimeSettings* pSettings =
      GetMutableDefault<UCesiumRuntimeSettings>();
  if (pSettings) {
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    if (!pSettings->DefaultIonAccessTokenId_DEPRECATED.IsEmpty() ||
        !pSettings->DefaultIonAccessToken_DEPRECATED.IsEmpty()) {
      UCesiumIonServer* pServer = UCesiumIonServer::GetDefaultServer();
      pServer->Modify();

      pServer->DefaultIonAccessTokenId =
          std::move(pSettings->DefaultIonAccessTokenId_DEPRECATED);
      pSettings->DefaultIonAccessTokenId_DEPRECATED.Empty();

      pServer->DefaultIonAccessToken =
          std::move(pSettings->DefaultIonAccessToken_DEPRECATED);
      pSettings->DefaultIonAccessToken_DEPRECATED.Empty();

      UEditorLoadingAndSavingUtils::SavePackages({pServer->GetPackage()}, true);

      CesiumSourceControl::PromptToCheckoutConfigFile(
          pSettings->GetDefaultConfigFilename());

      pSettings->Modify();
      pSettings->TryUpdateDefaultConfigFile();
    }
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
  }

  UCesiumEditorSettings* pEditorSettings =
      GetMutableDefault<UCesiumEditorSettings>();
  if (pEditorSettings) {
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    if (!pEditorSettings->UserAccessToken_DEPRECATED.IsEmpty()) {
      UCesiumIonServer* pServer = UCesiumIonServer::GetDefaultServer();
      pEditorSettings->UserAccessTokenMap.Add(
          pServer,
          pEditorSettings->UserAccessToken_DEPRECATED);
      pEditorSettings->UserAccessToken_DEPRECATED.Empty();
      pEditorSettings->Save();
    }
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
  }

  UCesiumIonServer::SetServerForNewObjects(this->GetCurrentServer());
}

void CesiumIonServerManager::ResumeAll() {
  const TArray<TWeakObjectPtr<UCesiumIonServer>>& servers =
      this->GetServerList();
  for (const TWeakObjectPtr<UCesiumIonServer>& pWeakServer : servers) {
    UCesiumIonServer* pServer = pWeakServer.Get();
    if (pServer) {
      std::shared_ptr<CesiumIonSession> pSession = this->GetSession(pServer);
      pSession->resume();
      pSession->refreshProfileIfNeeded();
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
  return this->GetSession(this->GetCurrentServer());
}

const TArray<TWeakObjectPtr<UCesiumIonServer>>&
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

  this->ServerListChanged.Broadcast();
}

UCesiumIonServer* CesiumIonServerManager::GetCurrentServer() {
  UCesiumEditorSettings* pSettings = GetMutableDefault<UCesiumEditorSettings>();

  if (!pSettings) {
    return UCesiumIonServer::GetDefaultServer();
  }

  UCesiumIonServer* pServer =
      pSettings->CurrentCesiumIonServer.LoadSynchronous();
  if (pServer == nullptr) {
    pServer = UCesiumIonServer::GetDefaultServer();
    pSettings->CurrentCesiumIonServer = pServer;
    pSettings->Save();
  }

  return pServer;
}

void CesiumIonServerManager::SetCurrentServer(UCesiumIonServer* pServer) {
  UCesiumEditorSettings* pSettings = GetMutableDefault<UCesiumEditorSettings>();
  if (pSettings) {
    pSettings->CurrentCesiumIonServer = pServer;
    pSettings->Save();
  }

  if (UCesiumIonServer::GetServerForNewObjects() != pServer) {
    UCesiumIonServer::SetServerForNewObjects(pServer);
    CurrentServerChanged.Broadcast();
  }
}

void CesiumIonServerManager::OnAssetAdded(const FAssetData& asset) {
  if (asset.AssetClassPath !=
      UCesiumIonServer::StaticClass()->GetClassPathName())
    return;

  this->RefreshServerList();
}

void CesiumIonServerManager::OnAssetRemoved(const FAssetData& asset) {
  if (asset.AssetClassPath !=
      UCesiumIonServer::StaticClass()->GetClassPathName())
    return;

  this->RefreshServerList();

  UCesiumIonServer* pServer = Cast<UCesiumIonServer>(asset.GetAsset());
  if (pServer && this->GetCurrentServer() == pServer) {
    // Current server is being removed, so select a different one.
    TWeakObjectPtr<UCesiumIonServer>* ppNewServer =
        this->_servers.FindByPredicate(
            [pServer](const TWeakObjectPtr<UCesiumIonServer>& pCandidate) {
              return pCandidate.Get() != pServer;
            });
    if (ppNewServer != nullptr) {
      this->SetCurrentServer(ppNewServer->Get());
    } else {
      this->SetCurrentServer(nullptr);
    }
  }
}

void CesiumIonServerManager::OnAssetUpdated(const FAssetData& asset) {
  if (!GEditor)
    return;

  if (asset.AssetClassPath !=
      UCesiumIonServer::StaticClass()->GetClassPathName())
    return;

  // When a Cesium ion Server definition changes, refresh any objects that use
  // it.
  UCesiumIonServer* pServer = Cast<UCesiumIonServer>(asset.GetAsset());
  if (!pServer)
    return;

  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  if (!pCurrentWorld)
    return;

  for (TActorIterator<ACesium3DTileset> it(pCurrentWorld); it; ++it) {
    if (it->GetCesiumIonServer() == pServer) {
      it->RefreshTileset();
    } else {
      TArray<UCesiumIonRasterOverlay*> rasterOverlays;
      it->GetComponents<UCesiumIonRasterOverlay>(rasterOverlays);

      for (UCesiumIonRasterOverlay* pOverlay : rasterOverlays) {
        if (pOverlay->CesiumIonServer == pServer)
          pOverlay->Refresh();
      }
    }
  }
}

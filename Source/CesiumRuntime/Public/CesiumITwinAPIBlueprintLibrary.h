// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTileset.h"
#include "CesiumGeoreference.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SharedPointer.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
THIRD_PARTY_INCLUDES_START
#include <CesiumITwinClient/Connection.h>
THIRD_PARTY_INCLUDES_END

#include "CesiumITwinAPIBlueprintLibrary.generated.h"

UCLASS(BlueprintType)
class UCesiumITwinUserProfile : public UObject {
  GENERATED_BODY()
public:
  UCesiumITwinUserProfile() : UObject(), _profile() {}

  UCesiumITwinUserProfile(CesiumITwinClient::UserProfile&& profile)
      : UObject(), _profile(std::move(profile)) {}

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetID() { return UTF8_TO_TCHAR(_profile.id.c_str()); }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetDisplayName() {
    return UTF8_TO_TCHAR(_profile.displayName.c_str());
  }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetGivenName() { return UTF8_TO_TCHAR(_profile.givenName.c_str()); }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetSurname() { return UTF8_TO_TCHAR(_profile.surname.c_str()); }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetEmail() { return UTF8_TO_TCHAR(_profile.email.c_str()); }

  void SetProfile(CesiumITwinClient::UserProfile&& profile) {
    this->_profile = std::move(profile);
  }

private:
  CesiumITwinClient::UserProfile _profile;
};

UCLASS(BlueprintType)
class UCesiumITwinConnection : public UObject {
  GENERATED_BODY()
public:
  UCesiumITwinConnection() : UObject(), pConnection(nullptr) {}

  UCesiumITwinConnection(TSharedPtr<CesiumITwinClient::Connection> pConnection)
      : UObject(), pConnection(MoveTemp(pConnection)) {}

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  bool IsValid() { return this->pConnection != nullptr; }

  void SetConnection(TSharedPtr<CesiumITwinClient::Connection> pConnection) {
    this->pConnection = pConnection;
  }

  TSharedPtr<CesiumITwinClient::Connection> pConnection;
};

UCLASS(BlueprintType)
class UCesiumITwinResource : public UObject {
  GENERATED_BODY()
public:
  UCesiumITwinResource() : UObject() {}

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetID() { return UTF8_TO_TCHAR(_resource.id.c_str()); }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetDisplayName() {
    return UTF8_TO_TCHAR(_resource.displayName.c_str());
  }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetSource() {
    switch (_resource.source) {
    case CesiumITwinClient::ResourceSource::CesiumCuratedContent:
      return FString(TEXT("Cesium Curated Content"));
    case CesiumITwinClient::ResourceSource::MeshExport:
      return FString(TEXT("Mesh Export"));
    case CesiumITwinClient::ResourceSource::RealityData:
      return FString(TEXT("Reality Data"));
    }

    return FString(TEXT("Unknown"));
  }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetType() {
    switch (_resource.resourceType) {
    case CesiumITwinClient::ResourceType::Tileset:
      return FString(TEXT("Tileset"));
    case CesiumITwinClient::ResourceType::Imagery:
      return FString(TEXT("Imagery"));
    case CesiumITwinClient::ResourceType::Terrain:
      return FString(TEXT("Terrain"));
    }

    return FString(TEXT("Unknown"));
  }

  UFUNCTION(BlueprintCallable, Category = "Cesium|iTwin")
  void Spawn() {
    if (_resource.resourceType == CesiumITwinClient::ResourceType::Imagery) {
      // TODO
      return;
    }

    UWorld* pWorld = GetWorld();
    check(pWorld);

    ACesiumGeoreference* pParent =
        ACesiumGeoreference::GetDefaultGeoreference(pWorld);

    check(pParent);

    ACesium3DTileset* pTileset = pWorld->SpawnActor<ACesium3DTileset>();
    pTileset->AttachToActor(
        pParent,
        FAttachmentTransformRules::KeepRelativeTransform);

    pTileset->SetITwinAccessToken(
        UTF8_TO_TCHAR(this->_pConnection->getAccessToken().getToken().c_str()));

    switch (_resource.source) {
    case CesiumITwinClient::ResourceSource::CesiumCuratedContent:
      pTileset->SetTilesetSource(ETilesetSource::FromITwinCesiumCuratedContent);
      pTileset->SetITwinCesiumContentID(std::stoi(_resource.id));
      break;
    case CesiumITwinClient::ResourceSource::MeshExport:
      pTileset->SetTilesetSource(ETilesetSource::FromIModelMeshExportService);
      pTileset->SetIModelID(UTF8_TO_TCHAR(_resource.id.c_str()));
      break;
    case CesiumITwinClient::ResourceSource::RealityData:
      pTileset->SetTilesetSource(ETilesetSource::FromITwinRealityData);
      pTileset->SetITwinID(UTF8_TO_TCHAR(_resource.parentId->c_str()));
      pTileset->SetRealityDataID(UTF8_TO_TCHAR(_resource.parentId->c_str()));
      break;
    }
  }

  void SetResource(const CesiumITwinClient::ITwinResource& resource) {
    this->_resource = resource;
  }

  void SetConnection(TSharedPtr<CesiumITwinClient::Connection> pConnection) {
    this->_pConnection = pConnection;
  }

private:
  CesiumITwinClient::ITwinResource _resource;
  TSharedPtr<CesiumITwinClient::Connection> _pConnection;
};

UENUM(BlueprintType)
enum class ECesiumITwinDelegateType : uint8 {
  Invalid = 0,
  OpenUrl = 1,
  Failure = 2,
  Success = 3
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FCesiumITwinAuthorizationDelegate,
    ECesiumITwinDelegateType,
    Type,
    const FString&,
    Url,
    UCesiumITwinConnection*,
    Connection,
    const TArray<FString>&,
    Errors);

UCLASS()
class CESIUMRUNTIME_API UCesiumITwinAPIAuthorizeAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()

public:
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|iTwin",
      meta = (BlueprintInternalUseOnly = true))
  static UCesiumITwinAPIAuthorizeAsyncAction*
  Authorize(const FString& ClientID);

  UPROPERTY(BlueprintAssignable)
  FCesiumITwinAuthorizationDelegate OnAuthorizationEvent;

  virtual void Activate() override;

private:
  FString _clientId;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCesiumITwinGetProfileDelegate,
    UCesiumITwinUserProfile*,
    Profile,
    const TArray<FString>&,
    Errors);

UCLASS()
class CESIUMRUNTIME_API UCesiumITwinAPIGetProfileAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|iTwin",
      meta = (BlueprintInternalUseOnly = true))
  static UCesiumITwinAPIGetProfileAsyncAction*
  GetProfile(UCesiumITwinConnection* pConnection);

  UPROPERTY(BlueprintAssignable)
  FCesiumITwinGetProfileDelegate OnProfileResult;

  virtual void Activate() override;

  TSharedPtr<CesiumITwinClient::Connection> pConnection;
};

UENUM(BlueprintType)
enum class EGetResourcesCallbackType : uint8 { Status, Success, Failure };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
    FCesiumITwinGetResourcesDelegate,
    EGetResourcesCallbackType,
    Type,
    const TArray<UCesiumITwinResource*>&,
    Resources,
    int32,
    FinishedRequests,
    int32,
    TotalRequests,
    const TArray<FString>&,
    Errors);

UCLASS()
class CESIUMRUNTIME_API UCesiumITwinAPIGetResourcesAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|iTwin",
      meta =
          (BlueprintInternalUseOnly = true,
           WorldContext = "WorldContextObject"))
  static UCesiumITwinAPIGetResourcesAsyncAction* GetResources(
      const UObject* WorldContextObject,
      UCesiumITwinConnection* pConnection);

  UPROPERTY(BlueprintAssignable)
  FCesiumITwinGetResourcesDelegate OnResourcesEvent;

  virtual void Activate() override;

  TSharedPtr<CesiumITwinClient::Connection> pConnection;
};

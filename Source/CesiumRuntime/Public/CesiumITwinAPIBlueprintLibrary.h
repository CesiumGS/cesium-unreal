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
class UCesiumITwinConnection : public UObject {
  GENERATED_BODY()
public:
  UCesiumITwinConnection() : UObject(), pConnection(nullptr) {}

  UCesiumITwinConnection(TSharedPtr<CesiumITwinClient::Connection> pConnection)
      : UObject(), pConnection(MoveTemp(pConnection)) {}

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  bool IsValid() { return this->pConnection != nullptr; }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetAccessToken() {
    return UTF8_TO_TCHAR(
        this->pConnection->getAccessToken().getToken().c_str());
  }

  TSharedPtr<CesiumITwinClient::Connection>& GetConnection() {
    return this->pConnection;
  }

  void SetConnection(TSharedPtr<CesiumITwinClient::Connection> pConnection) {
    this->pConnection = pConnection;
  }

  TSharedPtr<CesiumITwinClient::Connection> pConnection;
};

UENUM(BlueprintType)
enum class ECesiumITwinAuthorizationDelegateType : uint8 {
  Invalid = 0,
  OpenUrl = 1,
  Failure = 2,
  Success = 3
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FCesiumITwinAuthorizationDelegate,
    ECesiumITwinAuthorizationDelegateType,
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

  CesiumUtility::IntrusivePointer<CesiumITwinClient::Connection> pConnection;
};

UENUM(BlueprintType)
enum class ECesiumITwinStatus : uint8 {
  Unknown = 0,
  Active = 1,
  Inactive = 2,
  Trial = 3
};

UCLASS(BlueprintType)
class UCesiumITwin : public UObject {
  GENERATED_BODY()
public:
  UCesiumITwin() : UObject(), _iTwin() {}

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetID() { return UTF8_TO_TCHAR(_iTwin.id.c_str()); }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetClass() { return UTF8_TO_TCHAR(_iTwin.iTwinClass.c_str()); }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetSubClass() { return UTF8_TO_TCHAR(_iTwin.subClass.c_str()); }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetType() { return UTF8_TO_TCHAR(_iTwin.type.c_str()); }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetNumber() { return UTF8_TO_TCHAR(_iTwin.number.c_str()); }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetDisplayName() { return UTF8_TO_TCHAR(_iTwin.displayName.c_str()); }

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  ECesiumITwinStatus GetStatus() { return (ECesiumITwinStatus)_iTwin.status; }

  void SetITwin(CesiumITwinClient::ITwin&& iTwin) {
    this->_iTwin = std::move(iTwin);
  }

private:
  CesiumITwinClient::ITwin _iTwin;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCesiumITwinListITwinsDelegate,
    TArray<UCesiumITwin*>,
    ITwins,
    bool,
    HasAnotherPage,
    const TArray<FString>&,
    Errors);

UCLASS()
class CESIUMRUNTIME_API UCesiumITwinAPIGetITwinsAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|iTwin",
      meta = (BlueprintInternalUseOnly = true))
  static UCesiumITwinAPIGetITwinsAsyncAction*
  GetITwins(UCesiumITwinConnection* pConnection);

  UPROPERTY(BlueprintAssignable)
  FCesiumITwinListITwinsDelegate OnITwinsResult;

  virtual void Activate() override;

  TSharedPtr<CesiumITwinClient::Connection> pConnection;
};

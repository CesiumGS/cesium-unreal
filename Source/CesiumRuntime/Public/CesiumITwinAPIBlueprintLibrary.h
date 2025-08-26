// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

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
        this->pConnection->getAuthenticationToken().getToken().c_str());
  }

  TSharedPtr<CesiumITwinClient::Connection>& GetConnection() {
    return this->pConnection;
  }

  void SetConnection(TSharedPtr<CesiumITwinClient::Connection> pConnection_) {
    this->pConnection = pConnection_;
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

  TSharedPtr<CesiumITwinClient::Connection> pConnection;
};

UENUM(BlueprintType)
enum class ECesiumITwinStatus : uint8 {
  Unknown = 0,
  Active = 1,
  Inactive = 2,
  Trial = 3
};

/**
 * @brief Information on a single iTwin.
 *
 * See
 * https://developer.bentley.com/apis/itwins/operations/get-my-itwins/#itwin-summary
 * for more information.
 */
UCLASS(BlueprintType)
class UCesiumITwin : public UObject {
  GENERATED_BODY()
public:
  UCesiumITwin() : UObject(), _iTwin() {}

  /** @brief The iTwin Id. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetID() { return UTF8_TO_TCHAR(_iTwin.id.c_str()); }

  /**
   * @brief The `Class` of your iTwin.
   *
   * See
   * https://developer.bentley.com/apis/itwins/overview/#itwin-classes-and-subclasses
   * for more information.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetClass() { return UTF8_TO_TCHAR(_iTwin.iTwinClass.c_str()); }

  /**
   * @brief The `subClass` of your iTwin.
   *
   * See
   * https://developer.bentley.com/apis/itwins/overview/#itwin-classes-and-subclasses
   * for more information.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetSubClass() { return UTF8_TO_TCHAR(_iTwin.subClass.c_str()); }

  /** @brief An open ended property to better define your iTwin's Type. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetType() { return UTF8_TO_TCHAR(_iTwin.type.c_str()); }

  /**
   * @brief A unique number or code for the iTwin.
   *
   * This is the value that uniquely identifies the iTwin within your
   * organization.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetNumber() { return UTF8_TO_TCHAR(_iTwin.number.c_str()); }

  /** @brief A display name for the iTwin. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetDisplayName() { return UTF8_TO_TCHAR(_iTwin.displayName.c_str()); }

  /** @brief The status of the iTwin. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  ECesiumITwinStatus GetStatus() { return (ECesiumITwinStatus)_iTwin.status; }

  void SetITwin(CesiumITwinClient::ITwin&& iTwin) {
    this->_iTwin = std::move(iTwin);
  }

private:
  CesiumITwinClient::ITwin _iTwin;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
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
  GetITwins(UCesiumITwinConnection* pConnection, int Page);

  UPROPERTY(BlueprintAssignable)
  FCesiumITwinListITwinsDelegate OnITwinsResult;

  virtual void Activate() override;

  TSharedPtr<CesiumITwinClient::Connection> pConnection;
  int page;
};

UENUM(BlueprintType)
enum class ECesiumIModelState : uint8 {
  Unknown = 0,
  Initialized = 1,
  NotInitialized = 2
};

/**
 * @brief An iModel.
 *
 * See
 * https://developer.bentley.com/apis/imodels-v2/operations/get-imodel-details/#imodel
 * for more information.
 */
UCLASS(BlueprintType)
class UCesiumIModel : public UObject {
  GENERATED_BODY()
public:
  UCesiumIModel() : UObject(), _iModel() {}

  /** @brief The iModel Id. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetID() { return UTF8_TO_TCHAR(_iModel.id.c_str()); }

  /** @brief Display name of the iModel. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetDisplayName() {
    return UTF8_TO_TCHAR(_iModel.displayName.c_str());
  }

  /** @brief Name of the iModel. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetName() { return UTF8_TO_TCHAR(_iModel.name.c_str()); }

  /** @brief Description of the iModel. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetDescription() {
    return UTF8_TO_TCHAR(_iModel.description.c_str());
  }

  /** @brief Indicates the state of the iModel. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  ECesiumIModelState GetState() { return (ECesiumIModelState)_iModel.state; }

  /**
   * @brief The maximum rectangular area on the Earth which encloses the iModel.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FBox GetExtent() {
    const CesiumGeospatial::Cartographic& southWest =
        _iModel.extent.getSouthwest();
    const CesiumGeospatial::Cartographic& northEast =
        _iModel.extent.getNortheast();
    return FBox(
        FVector(
            CesiumUtility::Math::radiansToDegrees(southWest.longitude),
            CesiumUtility::Math::radiansToDegrees(southWest.latitude),
            southWest.height),
        FVector(
            CesiumUtility::Math::radiansToDegrees(northEast.longitude),
            CesiumUtility::Math::radiansToDegrees(northEast.latitude),
            northEast.height));
  }

  void SetIModel(CesiumITwinClient::IModel&& iModel) {
    this->_iModel = std::move(iModel);
  }

private:
  CesiumITwinClient::IModel _iModel;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCesiumITwinListIModelsDelegate,
    TArray<UCesiumIModel*>,
    IModels,
    bool,
    HasAnotherPage,
    const TArray<FString>&,
    Errors);

UCLASS()
class CESIUMRUNTIME_API UCesiumITwinAPIGetIModelsAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|iTwin",
      meta = (BlueprintInternalUseOnly = true))
  static UCesiumITwinAPIGetIModelsAsyncAction* GetIModels(
      UCesiumITwinConnection* pConnection,
      const FString& ITwinId,
      int Page);

  UPROPERTY(BlueprintAssignable)
  FCesiumITwinListIModelsDelegate OnIModelsResult;

  virtual void Activate() override;

  TSharedPtr<CesiumITwinClient::Connection> pConnection;
  int page;
  FString iTwinId;
};

/**
 * @brief The status of an iModel Mesh Export.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#exportstatus
 * for more information.
 */
UENUM(BlueprintType)
enum class ECesiumIModelMeshExportStatus : uint8 {
  Unknown = 0,
  NotStarted = 1,
  InProgress = 2,
  Complete = 3,
  Invalid = 4
};

/**
 * @brief The type of mesh exported.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#startexport-exporttype
 * for more information.
 */
UENUM(BlueprintType)
enum class ECesiumIModelMeshExportType : uint8 {
  Unknown = 0,
  /**
   * @brief iTwin "3D Fast Transmission" (3DFT) format.
   */
  ITwin3DFT = 1,
  IModel = 2,
  Cesium = 3,
  Cesium3DTiles = 4,
};

/**
 * @brief An iModel mesh export.
 *
 * See
 * https://developer.bentley.com/apis/mesh-export/operations/get-export/#export
 * for more information.
 */
UCLASS(BlueprintType)
class UCesiumIModelMeshExport : public UObject {
  GENERATED_BODY()
public:
  UCesiumIModelMeshExport() : UObject(), _meshExport() {}

  /** @brief ID of the export request. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetID() { return UTF8_TO_TCHAR(_meshExport.id.c_str()); }

  /** @brief Name of the exported iModel. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetDisplayName() {
    return UTF8_TO_TCHAR(_meshExport.displayName.c_str());
  }

  /** @brief The status of the export job. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  ECesiumIModelMeshExportStatus GetState() {
    return (ECesiumIModelMeshExportStatus)_meshExport.status;
  }

  /** @brief The status of the export job. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  ECesiumIModelMeshExportType GetExportType() {
    return (ECesiumIModelMeshExportType)_meshExport.exportType;
  }

  void SetIModelMeshExport(CesiumITwinClient::IModelMeshExport&& iModel) {
    this->_meshExport = std::move(iModel);
  }

private:
  CesiumITwinClient::IModelMeshExport _meshExport;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCesiumITwinListIModelMeshExportsDelegate,
    TArray<UCesiumIModelMeshExport*>,
    MeshExports,
    bool,
    HasAnotherPage,
    const TArray<FString>&,
    Errors);

UCLASS()
class CESIUMRUNTIME_API UCesiumITwinAPIGetIModelMeshExportsAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|iTwin",
      meta = (BlueprintInternalUseOnly = true))
  static UCesiumITwinAPIGetIModelMeshExportsAsyncAction* GetIModelMeshExports(
      UCesiumITwinConnection* pConnection,
      const FString& iModelId,
      int Page);

  UPROPERTY(BlueprintAssignable)
  FCesiumITwinListIModelMeshExportsDelegate OnIModelMeshExportsResult;

  virtual void Activate() override;

  TSharedPtr<CesiumITwinClient::Connection> pConnection;
  int page;
  FString iModelId;
};

/**
 * @brief Indicates the nature of the reality data.
 *
 * See
 * https://developer.bentley.com/apis/reality-management/rm-rd-details/#classification
 * for more information.
 */
UENUM(BlueprintType)
enum class ECesiumITwinRealityDataClassification : uint8 {
  Unknown = 0,
  Terrain = 1,
  Imagery = 2,
  Pinned = 3,
  Model = 4,
  Undefined = 5
};

/**
 * @brief Information on reality data.
 *
 * See
 * https://developer.bentley.com/apis/reality-management/operations/get-all-reality-data/#reality-data-metadata
 * for more information.
 */
UCLASS(BlueprintType)
class UCesiumITwinRealityData : public UObject {
  GENERATED_BODY()
public:
  UCesiumITwinRealityData() : UObject(), _realityData() {}

  /**
   * @brief Identifier of the reality data.
   *
   * This identifier is assigned by the service at the creation of the reality
   * data. It is also unique.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetID() { return UTF8_TO_TCHAR(_realityData.id.c_str()); }

  /**
   * @brief The name of the reality data.
   *
   * This property may not contain any control sequence such as a URL or code.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetDisplayName() {
    return UTF8_TO_TCHAR(_realityData.displayName.c_str());
  }

  /**
   * @brief A textual description of the reality data.
   *
   * This property may not contain any control sequence such as a URL or code.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetDescription() {
    return UTF8_TO_TCHAR(_realityData.description.c_str());
  }

  /**
   * @brief Specific value constrained field that indicates the nature of the
   * reality data.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  ECesiumITwinRealityDataClassification GetClassification() {
    return (ECesiumITwinRealityDataClassification)_realityData.classification;
  }

  /**
   * @brief A key indicating the format of the data.
   *
   * The type property should be a specific indication of the format of the
   * reality data. Given a type, the consuming software should be able to
   * determine if it has the capacity to open the reality data. Although the
   * type field is a free string some specific values are reserved and other
   * values should be selected judiciously. Look at the documentation for <a
   * href="https://developer.bentley.com/apis/reality-management/rm-rd-details/#types">an
   * exhaustive list of reserved reality data types</a>.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetType() { return UTF8_TO_TCHAR(_realityData.type.c_str()); }

  /**
   * @brief Contains the rectangular area on the Earth which encloses the
   * reality data.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FBox GetExtent() {
    const CesiumGeospatial::Cartographic& southWest =
        _realityData.extent.getSouthwest();
    const CesiumGeospatial::Cartographic& northEast =
        _realityData.extent.getNortheast();
    return FBox(
        FVector(
            CesiumUtility::Math::radiansToDegrees(southWest.longitude),
            CesiumUtility::Math::radiansToDegrees(southWest.latitude),
            southWest.height),
        FVector(
            CesiumUtility::Math::radiansToDegrees(northEast.longitude),
            CesiumUtility::Math::radiansToDegrees(northEast.latitude),
            northEast.height));
  }

  /**
   * @brief A boolean value that is true if the data is being created. It is
   * false if the data has been completely uploaded.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  bool GetAuthoring() { return this->_realityData.authoring; }

  void SetITwinRealityData(CesiumITwinClient::ITwinRealityData&& realityData) {
    this->_realityData = std::move(realityData);
  }

private:
  CesiumITwinClient::ITwinRealityData _realityData;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCesiumITwinListRealityDataDelegate,
    TArray<UCesiumITwinRealityData*>,
    RealityData,
    bool,
    HasAnotherPage,
    const TArray<FString>&,
    Errors);

UCLASS()
class CESIUMRUNTIME_API UCesiumITwinAPIGetRealityDataAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|iTwin",
      meta = (BlueprintInternalUseOnly = true))
  static UCesiumITwinAPIGetRealityDataAsyncAction* GetITwinRealityData(
      UCesiumITwinConnection* pConnection,
      const FString& iTwinId,
      int Page);

  UPROPERTY(BlueprintAssignable)
  FCesiumITwinListRealityDataDelegate OnITwinRealityDataResult;

  virtual void Activate() override;

  TSharedPtr<CesiumITwinClient::Connection> pConnection;
  int page;
  FString iTwinId;
};

/**
 * @brief The type of content obtained from the iTwin Cesium Curated Content
 * API.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#contenttype
 * for more information.
 */
UENUM(BlueprintType)
enum class ECesiumCuratedContentType : uint8 {
  /** @brief The content type returned is not a known type. */
  Unknown = 0,
  /** @brief The content is a 3D Tiles tileset. */
  Cesium3DTiles = 1,
  /** @brief The content is a glTF model. */
  Gltf = 2,
  /**
   * @brief The content is imagery that can be loaded with \ref
   * CesiumRasterOverlays::ITwinCesiumCuratedContentRasterOverlay.
   */
  Imagery = 3,
  /** @brief The content is quantized mesh terrain. */
  Terrain = 4,
  /** @brief The content is in the Keyhole Markup Language (KML) format. */
  Kml = 5,
  /**
   * @brief The content is in the Cesium Language (CZML) format.
   *
   * See https://github.com/AnalyticalGraphicsInc/czml-writer/wiki/CZML-Guide
   * for more information.
   */
  Czml = 6,
  /** @brief The content is in the GeoJSON format. */
  GeoJson = 7,
};

/**
 * @brief Describes the state of the content during the upload and tiling
 * processes.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#contentstatus
 * for more information.
 */
UENUM(BlueprintType)
enum class ECesiumCuratedContentStatus : uint8 {
  Unknown = 0,
  AwaitingFiles = 1,
  NotStarted = 2,
  InProgress = 3,
  Complete = 4,
  Error = 5,
  DataError = 6
};

/**
 * @brief A single asset obtained from the iTwin Cesium Curated Content API.
 *
 * See
 * https://developer.bentley.com/apis/cesium-curated-content/operations/list-content/#content
 * for more information.
 */
UCLASS(BlueprintType)
class UCesiumCuratedContentAsset : public UObject {
  GENERATED_BODY()
public:
  UCesiumCuratedContentAsset() : UObject(), _asset() {}

  /**
   * @brief The unique identifier for the content.
   *
   * @remark The value returned from the API is a `uint64`, which Unreal does
   * not support in Blueprint. As this value is only used as a unique idenitifer
   * and not a numeric value, it is converted to a string for use in Blueprint.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetID() { return FString::Printf(TEXT("%llu"), _asset.id); }

  /** @brief The type of the content. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  ECesiumCuratedContentType GetType() {
    return (ECesiumCuratedContentType)_asset.type;
  }

  /** @brief Name of the exported iModel. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetName() { return UTF8_TO_TCHAR(_asset.name.c_str()); }

  /** @brief A Markdown string describing the content. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetDescription() { return UTF8_TO_TCHAR(_asset.description.c_str()); }

  /**
   * @brief A Markdown compatible string containing any required attribution for
   * the content.
   *
   * Clients will be required to display the attribution to end users.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  FString GetAttribution() { return UTF8_TO_TCHAR(_asset.attribution.c_str()); }

  /** @brief The status of the content. */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|iTwin")
  ECesiumCuratedContentStatus GetState() {
    return (ECesiumCuratedContentStatus)_asset.status;
  }

  void SetCesiumCuratedContentAsset(
      CesiumITwinClient::CesiumCuratedContentAsset&& asset) {
    this->_asset = std::move(asset);
  }

private:
  CesiumITwinClient::CesiumCuratedContentAsset _asset;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCesiumITwinListCesiumCuratedContentDelegate,
    TArray<UCesiumCuratedContentAsset*>,
    Assets,
    const TArray<FString>&,
    Errors);

UCLASS()
class CESIUMRUNTIME_API UCesiumITwinAPIListCesiumCuratedContentAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()
public:
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|iTwin",
      meta = (BlueprintInternalUseOnly = true))
  static UCesiumITwinAPIListCesiumCuratedContentAsyncAction*
  GetCesiumCuratedContentAssets(UCesiumITwinConnection* pConnection);

  UPROPERTY(BlueprintAssignable)
  FCesiumITwinListCesiumCuratedContentDelegate
      OnListCesiumCuratedContentDelegate;

  virtual void Activate() override;

  TSharedPtr<CesiumITwinClient::Connection> pConnection;
};

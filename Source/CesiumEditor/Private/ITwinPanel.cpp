// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "ITwinPanel.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Cesium3DTileset.h"
#include "CesiumCommands.h"
#include "CesiumEditor.h"
#include "CesiumIonPanel.h"
#include "CesiumIonServer.h"
#include "CesiumIonServerSelector.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"
THIRD_PARTY_INCLUDES_START
#include "CesiumUtility/Uri.h"
THIRD_PARTY_INCLUDES_END
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Interfaces/IPluginManager.h"
#include "ITwinLoginPanel.h"
#include "IonQuickAddPanel.h"
#include "LevelEditor.h"
#include "SelectCesiumIonToken.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScrollBox.h"

namespace {
bool isITwinSignedIn() {
  return FCesiumEditorModule::iTwinSession().getConnection().has_value();
}
} // namespace

ITwinPanel::ITwinPanel() : _pQuickAddPanel(nullptr) {}

ITwinPanel::~ITwinPanel() {
}

void ITwinPanel::Construct(const FArguments& InArgs) {
  ChildSlot
      [SNew(SVerticalBox) +
       SVerticalBox::Slot().AutoHeight()[Toolbar()] +
       SVerticalBox::Slot().VAlign(VAlign_Fill)
           [SNew(SScrollBox) + SScrollBox::Slot()[BasicQuickAddPanel()] +
            SScrollBox::Slot()[LoginPanel()] +
            SScrollBox::Slot()[MainIonQuickAddPanel()]] +
       SVerticalBox::Slot()
           .AutoHeight()
           .VAlign(VAlign_Bottom)
           .HAlign(HAlign_Right)[Version()]];
}

void ITwinPanel::Tick(
    const FGeometry& AllottedGeometry,
    const double InCurrentTime,
    const float InDeltaTime) {
  getAsyncSystem().dispatchMainThreadTasks();
  SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void ITwinPanel::Refresh() {
  if (!this->_pQuickAddPanel)
    return;

  std::shared_ptr<CesiumITwinSession> pSession =
      FCesiumEditorModule::iTwinSession().shared_from_this();

  this->_pQuickAddPanel->ClearItems();

  /*if (pSession->isLoadingDefaults()) {
    this->_pQuickAddPanel->SetMessage(FText::FromString("Loading..."));
  } else if (!pSession->isDefaultsLoaded()) {
    this->_pQuickAddPanel->SetMessage(
        FText::FromString("This server does not define any Quick Add assets."));
  } else {
    const CesiumIonClient::Defaults& defaults = pSession->getDefaults();

    this->_pQuickAddPanel->SetMessage(FText());

    for (const CesiumIonClient::QuickAddAsset& asset :
         defaults.quickAddAssets) {
      if (asset.type == "3DTILES" ||
          (asset.type == "TERRAIN" && !asset.rasterOverlays.empty())) {
        this->_pQuickAddPanel->AddItem(QuickAddItem{
            QuickAddItemType::TILESET,
            asset.name,
            asset.description,
            asset.objectName,
            asset.assetId,
            asset.rasterOverlays.empty() ? "" : asset.rasterOverlays[0].name,
            asset.rasterOverlays.empty() ? -1
                                         : asset.rasterOverlays[0].assetId});
      }
    }
  }*/

  this->_pQuickAddPanel->Refresh();
}

TSharedRef<SWidget> ITwinPanel::Toolbar() {
  TSharedRef<FUICommandList> commandList = MakeShared<FUICommandList>();

  commandList->MapAction(
      FCesiumCommands::Get().AddFromIon,
      FExecuteAction::CreateSP(this, &ITwinPanel::addFromIon),
      FCanExecuteAction::CreateStatic(isITwinSignedIn));
  commandList->MapAction(
      FCesiumCommands::Get().UploadToIon,
      FExecuteAction::CreateSP(this, &ITwinPanel::uploadToIon),
      FCanExecuteAction::CreateStatic(isITwinSignedIn));
  commandList->MapAction(
      FCesiumCommands::Get().SignOut,
      FExecuteAction::CreateSP(this, &ITwinPanel::signOut),
      FCanExecuteAction::CreateStatic(isITwinSignedIn));
  commandList->MapAction(
      FCesiumCommands::Get().OpenDocumentation,
      FExecuteAction::CreateSP(this, &ITwinPanel::openDocumentation));
  commandList->MapAction(
      FCesiumCommands::Get().OpenSupport,
      FExecuteAction::CreateSP(this, &ITwinPanel::openSupport));

  FToolBarBuilder builder(commandList, FMultiBoxCustomization::None);

  builder.AddToolBarButton(FCesiumCommands::Get().AddFromIon);
  builder.AddToolBarButton(FCesiumCommands::Get().UploadToIon);
  builder.AddToolBarButton(FCesiumCommands::Get().OpenTokenSelector);
  builder.AddToolBarButton(FCesiumCommands::Get().OpenDocumentation);
  builder.AddToolBarButton(FCesiumCommands::Get().OpenSupport);
  builder.AddToolBarButton(FCesiumCommands::Get().SignOut);

  return builder.MakeWidget();
}

TSharedRef<SWidget> ITwinPanel::LoginPanel() {
  return SNew(ITwinLoginPanel).Visibility_Lambda([]() {
    return isITwinSignedIn() ? EVisibility::Collapsed : EVisibility::Visible;
  });
}

TSharedRef<SWidget> ITwinPanel::MainIonQuickAddPanel() {
  this->_pQuickAddPanel =
      SNew(IonQuickAddPanel)
          .Title(FText::FromString("Quick Add Cesium ion Assets"))
          .Visibility_Lambda([]() {
            return isITwinSignedIn() ? EVisibility::Visible : EVisibility::Collapsed;
          });

  this->Refresh();

  return this->_pQuickAddPanel.ToSharedRef();
}

TSharedRef<SWidget> ITwinPanel::BasicQuickAddPanel() {
  TSharedPtr<IonQuickAddPanel> quickAddPanel =
      SNew(IonQuickAddPanel).Title(FText::FromString("Quick Add Basic Actors"));
  quickAddPanel->AddItem(
      {QuickAddItemType::TILESET,
       "Blank 3D Tiles Tileset",
       "An empty tileset that can be configured to show Cesium ion assets or tilesets from other sources.",
       "Blank Tileset",
       -1,
       "",
       -1});
  quickAddPanel->AddItem(
      {QuickAddItemType::SUNSKY,
       "Cesium SunSky",
       "An actor that represents a geospatially accurate sun and sky.",
       "",
       -1,
       "",
       -1});
  quickAddPanel->AddItem(QuickAddItem{
      QuickAddItemType::DYNAMIC_PAWN,
      "Dynamic Pawn",
      "A pawn that can be used to intuitively navigate in a geospatial environment.",
      "",
      -1,
      "",
      -1});
  quickAddPanel->AddItem(QuickAddItem{
      QuickAddItemType::CARTOGRAPHIC_POLYGON,
      "Cesium Cartographic Polygon",
      "An actor that can be used to draw out regions for use with clipping or other material effects.",
      "",
      -1,
      "",
      -1});
  return quickAddPanel.ToSharedRef();
}

TSharedRef<SWidget> ITwinPanel::Version() {
  IPluginManager& PluginManager = IPluginManager::Get();
  TSharedPtr<IPlugin> Plugin =
      PluginManager.FindPlugin(TEXT("CesiumForUnreal"));

  FString Version = Plugin ? TEXT("v") + Plugin->GetDescriptor().VersionName
                           : TEXT("Unknown Version");

  return SNew(SHyperlink)
      .Text(FText::FromString(Version))
      .ToolTipText(FText::FromString(
          TEXT("Open the Cesium for Unreal changelog in your web browser")))
      .OnNavigate_Lambda([]() {
        FPlatformProcess::LaunchURL(
            TEXT(
                "https://github.com/CesiumGS/cesium-unreal/blob/main/CHANGES.md"),
            NULL,
            NULL);
      });
}

void ITwinPanel::OnConnectionUpdated() {
  this->Refresh();
}

void ITwinPanel::OnDefaultsUpdated() { this->Refresh(); }

void ITwinPanel::addFromIon() {
  FLevelEditorModule* pLevelEditorModule =
      FModuleManager::GetModulePtr<FLevelEditorModule>(
          FName(TEXT("LevelEditor")));
  TSharedPtr<FTabManager> pTabManager =
      pLevelEditorModule ? pLevelEditorModule->GetLevelEditorTabManager()
                         : FGlobalTabmanager::Get();
  pTabManager->TryInvokeTab(FTabId(TEXT("CesiumIon")));
}

void ITwinPanel::uploadToIon() {
  UCesiumIonServer* pServer =
      FCesiumEditorModule::serverManager().GetCurrentServer();
  FPlatformProcess::LaunchURL(
      UTF8_TO_TCHAR(CesiumUtility::Uri::resolve(
                        TCHAR_TO_UTF8(*pServer->ServerUrl),
                        "addasset")
                        .c_str()),
      NULL,
      NULL);
}

void ITwinPanel::visitIon() {
  UCesiumIonServer* pServer =
      FCesiumEditorModule::serverManager().GetCurrentServer();
  FPlatformProcess::LaunchURL(*pServer->ServerUrl, NULL, NULL);
}

void ITwinPanel::signOut() {
  FCesiumEditorModule::iTwinSession().disconnect();
}

void ITwinPanel::openDocumentation() {
  FPlatformProcess::LaunchURL(TEXT("https://cesium.com/docs"), NULL, NULL);
}

void ITwinPanel::openSupport() {
  FPlatformProcess::LaunchURL(
      TEXT("https://community.cesium.com/"),
      NULL,
      NULL);
}

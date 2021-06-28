// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPanel.h"
#include "Cesium3DTileset.h"
#include "CesiumCommands.h"
#include "CesiumEditor.h"
#include "CesiumIonPanel.h"
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IonLoginPanel.h"
#include "IonQuickAddPanel.h"
#include "LevelEditor.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScrollBox.h"

void CesiumPanel::Construct(const FArguments& InArgs) {
  ChildSlot
      [SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[Toolbar()] +
       SVerticalBox::Slot().VAlign(VAlign_Fill)
           [SNew(SScrollBox) + SScrollBox::Slot()[BasicQuickAddPanel()] +
            SScrollBox::Slot()[LoginPanel()] +
            SScrollBox::Slot()[MainIonQuickAddPanel()]] +
       SVerticalBox::Slot()
           .AutoHeight()
           .VAlign(VAlign_Bottom)
           .HAlign(HAlign_Right)[ConnectionStatus()]];
}

void CesiumPanel::Tick(
    const FGeometry& AllottedGeometry,
    const double InCurrentTime,
    const float InDeltaTime) {
  FCesiumEditorModule::ion().getAsyncSystem().dispatchMainThreadTasks();
  SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

static bool isSignedIn() {
  return FCesiumEditorModule::ion().isConnected() &&
         FCesiumEditorModule::ion().refreshAssetAccessTokenIfNeeded();
}

TSharedRef<SWidget> CesiumPanel::Toolbar() {
  TSharedRef<FUICommandList> commandList = MakeShared<FUICommandList>();

  commandList->MapAction(
      FCesiumCommands::Get().AddFromIon,
      FExecuteAction::CreateSP(this, &CesiumPanel::addFromIon),
      FCanExecuteAction::CreateStatic(isSignedIn));
  commandList->MapAction(
      FCesiumCommands::Get().UploadToIon,
      FExecuteAction::CreateSP(this, &CesiumPanel::uploadToIon),
      FCanExecuteAction::CreateStatic(isSignedIn));
  // commandList->MapAction(
  //    FCesiumCommands::Get().AddBlankTileset,
  //    FExecuteAction::CreateSP(this, &CesiumPanel::addBlankTileset));
  // commandList->MapAction(FCesiumCommands::Get().AccessToken,
  // FExecuteAction::CreateSP(this, &CesiumPanel::accessToken),
  // FCanExecuteAction::CreateStatic(isSignedIn));
  commandList->MapAction(
      FCesiumCommands::Get().SignOut,
      FExecuteAction::CreateSP(this, &CesiumPanel::signOut),
      FCanExecuteAction::CreateStatic(isSignedIn));
  commandList->MapAction(
      FCesiumCommands::Get().OpenDocumentation,
      FExecuteAction::CreateSP(this, &CesiumPanel::openDocumentation));
  commandList->MapAction(
      FCesiumCommands::Get().OpenSupport,
      FExecuteAction::CreateSP(this, &CesiumPanel::openSupport));

  FToolBarBuilder builder(commandList, FMultiBoxCustomization::None);

  builder.AddToolBarButton(FCesiumCommands::Get().AddFromIon);
  builder.AddToolBarButton(FCesiumCommands::Get().UploadToIon);
  // builder.AddToolBarButton(FCesiumCommands::Get().AddBlankTileset);
  // builder.AddToolBarButton(FCesiumCommands::Get().AccessToken);
  builder.AddToolBarButton(FCesiumCommands::Get().OpenDocumentation);
  builder.AddToolBarButton(FCesiumCommands::Get().OpenSupport);
  builder.AddToolBarButton(FCesiumCommands::Get().SignOut);

  return builder.MakeWidget();
}

TSharedRef<SWidget> CesiumPanel::LoginPanel() {
  return SNew(IonLoginPanel).Visibility_Lambda([]() {
    return isSignedIn() ? EVisibility::Collapsed : EVisibility::Visible;
  });
}

TSharedRef<SWidget> CesiumPanel::MainIonQuickAddPanel() {
  TSharedPtr<IonQuickAddPanel> quickAddPanel =
      SNew(IonQuickAddPanel)
          .Title(FText::FromString("Add Cesium ion Assets"))
          .Visibility_Lambda([]() {
            return isSignedIn() ? EVisibility::Visible : EVisibility::Collapsed;
          });
  quickAddPanel->AddItem(
      {QuickAddItemType::TILESET,
       "Cesium World Terrain + Bing Maps Aerial imagery",
       "High-resolution global terrain tileset curated from several data sources, textured with Bing Maps satellite imagery.",
       "Cesium World Terrain",
       1,
       "Bing Maps Aerial",
       2});
  quickAddPanel->AddItem(
      {QuickAddItemType::TILESET,
       "Cesium World Terrain + Bing Maps Aerial with Labels imagery",
       "High-resolution global terrain tileset curated from several data sources, textured with labeled Bing Maps satellite imagery.",
       "Cesium World Terrain",
       1,
       "Bing Maps Aerial with Labels",
       3});
  quickAddPanel->AddItem(
      {QuickAddItemType::TILESET,
       "Cesium World Terrain + Bing Maps Road imagery",
       "High-resolution global terrain tileset curated from several data sources, textured with Bing Maps imagery.",
       "Cesium World Terrain",
       1,
       "Bing Maps Road",
       4});
  quickAddPanel->AddItem(
      {QuickAddItemType::TILESET,
       "Cesium World Terrain + Sentinel-2 imagery",
       "High-resolution global terrain tileset curated from several data sources, textured with high-resolution satellite imagery from the Sentinel-2 project.",
       "Cesium World Terrain",
       1,
       "Sentinel-2 imagery",
       3954});
  quickAddPanel->AddItem(
      {QuickAddItemType::TILESET,
       "Cesium OSM Buildings",
       "A 3D buildings layer derived from OpenStreetMap covering the entire world.",
       "Cesium OSM Buildings",
       96188,
       "",
       -1});

  return quickAddPanel.ToSharedRef();
}

TSharedRef<SWidget> CesiumPanel::BasicQuickAddPanel() {
  TSharedPtr<IonQuickAddPanel> quickAddPanel =
      SNew(IonQuickAddPanel).Title(FText::FromString("Add basic Actors"));
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
  return quickAddPanel.ToSharedRef();
}

TSharedRef<SWidget> CesiumPanel::ConnectionStatus() {

  auto linkVisibility = []() {
    FCesiumEditorModule::ion().refreshProfileIfNeeded();
    if (!FCesiumEditorModule::ion().isProfileLoaded()) {
      return EVisibility::Collapsed;
    }
    if (!isSignedIn()) {
      return EVisibility::Collapsed;
    }
    return EVisibility::Visible;
  };
  auto linkText = []() {
    auto& profile = FCesiumEditorModule::ion().getProfile();
    std::string s = "Connected to Cesium ion as " + profile.username;
    return FText::FromString(UTF8_TO_TCHAR(s.c_str()));
  };
  auto loadingMessageVisibility = []() {
    return FCesiumEditorModule::ion().isLoadingProfile()
               ? EVisibility::Visible
               : EVisibility::Collapsed;
  };
  return SNew(SVerticalBox) +
         SVerticalBox::Slot()
             [SNew(SHyperlink)
                  .Visibility_Lambda(linkVisibility)
                  .Text_Lambda(linkText)
                  .ToolTipText(FText::FromString(
                      TEXT("Open your Cesium ion account in your browser")))
                  .OnNavigate(this, &CesiumPanel::visitIon)] +
         SVerticalBox::Slot()[SNew(STextBlock)
                                  .Visibility_Lambda(loadingMessageVisibility)
                                  .Text(FText::FromString(
                                      TEXT("Loading user information...")))];
}

void CesiumPanel::addFromIon() {
  FLevelEditorModule* pLevelEditorModule =
      FModuleManager::GetModulePtr<FLevelEditorModule>(
          FName(TEXT("LevelEditor")));
  TSharedPtr<FTabManager> pTabManager =
      pLevelEditorModule ? pLevelEditorModule->GetLevelEditorTabManager()
                         : FGlobalTabmanager::Get();
  pTabManager->TryInvokeTab(FTabId(TEXT("CesiumIon")));
}

void CesiumPanel::uploadToIon() {
  FPlatformProcess::LaunchURL(
      TEXT("https://cesium.com/ion/addasset"),
      NULL,
      NULL);
}

void CesiumPanel::visitIon() {
  FPlatformProcess::LaunchURL(TEXT("https://cesium.com/ion"), NULL, NULL);
}

void CesiumPanel::addBlankTileset() {
  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

  GEditor->AddActor(
      pCurrentLevel,
      ACesium3DTileset::StaticClass(),
      FTransform(),
      false,
      RF_Public | RF_Transactional);
}

void CesiumPanel::accessToken() {}

void CesiumPanel::signOut() { FCesiumEditorModule::ion().disconnect(); }

void CesiumPanel::openDocumentation() {
  FPlatformProcess::LaunchURL(TEXT("https://cesium.com/docs"), NULL, NULL);
}

void CesiumPanel::openSupport() {
  FPlatformProcess::LaunchURL(
      TEXT("https://community.cesium.com/"),
      NULL,
      NULL);
}

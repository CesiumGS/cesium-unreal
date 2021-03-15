// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPanel.h"
#include "ACesium3DTileset.h"
#include "CesiumCommands.h"
#include "CesiumEditor.h"
#include "CesiumIonPanel.h"
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IonLoginPanel.h"
#include "IonQuickAddPanel.h"
#include "LevelEditor.h"
#include "Styling/SlateStyleRegistry.h"
#include "UnrealConversions.h"
#include "Widgets/Layout/SHeader.h"

void CesiumPanel::Construct(const FArguments& InArgs) {
  ChildSlot
      [SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[Toolbar()] +
       SVerticalBox::Slot().HAlign(HAlign_Fill)[LoginPanel()] +
       SVerticalBox::Slot().HAlign(HAlign_Fill)[MainPanel()] +
       SVerticalBox::Slot().AutoHeight().HAlign(
           HAlign_Right)[ConnectionStatus()]];
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
  commandList->MapAction(
      FCesiumCommands::Get().AddBlankTileset,
      FExecuteAction::CreateSP(this, &CesiumPanel::addBlankTileset));
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

  FToolBarBuilder builder(commandList, FMultiBoxCustomization::None);

  builder.AddToolBarButton(FCesiumCommands::Get().AddFromIon);
  builder.AddToolBarButton(FCesiumCommands::Get().UploadToIon);
  builder.AddToolBarButton(FCesiumCommands::Get().AddBlankTileset);
  // builder.AddToolBarButton(FCesiumCommands::Get().AccessToken);
  builder.AddToolBarButton(FCesiumCommands::Get().SignOut);
  builder.AddToolBarButton(FCesiumCommands::Get().OpenDocumentation);

  return builder.MakeWidget();
}

TSharedRef<SWidget> CesiumPanel::LoginPanel() {
  return SNew(IonLoginPanel).Visibility_Lambda([]() {
    return isSignedIn() ? EVisibility::Collapsed : EVisibility::Visible;
  });
}

TSharedRef<SWidget> CesiumPanel::MainPanel() {
  return SNew(IonQuickAddPanel).Visibility_Lambda([]() {
    return isSignedIn() ? EVisibility::Visible : EVisibility::Collapsed;
  });
}

TSharedRef<SWidget> CesiumPanel::ConnectionStatus() {
  return SNew(SHeader)
      .Visibility_Lambda([]() {
        return isSignedIn() ? EVisibility::Visible : EVisibility::Collapsed;
      })
      .HAlign(EHorizontalAlignment::HAlign_Right)
      .Content()[SNew(STextBlock).Text_Lambda([]() {
        if (FCesiumEditorModule::ion().refreshProfileIfNeeded()) {
          auto& profile = FCesiumEditorModule::ion().getProfile();
          std::string s = "Connected to Cesium ion as " + profile.username;
          return FText::FromString(utf8_to_wstr(s));
        } else {
          return FText::FromString(TEXT("Loading user information..."));
        }
      })];
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

void CesiumPanel::addBlankTileset() {
  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

  GEditor->AddActor(
      pCurrentLevel,
      ACesium3DTileset::StaticClass(),
      FTransform(),
      false,
      RF_Public | RF_Standalone | RF_Transactional);
}

void CesiumPanel::accessToken() {}

void CesiumPanel::signOut() { FCesiumEditorModule::ion().disconnect(); }

void CesiumPanel::openDocumentation() {
  FPlatformProcess::LaunchURL(
      TEXT("https://cesium.com/docs"),
      NULL,
      NULL);
}

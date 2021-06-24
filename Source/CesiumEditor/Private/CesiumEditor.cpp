// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumEditor.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTileset.h"
#include "CesiumCommands.h"
#include "CesiumIonPanel.h"
#include "CesiumIonRasterOverlay.h"
#include "CesiumPanel.h"
#include "ClassIconFinder.h"
#include "Editor.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "EngineUtils.h"
#include "Framework/Docking/LayoutExtender.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Interfaces/IPluginManager.h"
#include "LevelEditor.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "UnrealAssetAccessor.h"
#include "UnrealTaskProcessor.h"

IMPLEMENT_MODULE(FCesiumEditorModule, CesiumEditor)
DEFINE_LOG_CATEGORY(LogCesiumEditor);

#define IMAGE_BRUSH(RelativePath, ...)                                         \
  FSlateImageBrush(                                                            \
      FCesiumEditorModule::InContent(RelativePath, ".png"),                    \
      __VA_ARGS__)

FString FCesiumEditorModule::InContent(
    const FString& RelativePath,
    const ANSICHAR* Extension) {
  static FString ContentDir = IPluginManager::Get()
                                  .FindPlugin(TEXT("CesiumForUnreal"))
                                  ->GetContentDir();
  return (ContentDir / RelativePath) + Extension;
}

TSharedPtr<FSlateStyleSet> FCesiumEditorModule::StyleSet = nullptr;
FCesiumEditorModule* FCesiumEditorModule::_pModule = nullptr;

namespace {
/**
 * Register an icon in the StyleSet, using the given property
 * name and relative resource path.
 *
 * This will register the icon once with a default size of
 * 40x40, and once under the same name, extended by the
 * suffix `".Small"`, with a size of 20x20, which will be
 * used when the "useSmallToolbarIcons" editor preference
 * was enabled.
 *
 * @param styleSet The style set
 * @param The property name
 * @param The resource path
 */
void registerIcon(
    TSharedPtr<FSlateStyleSet>& styleSet,
    const FString& propertyName,
    const FString& relativePath) {
  const FVector2D Icon40x40(40.0f, 40.0f);
  const FVector2D Icon20x20(20.0f, 20.0f);
  styleSet->Set(FName(propertyName), new IMAGE_BRUSH(relativePath, Icon40x40));
  styleSet->Set(
      FName(propertyName + ".Small"),
      new IMAGE_BRUSH(relativePath, Icon20x20));
}
} // namespace

void FCesiumEditorModule::StartupModule() {
  static CesiumAsync::AsyncSystem asyncSystem(
      std::make_shared<UnrealTaskProcessor>());

  _pModule = this;

  IModuleInterface::StartupModule();

  auto pAssetAccessor = std::make_shared<UnrealAssetAccessor>();

  this->_pIonSession =
      std::make_shared<CesiumIonSession>(asyncSystem, pAssetAccessor);
  this->_pIonSession->resume();

  // Only register style once
  if (!StyleSet.IsValid()) {
    const FVector2D Icon16x16(16.0f, 16.0f);
    const FVector2D Icon40x40(40.0f, 40.0f);
    const FVector2D Icon64x64(64.0f, 64.0f);

    StyleSet = MakeShareable(new FSlateStyleSet("CesiumStyleSet"));
    StyleSet->Set(
        "Cesium.MenuIcon",
        new IMAGE_BRUSH(TEXT("Cesium-icon-16x16"), Icon16x16));

    // Give Cesium Actors a Cesium icon in the editor
    StyleSet->Set(
        "ClassIcon.Cesium3DTileset",
        new IMAGE_BRUSH(TEXT("Cesium-icon-16x16"), Icon16x16));
    StyleSet->Set(
        "ClassThumbnail.Cesium3DTileset",
        new IMAGE_BRUSH(TEXT("Cesium-64x64"), Icon64x64));
    StyleSet->Set(
        "ClassIcon.CesiumGeoreference",
        new IMAGE_BRUSH(TEXT("Cesium-icon-16x16"), Icon16x16));
    StyleSet->Set(
        "ClassThumbnail.CesiumGeoreference",
        new IMAGE_BRUSH(TEXT("Cesium-64x64"), Icon64x64));

    registerIcon(
        StyleSet,
        "Cesium.Common.AddFromIon",
        "FontAwesome/plus-solid");
    registerIcon(
        StyleSet,
        "Cesium.Common.UploadToIon",
        "FontAwesome/cloud-upload-alt-solid");
    registerIcon(
        StyleSet,
        "Cesium.Common.AddBlankTileset",
        "FontAwesome/globe-solid");
    registerIcon(
        StyleSet,
        "Cesium.Common.AccessToken",
        "FontAwesome/key-solid");
    registerIcon(
        StyleSet,
        "Cesium.Common.SignOut",
        "FontAwesome/sign-out-alt-solid");
    registerIcon(
        StyleSet,
        "Cesium.Common.OpenDocumentation",
        "FontAwesome/book-reader-solid");
    registerIcon(
        StyleSet,
        "Cesium.Common.OpenSupport",
        "FontAwesome/hands-helping-solid");
    registerIcon(StyleSet, "Cesium.Common.OpenCesiumPanel", "Cesium-64x64");

    StyleSet->Set(
        "Cesium.Logo",
        new IMAGE_BRUSH(
            "Cesium-for-Unreal-Logo-Micro-BlackV",
            FVector2D(222.0, 200.0f)));

    StyleSet->Set(
        "WelcomeText",
        FTextBlockStyle()
            .SetColorAndOpacity(FSlateColor::UseForeground())
            .SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14)));

    StyleSet->Set(
        "Heading",
        FTextBlockStyle()
            .SetColorAndOpacity(FSlateColor::UseForeground())
            .SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12)));

    StyleSet->Set(
        "AssetDetailsFieldHeader",
        FTextBlockStyle()
            .SetColorAndOpacity(FSlateColor::UseForeground())
            .SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11)));

    StyleSet->Set(
        "AssetDetailsFieldValue",
        FTextBlockStyle()
            .SetColorAndOpacity(FSlateColor::UseForeground())
            .SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 9)));

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
  }

  FCesiumCommands::Register();

  FGlobalTabmanager::Get()
      ->RegisterNomadTabSpawner(
          TEXT("Cesium"),
          FOnSpawnTab::CreateRaw(this, &FCesiumEditorModule::SpawnCesiumTab))
      .SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
      .SetDisplayName(FText::FromString(TEXT("Cesium")))
      .SetTooltipText(FText::FromString(TEXT("Cesium")))
      .SetIcon(FSlateIcon(TEXT("CesiumStyleSet"), TEXT("Cesium.MenuIcon")));

  FGlobalTabmanager::Get()
      ->RegisterNomadTabSpawner(
          TEXT("CesiumIon"),
          FOnSpawnTab::CreateRaw(
              this,
              &FCesiumEditorModule::SpawnCesiumIonAssetBrowserTab))
      .SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
      .SetDisplayName(FText::FromString(TEXT("Cesium ion Assets")))
      .SetTooltipText(FText::FromString(TEXT("Cesium ion Assets")))
      .SetIcon(FSlateIcon(TEXT("CesiumStyleSet"), TEXT("Cesium.MenuIcon")));

  FLevelEditorModule* pLevelEditorModule =
      FModuleManager::GetModulePtr<FLevelEditorModule>(
          FName(TEXT("LevelEditor")));
  if (pLevelEditorModule) {
    pLevelEditorModule->OnRegisterLayoutExtensions().AddLambda(
        [](FLayoutExtender& extender) {
          extender.ExtendLayout(
              FTabId("PlacementBrowser"),
              ELayoutExtensionPosition::After,
              FTabManager::FTab(FName("Cesium"), ETabState::OpenedTab));
          extender.ExtendLayout(
              FTabId("OutputLog"),
              ELayoutExtensionPosition::Before,
              FTabManager::FTab(FName("CesiumIon"), ETabState::ClosedTab));
        });

    TSharedRef<FUICommandList> toolbarCommandList =
        MakeShared<FUICommandList>();

    toolbarCommandList->MapAction(
        FCesiumCommands::Get().OpenCesiumPanel,
        FExecuteAction::CreateLambda([]() {
          FLevelEditorModule* pLevelEditorModule =
              FModuleManager::GetModulePtr<FLevelEditorModule>(
                  FName(TEXT("LevelEditor")));
          TSharedPtr<FTabManager> pTabManager =
              pLevelEditorModule
                  ? pLevelEditorModule->GetLevelEditorTabManager()
                  : FGlobalTabmanager::Get();
          pTabManager->TryInvokeTab(FTabId(TEXT("Cesium")));
        }));

    TSharedPtr<FExtender> pToolbarExtender = MakeShared<FExtender>();
    pToolbarExtender->AddToolBarExtension(
        "Settings",
        EExtensionHook::After,
        toolbarCommandList,
        FToolBarExtensionDelegate::CreateLambda([](FToolBarBuilder& builder) {
          builder.BeginSection("Cesium");
          builder.AddToolBarButton(FCesiumCommands::Get().OpenCesiumPanel);
          builder.EndSection();
        }));
    pLevelEditorModule->GetToolBarExtensibilityManager()->AddExtender(
        pToolbarExtender);
  }
}

void FCesiumEditorModule::ShutdownModule() {
  FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TEXT("Cesium"));
  FCesiumCommands::Unregister();
  IModuleInterface::ShutdownModule();

  _pModule = nullptr;
}

TSharedRef<SDockTab>
FCesiumEditorModule::SpawnCesiumTab(const FSpawnTabArgs& TabSpawnArgs) {
  TSharedRef<SDockTab> SpawnedTab =
      SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(CesiumPanel)];

  return SpawnedTab;
}

TSharedRef<SDockTab> FCesiumEditorModule::SpawnCesiumIonAssetBrowserTab(
    const FSpawnTabArgs& TabSpawnArgs) {
  TSharedRef<SDockTab> SpawnedTab =
      SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(CesiumIonPanel)];

  return SpawnedTab;
}

TSharedPtr<FSlateStyleSet> FCesiumEditorModule::GetStyle() { return StyleSet; }

const FName& FCesiumEditorModule::GetStyleSetName() {
  return StyleSet->GetStyleSetName();
}

ACesium3DTileset* FCesiumEditorModule::FindFirstTilesetSupportingOverlays() {
  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

  for (TActorIterator<ACesium3DTileset> it(pCurrentWorld); it; ++it) {
    const Cesium3DTiles::Tileset* pTileset = it->GetTileset();
    if (pTileset && pTileset->supportsRasterOverlays()) {
      return *it;
    }
  }

  return nullptr;
}

ACesium3DTileset*
FCesiumEditorModule::FindFirstTilesetWithAssetID(int64_t assetID) {
  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

  ACesium3DTileset* pTilesetActor = nullptr;

  for (TActorIterator<ACesium3DTileset> it(pCurrentWorld); !pTilesetActor && it;
       ++it) {
    const Cesium3DTiles::Tileset* pTileset = it->GetTileset();
    if (pTileset && pTileset->getIonAssetID() == assetID) {
      return *it;
    }
  }

  return nullptr;
}

ACesium3DTileset*
FCesiumEditorModule::CreateTileset(const std::string& name, int64_t assetID) {
  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

  AActor* pNewActor = GEditor->AddActor(
      pCurrentLevel,
      ACesium3DTileset::StaticClass(),
      FTransform(),
      false,
      RF_Public | RF_Transactional);
  ACesium3DTileset* pTilesetActor = Cast<ACesium3DTileset>(pNewActor);
  pTilesetActor->SetActorLabel(UTF8_TO_TCHAR(name.c_str()));
  pTilesetActor->SetIonAssetID(assetID);
  pTilesetActor->SetIonAccessToken(UTF8_TO_TCHAR(
      FCesiumEditorModule::ion().getAssetAccessToken().token.c_str()));

  return pTilesetActor;
}

UCesiumIonRasterOverlay* FCesiumEditorModule::AddOverlay(
    ACesium3DTileset* pTilesetActor,
    const std::string& name,
    int64_t assetID) {
  // Remove any existing overlays and add the new one.
  // TODO: ideally we wouldn't remove the old overlays but the number of overlay
  // textures we can support is currently very limited.
  TArray<UCesiumRasterOverlay*> rasterOverlays;
  pTilesetActor->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

  for (UCesiumRasterOverlay* pOverlay : rasterOverlays) {
    pOverlay->DestroyComponent(false);
  }

  UCesiumIonRasterOverlay* pOverlay = NewObject<UCesiumIonRasterOverlay>(
      pTilesetActor,
      FName(name.c_str()),
      RF_Public | RF_Transactional);
  pOverlay->IonAssetID = assetID;
  pOverlay->IonAccessToken = UTF8_TO_TCHAR(
      FCesiumEditorModule::ion().getAssetAccessToken().token.c_str());
  pOverlay->SetActive(true);
  pOverlay->OnComponentCreated();

  pTilesetActor->AddInstanceComponent(pOverlay);

  return pOverlay;
}

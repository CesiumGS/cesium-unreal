// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumEditor.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTileset.h"
#include "CesiumCommands.h"
#include "CesiumIonPanel.h"
#include "CesiumIonRasterOverlay.h"
#include "CesiumIonTokenTroubleshooting.h"
#include "CesiumPanel.h"
#include "CesiumRuntime.h"
#include "CesiumSunSky.h"
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

constexpr int MaximumOverlaysWithDefaultMaterial = 3;

IMPLEMENT_MODULE(FCesiumEditorModule, CesiumEditor)
DEFINE_LOG_CATEGORY(LogCesiumEditor);

#define IMAGE_BRUSH(RelativePath, ...)                                         \
  FSlateImageBrush(                                                            \
      FCesiumEditorModule::InContent(RelativePath, ".png"),                    \
      __VA_ARGS__)

#define BOX_BRUSH(RelativePath, ...)                                           \
  FSlateBoxBrush(                                                              \
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
/**
 * Create a slate box brush that can be used as the
 * normal-, hovered-, or pressed-brush for a button,
 * based on a resource with the given name, that
 * contains a slate box image with a margin of 4 pixels.
 *
 * @param name The name of the image (without extension, PNG is assumed)
 * @param color The color used for "dyeing" the image
 * @return The box brush
 */
FSlateBoxBrush
createButtonBoxBrush(const FString& name, const FLinearColor& color) {
  return BOX_BRUSH(name, FMargin(4 / 16.0f), color);
}

} // namespace

void FCesiumEditorModule::StartupModule() {
  _pModule = this;

  IModuleInterface::StartupModule();

  this->_pIonSession =
      std::make_shared<CesiumIonSession>(getAsyncSystem(), getAssetAccessor());
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

    // Icons for the toolbar. These will be registered with
    // a default size, and a ".Small" suffix for the case
    // that the useSmallToolbarIcons preference is enabled
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
    registerIcon(
        StyleSet,
        "Cesium.Common.OpenTokenSelector",
        "FontAwesome/key-solid");

    StyleSet->Set(
        "Cesium.Common.GreenTick",
        new IMAGE_BRUSH(TEXT("FontAwesome/check-solid"), Icon16x16));
    StyleSet->Set(
        "Cesium.Common.RedX",
        new IMAGE_BRUSH(TEXT("FontAwesome/times-solid"), Icon16x16));

    registerIcon(StyleSet, "Cesium.Common.OpenCesiumPanel", "Cesium-64x64");

    StyleSet->Set(
        "Cesium.Common.Refresh",
        new IMAGE_BRUSH(TEXT("FontAwesome/sync-alt-solid"), Icon16x16));

    StyleSet->Set(
        "Cesium.Logo",
        new IMAGE_BRUSH(
            "Cesium_for_Unreal_light_color_vertical-height150",
            FVector2D(184.0f, 150.0f)));

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
        "BodyBold",
        FTextBlockStyle()
            .SetColorAndOpacity(FSlateColor::UseForeground())
            .SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 9)));

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

    const FLinearColor CesiumButtonLighter(0.16863f, 0.52941f, 0.76863f, 1.0f);
    const FLinearColor CesiumButton(0.07059f, 0.35686f, 0.59216f, 1.0f);
    const FLinearColor CesiumButtonDarker(0.05490f, 0.29412f, 0.45882f, 1.0f);
    const FButtonStyle CesiumButtonStyle =
        FButtonStyle()
            .SetNormalPadding(FMargin(10, 5, 10, 5))
            .SetPressedPadding(FMargin(10, 5, 10, 5))
            .SetNormal(createButtonBoxBrush("CesiumButton", CesiumButton))
            .SetHovered(
                createButtonBoxBrush("CesiumButton", CesiumButtonLighter))
            .SetPressed(
                createButtonBoxBrush("CesiumButton", CesiumButtonDarker));
    StyleSet->Set("CesiumButton", CesiumButtonStyle);

    const FTextBlockStyle CesiumButtonTextStyle =
        FTextBlockStyle()
            .SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
            .SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
    StyleSet->Set("CesiumButtonText", CesiumButtonTextStyle);

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

  this->_tilesetLoadFailureSubscription = OnCesium3DTilesetLoadFailure.AddRaw(
      this,
      &FCesiumEditorModule::OnTilesetLoadFailure);
  this->_rasterOverlayLoadFailureSubscription =
      OnCesiumRasterOverlayLoadFailure.AddRaw(
          this,
          &FCesiumEditorModule::OnRasterOverlayLoadFailure);

  this->_tilesetIonTroubleshootingSubscription =
      OnCesium3DTilesetIonTroubleshooting.AddRaw(
          this,
          &FCesiumEditorModule::OnTilesetIonTroubleshooting);
  this->_rasterOverlayIonTroubleshootingSubscription =
      OnCesiumRasterOverlayIonTroubleshooting.AddRaw(
          this,
          &FCesiumEditorModule::OnRasterOverlayIonTroubleshooting);
}

void FCesiumEditorModule::ShutdownModule() {
  if (this->_tilesetLoadFailureSubscription.IsValid()) {
    OnCesium3DTilesetLoadFailure.Remove(this->_tilesetLoadFailureSubscription);
    this->_tilesetLoadFailureSubscription.Reset();
  }
  if (this->_rasterOverlayLoadFailureSubscription.IsValid()) {
    OnCesiumRasterOverlayLoadFailure.Remove(
        this->_rasterOverlayLoadFailureSubscription);
    this->_rasterOverlayLoadFailureSubscription.Reset();
  }
  if (this->_tilesetIonTroubleshootingSubscription.IsValid()) {
    OnCesium3DTilesetIonTroubleshooting.Remove(
        this->_tilesetIonTroubleshootingSubscription);
    this->_tilesetIonTroubleshootingSubscription.Reset();
  }
  if (this->_rasterOverlayIonTroubleshootingSubscription.IsValid()) {
    OnCesiumRasterOverlayIonTroubleshooting.Remove(
        this->_rasterOverlayIonTroubleshootingSubscription);
    this->_rasterOverlayIonTroubleshootingSubscription.Reset();
  }
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

void FCesiumEditorModule::OnTilesetLoadFailure(
    const FCesium3DTilesetLoadFailureDetails& details) {
  if (!details.Tileset.IsValid()) {
    return;
  }

  // Don't pop a troubleshooting panel over a game world (including
  // Play-In-Editor).
  if (details.Tileset->GetWorld()->IsGameWorld()) {
    return;
  }

  FLevelEditorModule* pLevelEditorModule =
      FModuleManager::GetModulePtr<FLevelEditorModule>(
          FName(TEXT("LevelEditor")));
  if (pLevelEditorModule) {
    pLevelEditorModule->GetLevelEditorTabManager()->TryInvokeTab(
        FTabId("OutputLog"));
  }

  // Check for a 401 connecting to Cesium ion, which means the token is invalid
  // (or perhaps the asset ID is). Also check for a 404, because ion returns 404
  // when the token is valid but not authorized for the asset.
  if (details.Type == ECesium3DTilesetLoadType::CesiumIon &&
      (details.HttpStatusCode == 401 || details.HttpStatusCode == 404)) {
    CesiumIonTokenTroubleshooting::Open(details.Tileset.Get(), true);
  }
}

void FCesiumEditorModule::OnRasterOverlayLoadFailure(
    const FCesiumRasterOverlayLoadFailureDetails& details) {
  if (!details.Overlay.IsValid()) {
    return;
  }

  // Don't pop a troubleshooting panel over a game world (including
  // Play-In-Editor).
  if (details.Overlay->GetWorld()->IsGameWorld()) {
    return;
  }

  FLevelEditorModule* pLevelEditorModule =
      FModuleManager::GetModulePtr<FLevelEditorModule>(
          FName(TEXT("LevelEditor")));
  if (pLevelEditorModule) {
    pLevelEditorModule->GetLevelEditorTabManager()->TryInvokeTab(
        FTabId("OutputLog"));
  }

  // Check for a 401 connecting to Cesium ion, which means the token is invalid
  // (or perhaps the asset ID is). Also check for a 404, because ion returns 404
  // when the token is valid but not authorized for the asset.
  if (details.Type == ECesiumRasterOverlayLoadType::CesiumIon &&
      (details.HttpStatusCode == 401 || details.HttpStatusCode == 404)) {
    CesiumIonTokenTroubleshooting::Open(details.Overlay.Get(), true);
  }
}

void FCesiumEditorModule::OnTilesetIonTroubleshooting(
    ACesium3DTileset* pTileset) {
  CesiumIonTokenTroubleshooting::Open(pTileset, false);
}

void FCesiumEditorModule::OnRasterOverlayIonTroubleshooting(
    UCesiumRasterOverlay* pOverlay) {
  CesiumIonTokenTroubleshooting::Open(pOverlay, false);
}

TSharedPtr<FSlateStyleSet> FCesiumEditorModule::GetStyle() { return StyleSet; }

const FName& FCesiumEditorModule::GetStyleSetName() {
  return StyleSet->GetStyleSetName();
}

ACesium3DTileset* FCesiumEditorModule::FindFirstTilesetSupportingOverlays() {
  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

  for (TActorIterator<ACesium3DTileset> it(pCurrentWorld); it; ++it) {
    const Cesium3DTilesSelection::Tileset* pTileset = it->GetTileset();
    if (pTileset) {
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
    const Cesium3DTilesSelection::Tileset* pTileset = it->GetTileset();
    if (pTileset && it->GetIonAssetID() == assetID) {
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
  if (assetID != -1) {
    pTilesetActor->SetIonAssetID(assetID);
  }
  return pTilesetActor;
}

UCesiumIonRasterOverlay* FCesiumEditorModule::AddOverlay(
    ACesium3DTileset* pTilesetActor,
    const std::string& name,
    int64_t assetID) {
  // Remove an existing component with the same name but different types.
  // This is necessary because UE will die immediately if we create two
  // components with the same name.
  FName newName = FName(name.c_str());
  UObject* pExisting = static_cast<UObject*>(
      FindObjectWithOuter(pTilesetActor, nullptr, newName));
  if (pExisting) {
    UCesiumRasterOverlay* pCesiumOverlay =
        Cast<UCesiumRasterOverlay>(pExisting);
    if (pCesiumOverlay) {
      pCesiumOverlay->DestroyComponent();
    } else {
      // There's some object using our name, but it's not ours.
      // We could do complicated things here, but this should be a very uncommon
      // scenario so let's just log.
      UE_LOG(
          LogCesiumEditor,
          Warning,
          TEXT(
              "Cannot create raster overlay component %s because the name is already in use."),
          *newName.ToString());
    }
  }

  // Find the first available `OverlayN` MaterialLayerKey.
  TArray<UCesiumRasterOverlay*> rasterOverlays;
  pTilesetActor->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

  FString overlayKey = TEXT("Overlay0");
  auto materialLayerKeyMatches = [&newName,
                                  &overlayKey](UCesiumRasterOverlay* pOverlay) {
    return pOverlay->MaterialLayerKey == overlayKey;
  };

  int i = 0;
  while (rasterOverlays.FindByPredicate(materialLayerKeyMatches)) {
    ++i;
    overlayKey = FString(TEXT("Overlay")) + FString::FromInt(i);
  }

  UCesiumIonRasterOverlay* pOverlay = NewObject<UCesiumIonRasterOverlay>(
      pTilesetActor,
      FName(name.c_str()),
      RF_Public | RF_Transactional);
  pOverlay->MaterialLayerKey = overlayKey;
  pOverlay->IonAssetID = assetID;
  pOverlay->SetActive(true);
  pOverlay->OnComponentCreated();

  pTilesetActor->AddInstanceComponent(pOverlay);

  if (i >= MaximumOverlaysWithDefaultMaterial) {
    UE_LOG(
        LogCesiumEditor,
        Warning,
        TEXT(
            "The default material only supports up to %d raster overlays, and your tileset is now using %d, so the extra overlays will be ignored. Consider creating a custom Material Instance with support for more overlays."),
        MaximumOverlaysWithDefaultMaterial,
        i + 1);
  }

  return pOverlay;
}

UCesiumIonRasterOverlay* FCesiumEditorModule::AddBaseOverlay(
    ACesium3DTileset* pTilesetActor,
    const std::string& name,
    int64_t assetID) {
  // Remove Overlay0 (if it exists) and add the new one.
  TArray<UCesiumRasterOverlay*> rasterOverlays;
  pTilesetActor->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

  for (UCesiumRasterOverlay* pOverlay : rasterOverlays) {
    if (pOverlay->MaterialLayerKey == TEXT("Overlay0")) {
      pOverlay->DestroyComponent(false);
    }
  }

  return FCesiumEditorModule::AddOverlay(pTilesetActor, name, assetID);
}

namespace {
AActor* GetFirstCurrentLevelActorWithClass(UClass* pActorClass) {
  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();
  for (TActorIterator<AActor> it(pCurrentWorld); it; ++it) {
    if (it->GetClass() == pActorClass) {
      return *it;
    }
  }
  return nullptr;
}

/**
 * Returns whether the current level of the edited world contains
 * any actor with the given class.
 *
 * @param actorClass The expected class
 * @return Whether such an actor could be found
 */
bool CurrentLevelContainsActorWithClass(UClass* pActorClass) {
  return GetFirstCurrentLevelActorWithClass(pActorClass) != nullptr;
}
/**
 * Tries to spawn an actor with the given class, with all
 * default parameters, in the current level of the edited world.
 *
 * @param actorClass The class
 * @return The resulting actor, or `nullptr` if the actor
 * could not be spawned.
 */
AActor* SpawnActorWithClass(UClass* actorClass) {
  if (!actorClass) {
    return nullptr;
  }
  UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
  AActor* pNewActor = pCurrentWorld->SpawnActor<AActor>(actorClass);
  return pNewActor;
}
} // namespace

AActor* FCesiumEditorModule::GetCurrentLevelCesiumSunSky() {
  return GetFirstCurrentLevelActorWithClass(GetCesiumSunSkyClass());
}

AActor* FCesiumEditorModule::GetCurrentLevelDynamicPawn() {
  return GetFirstCurrentLevelActorWithClass(GetDynamicPawnBlueprintClass());
}

AActor* FCesiumEditorModule::SpawnCesiumSunSky() {
  return SpawnActorWithClass(GetCesiumSunSkyClass());
}

AActor* FCesiumEditorModule::SpawnDynamicPawn() {
  return SpawnActorWithClass(GetDynamicPawnBlueprintClass());
}

UClass* FCesiumEditorModule::GetCesiumSunSkyClass() {
  return ACesiumSunSky::StaticClass();
}

UClass* FCesiumEditorModule::GetDynamicPawnBlueprintClass() {
  static UClass* pResult = nullptr;

  if (!pResult) {
    pResult = LoadClass<AActor>(
        nullptr,
        TEXT("/CesiumForUnreal/DynamicPawn.DynamicPawn_C"));
    if (!pResult) {
      UE_LOG(
          LogCesiumEditor,
          Warning,
          TEXT("Could not load /CesiumForUnreal/DynamicPawn.DynamicPawn_C"));
    }
  }

  return pResult;
}

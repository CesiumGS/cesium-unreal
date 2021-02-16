#include "CesiumEditor.h"
#include "Framework/Docking/TabManager.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "Styling/SlateStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "ClassIconFinder.h"
#include "CesiumPanel.h"
#include "CesiumIonPanel.h"
#include "CesiumCommands.h"
#include "UnrealAssetAccessor.h"
#include "UnrealTaskProcessor.h"
#include "ACesium3DTileset.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Cesium3DTiles/Tileset.h"
#include "UnrealConversions.h"
#include "CesiumIonRasterOverlay.h"

IMPLEMENT_MODULE(FCesiumEditorModule, CesiumEditor)

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( FCesiumEditorModule::InContent( RelativePath, ".png" ), __VA_ARGS__ )

FString FCesiumEditorModule::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
    static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("Cesium"))->GetContentDir();
    return (ContentDir / RelativePath) + Extension;
}

TSharedPtr< FSlateStyleSet > FCesiumEditorModule::StyleSet = nullptr;
FCesiumEditorModule* FCesiumEditorModule::_pModule = nullptr;

void FCesiumEditorModule::StartupModule()
{
    _pModule = this;

    IModuleInterface::StartupModule();

    this->ionDetails.pAsyncSystem = std::make_unique<CesiumAsync::AsyncSystem>(
        std::make_shared<UnrealAssetAccessor>(),
        std::make_shared<UnrealTaskProcessor>()
    );

    // Only register style once
    if (!StyleSet.IsValid())
    {
        const FVector2D Icon16x16(16.0f, 16.0f);
        const FVector2D Icon40x40(40.0f, 40.0f);
        const FVector2D Icon64x64(64.0f, 64.0f);

        StyleSet = MakeShareable(new FSlateStyleSet("CesiumStyleSet"));
        StyleSet->Set("Cesium.MenuIcon", new IMAGE_BRUSH(TEXT("Cesium-icon-16x16"), Icon16x16));

        // Give Cesium Actors a Cesium icon in the editor
        StyleSet->Set("ClassIcon.Cesium3DTileset", new IMAGE_BRUSH(TEXT("Cesium-icon-16x16"), Icon16x16));
        StyleSet->Set("ClassThumbnail.Cesium3DTileset", new IMAGE_BRUSH(TEXT("Cesium-64x64"), Icon64x64));
        StyleSet->Set("ClassIcon.CesiumGeoreference", new IMAGE_BRUSH(TEXT("Cesium-icon-16x16"), Icon16x16));
        StyleSet->Set("ClassThumbnail.CesiumGeoreference", new IMAGE_BRUSH(TEXT("Cesium-64x64"), Icon64x64));

        StyleSet->Set("Cesium.Common.AddFromIon", new IMAGE_BRUSH("NounProject/noun_add_on_cloud_724752", Icon40x40));
        StyleSet->Set("Cesium.Common.UploadToIon", new IMAGE_BRUSH("NounProject/noun_Cloud_Upload_827113", Icon40x40));
        StyleSet->Set("Cesium.Common.AddBlankTileset", new IMAGE_BRUSH("NounProject/noun_edit_838988", Icon40x40));
        StyleSet->Set("Cesium.Common.AccessToken", new IMAGE_BRUSH("NounProject/noun_Key_679682", Icon40x40));
        StyleSet->Set("Cesium.Common.SignOut", new IMAGE_BRUSH("NounProject/noun_sign_out_538366", Icon40x40));

        StyleSet->Set("Cesium.Logo", new IMAGE_BRUSH("CESIUM-4-UNREAL-LOGOS_RGB_CESIUM-4-UNREAL-BlackV", FVector2D(222.0, 200.0f)));

        StyleSet->Set("WelcomeText", FTextBlockStyle()
            .SetColorAndOpacity(FSlateColor::UseForeground())
            .SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14))
        );

        StyleSet->Set("Heading", FTextBlockStyle()
            .SetColorAndOpacity(FSlateColor::UseForeground())
            .SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 12))
        );

        StyleSet->Set("AssetDetailsFieldHeader", FTextBlockStyle()
            .SetColorAndOpacity(FSlateColor::UseForeground())
            .SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11))
        );

        StyleSet->Set("AssetDetailsFieldValue", FTextBlockStyle()
            .SetColorAndOpacity(FSlateColor::UseForeground())
            .SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 9))
        );

        FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
    }

    FCesiumCommands::Register();

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TEXT("Cesium"), FOnSpawnTab::CreateRaw(this, &FCesiumEditorModule::SpawnCesiumTab))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
        .SetDisplayName(FText::FromString(TEXT("Cesium")))
        .SetTooltipText(FText::FromString(TEXT("Cesium")))
        .SetIcon(FSlateIcon(TEXT("CesiumStyleSet"), TEXT("Cesium.MenuIcon")));

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TEXT("CesiumIon"), FOnSpawnTab::CreateRaw(this, &FCesiumEditorModule::SpawnCesiumIonAssetBrowserTab))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
        .SetDisplayName(FText::FromString(TEXT("Cesium ion Assets")))
        .SetTooltipText(FText::FromString(TEXT("Cesium ion Assets")))
        .SetIcon(FSlateIcon(TEXT("CesiumStyleSet"), TEXT("Cesium.MenuIcon")));
}

void FCesiumEditorModule::ShutdownModule()
{
    this->ionDetails.pAsyncSystem.reset();

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TEXT("Cesium"));
    FCesiumCommands::Unregister();
    IModuleInterface::ShutdownModule();

    _pModule = nullptr;
}

TSharedRef<SDockTab> FCesiumEditorModule::SpawnCesiumTab(const FSpawnTabArgs& TabSpawnArgs)
{
    TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(CesiumPanel)
        ];

    return SpawnedTab;
}

TSharedRef<SDockTab> FCesiumEditorModule::SpawnCesiumIonAssetBrowserTab(const FSpawnTabArgs& TabSpawnArgs)
{
    TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(CesiumIonPanel)
        ];

    return SpawnedTab;
}

TSharedPtr<FSlateStyleSet> FCesiumEditorModule::GetStyle() {
    return StyleSet;
}

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

ACesium3DTileset* FCesiumEditorModule::FindFirstTilesetWithAssetID(int64_t assetID) {
    UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
    ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

    ACesium3DTileset* pTilesetActor = nullptr;

    for (TActorIterator<ACesium3DTileset> it(pCurrentWorld); !pTilesetActor && it; ++it) {
        const Cesium3DTiles::Tileset* pTileset = it->GetTileset();
        if (pTileset && pTileset->getIonAssetID() == assetID) {
            return *it;
        }
    }

    return nullptr;
}

ACesium3DTileset* FCesiumEditorModule::CreateTileset(const std::string& name, int64_t assetID) {
    UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
    ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

    AActor* pNewActor = GEditor->AddActor(pCurrentLevel, ACesium3DTileset::StaticClass(), FTransform(), false, RF_Public | RF_Transactional);
    ACesium3DTileset* pTilesetActor = Cast<ACesium3DTileset>(pNewActor);
    pTilesetActor->SetActorLabel(utf8_to_wstr(name));
    pTilesetActor->IonAssetID = assetID;
    pTilesetActor->IonAccessToken = utf8_to_wstr(FCesiumEditorModule::ion().token);

    return pTilesetActor;
}

UCesiumIonRasterOverlay* FCesiumEditorModule::AddOverlay(ACesium3DTileset* pTilesetActor, const std::string& name, int64_t assetID) {
    // Remove any existing overlays and add the new one.
    // TODO: ideally we wouldn't remove the old overlays but the number of overlay textures we can support
    // is currently very limited.
    TArray<UCesiumRasterOverlay*> rasterOverlays;
    pTilesetActor->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

    for (UCesiumRasterOverlay* pOverlay : rasterOverlays) {
        pOverlay->DestroyComponent(false);
    }

    UCesiumIonRasterOverlay* pOverlay = NewObject<UCesiumIonRasterOverlay>(pTilesetActor, FName(utf8_to_wstr(name)), RF_Public | RF_Transactional);
    pOverlay->IonAssetID = assetID;
    pOverlay->IonAccessToken = utf8_to_wstr(FCesiumEditorModule::ion().token);
    pOverlay->SetActive(true);
    pOverlay->OnComponentCreated();

    pTilesetActor->AddInstanceComponent(pOverlay);

    return pOverlay;
}

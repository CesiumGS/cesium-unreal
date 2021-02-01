#include "CesiumEditor.h"
#include "Framework/Docking/TabManager.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "Styling/SlateStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "ClassIconFinder.h"

IMPLEMENT_MODULE(FCesiumEditorModule, CesiumEditor)

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( FCesiumEditorModule::InContent( RelativePath, ".png" ), __VA_ARGS__ )

FString FCesiumEditorModule::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
    static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("Cesium"))->GetContentDir();
    return (ContentDir / RelativePath) + Extension;
}

TSharedPtr< FSlateStyleSet > FCesiumEditorModule::StyleSet = nullptr;

void FCesiumEditorModule::StartupModule()
{
    IModuleInterface::StartupModule();

    // Only register style once
    if (!StyleSet.IsValid())
    {
        const FVector2D Icon16x16(16.0f, 16.0f);
        const FVector2D Icon64x64(64.0f, 64.0f);

        StyleSet = MakeShareable(new FSlateStyleSet("CesiumStyleSet"));
        StyleSet->Set("Cesium.MenuIcon", new IMAGE_BRUSH(TEXT("Cesium-icon-16x16"), Icon16x16));

        // Give Cesium Actors a Cesium icon in the editor
        StyleSet->Set("ClassIcon.Cesium3DTileset", new IMAGE_BRUSH(TEXT("Cesium-icon-16x16"), Icon16x16));
        StyleSet->Set("ClassThumbnail.Cesium3DTileset", new IMAGE_BRUSH(TEXT("Cesium-64x64"), Icon64x64));
        StyleSet->Set("ClassIcon.CesiumGeoreference", new IMAGE_BRUSH(TEXT("Cesium-icon-16x16"), Icon16x16));
        StyleSet->Set("ClassThumbnail.CesiumGeoreference", new IMAGE_BRUSH(TEXT("Cesium-64x64"), Icon64x64));

        FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
    }

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TEXT("Cesium"), FOnSpawnTab::CreateRaw(this, &FCesiumEditorModule::SpawnCesiumTab))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
        .SetDisplayName(FText::FromString(TEXT("Cesium")))
        .SetTooltipText(FText::FromString(TEXT("TODO Cesium tooltip")))
        .SetIcon(FSlateIcon(TEXT("CesiumStyleSet"), TEXT("Cesium.MenuIcon")));
}

void FCesiumEditorModule::ShutdownModule()
{
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TEXT("Cesium"));
    IModuleInterface::ShutdownModule();
}

TSharedRef<SDockTab> FCesiumEditorModule::SpawnCesiumTab(const FSpawnTabArgs& TabSpawnArgs)
{
    throw 1;
}

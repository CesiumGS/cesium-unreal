#include "CesiumEditor.h"
#include "Framework/Docking/TabManager.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"
#include "Styling/SlateStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "ClassIconFinder.h"
#include "CesiumPanel.h"
#include "CesiumCommands.h"
#include "UnrealAssetAccessor.h"
#include "UnrealTaskProcessor.h"

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

    this->_pAsyncSystem = std::make_unique<CesiumAsync::AsyncSystem>(
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

        StyleSet->Set("WelcomeText", FTextBlockStyle()
            .SetColorAndOpacity(FSlateColor::UseForeground())
            .SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14))
        );

        FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
    }

    FCesiumCommands::Register();

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TEXT("Cesium"), FOnSpawnTab::CreateRaw(this, &FCesiumEditorModule::SpawnCesiumTab))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
        .SetDisplayName(FText::FromString(TEXT("Cesium")))
        .SetTooltipText(FText::FromString(TEXT("TODO Cesium tooltip")))
        .SetIcon(FSlateIcon(TEXT("CesiumStyleSet"), TEXT("Cesium.MenuIcon")));
}

void FCesiumEditorModule::ShutdownModule()
{
    this->_pAsyncSystem.reset();

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TEXT("Cesium"));
    FCesiumCommands::Unregister();
    IModuleInterface::ShutdownModule();

    _pModule = nullptr;
}

const std::optional<CesiumIonClient::CesiumIonConnection>& FCesiumEditorModule::getIonConnection() const {
    return this->_ionConnection;
}

void FCesiumEditorModule::setIonConnection(const std::optional<CesiumIonClient::CesiumIonConnection>& newConnection) {
    this->_ionConnection = newConnection;
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

TSharedPtr<FSlateStyleSet> FCesiumEditorModule::GetStyle() {
    return StyleSet;
}

const FName& FCesiumEditorModule::GetStyleSetName() {
    return StyleSet->GetStyleSetName();
}

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/SlateStyle.h"
#include "CesiumIonSession.h"
#include <optional>

class FSpawnTabArgs;
class ACesium3DTileset;
class UCesiumIonRasterOverlay;

class FCesiumEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FString InContent(const FString& RelativePath, const ANSICHAR* Extension);

	static TSharedPtr<FSlateStyleSet> GetStyle();
	static const FName& GetStyleSetName();

	static FCesiumEditorModule* get() { return _pModule; }

	static CesiumIonSession& ion() { assert(_pModule); return *_pModule->_pIonSession; }

	static ACesium3DTileset* FindFirstTilesetSupportingOverlays();
	static ACesium3DTileset* FindFirstTilesetWithAssetID(int64_t assetID);
	static ACesium3DTileset* CreateTileset(const std::string& name, int64_t assetID);
	static UCesiumIonRasterOverlay* AddOverlay(ACesium3DTileset* pTilesetActor, const std::string& name, int64_t assetID);

private:
	TSharedRef<SDockTab> SpawnCesiumTab(const FSpawnTabArgs& TabSpawnArgs);
	TSharedRef<SDockTab> SpawnCesiumIonAssetBrowserTab(const FSpawnTabArgs& TabSpawnArgs);

	std::shared_ptr<CesiumIonSession> _pIonSession;


	static TSharedPtr<FSlateStyleSet> StyleSet;
	static FCesiumEditorModule* _pModule;
};

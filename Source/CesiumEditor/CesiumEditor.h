#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/SlateStyle.h"
#include "CesiumIonClient/CesiumIonConnection.h"
#include <optional>

class FSpawnTabArgs;

class FCesiumEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FString InContent(const FString& RelativePath, const ANSICHAR* Extension);

	static TSharedPtr<FSlateStyleSet> GetStyle();
	static const FName& GetStyleSetName();

	const std::optional<CesiumIonClient::CesiumIonConnection>& getIonConnection() const;
	void setIonConnection(const std::optional<CesiumIonClient::CesiumIonConnection>& newConnection);

	CesiumAsync::AsyncSystem* getAsyncSystem() { return this->_pAsyncSystem.get(); }
	const CesiumAsync::AsyncSystem* getAsyncSystem() const { return this->_pAsyncSystem.get(); }

	static FCesiumEditorModule* get() { return _pModule; }

private:
	TSharedRef<SDockTab> SpawnCesiumTab(const FSpawnTabArgs& TabSpawnArgs);

	std::optional<CesiumIonClient::CesiumIonConnection> _ionConnection;
	std::unique_ptr<CesiumAsync::AsyncSystem> _pAsyncSystem;
	
	static TSharedPtr<FSlateStyleSet> StyleSet;
	static FCesiumEditorModule* _pModule;
};

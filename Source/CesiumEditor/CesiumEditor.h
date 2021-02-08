#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/SlateStyle.h"

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

private:
	TSharedRef<SDockTab> SpawnCesiumTab(const FSpawnTabArgs& TabSpawnArgs);
	
	static TSharedPtr<FSlateStyleSet> StyleSet;
};

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FCesiumCommands : public TCommands<FCesiumCommands>
{
public:
	FCesiumCommands();

    TSharedPtr<FUICommandInfo> AddFromIon;
    TSharedPtr<FUICommandInfo> UploadToIon;
	TSharedPtr<FUICommandInfo> AddBlankTileset;
	TSharedPtr<FUICommandInfo> AccessToken;
	TSharedPtr<FUICommandInfo> SignOut;

	virtual void RegisterCommands() override;
};

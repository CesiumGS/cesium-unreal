// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FCesiumCommands : public TCommands<FCesiumCommands> {
public:
  FCesiumCommands();

  TSharedPtr<FUICommandInfo> AddFromIon;
  TSharedPtr<FUICommandInfo> UploadToIon;
  TSharedPtr<FUICommandInfo> AddBlankTileset;
  TSharedPtr<FUICommandInfo> AccessToken;
  TSharedPtr<FUICommandInfo> SignOut;
  TSharedPtr<FUICommandInfo> OpenDocumentation;
  TSharedPtr<FUICommandInfo> OpenSupport;

  virtual void RegisterCommands() override;
};

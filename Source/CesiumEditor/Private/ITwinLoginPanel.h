// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;

class ITwinLoginPanel : public SCompoundWidget {
  SLATE_BEGIN_ARGS(ITwinLoginPanel) {}
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

private:
  void LaunchBrowserAgain();

  FReply SignIn();
  FReply CancelSignIn();
  FReply CopyAuthorizeUrlToClipboard();
};

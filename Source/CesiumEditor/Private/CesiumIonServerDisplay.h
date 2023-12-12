// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;
class UCesiumIonServer;

class CesiumIonServerDisplay : public SCompoundWidget {
  SLATE_BEGIN_ARGS(CesiumIonServerDisplay) {}
  SLATE_ARGUMENT(UCesiumIonServer*, Server)
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

private:
  void OnBrowseForServer();
};

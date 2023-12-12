// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;
class UCesiumIonServer;

class CesiumIonServerSelector : public SCompoundWidget {
  SLATE_BEGIN_ARGS(CesiumIonServerSelector) {}
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

private:
  TSharedRef<SWidget>
  OnGenerateServerEntry(TWeakObjectPtr<UCesiumIonServer> pServerAsset);

  FText GetServerValueAsText() const;

  void OnServerSelectionChanged(
      TWeakObjectPtr<UCesiumIonServer> InItem,
      ESelectInfo::Type InSeletionInfo);
  void OnBrowseForServer();
};

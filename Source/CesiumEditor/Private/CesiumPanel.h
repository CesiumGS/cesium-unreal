// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;
class UCesiumIonServer;

class CesiumPanel : public SCompoundWidget {
  SLATE_BEGIN_ARGS(CesiumPanel) {}
  SLATE_END_ARGS()

  void Construct(const FArguments& InArgs);

  virtual void Tick(
      const FGeometry& AllottedGeometry,
      const double InCurrentTime,
      const float InDeltaTime) override;

private:
  TSharedRef<SWidget> ServerSelector();
  TSharedRef<SWidget> Toolbar();
  TSharedRef<SWidget> LoginPanel();
  TSharedRef<SWidget> MainIonQuickAddPanel();
  TSharedRef<SWidget> BasicQuickAddPanel();
  TSharedRef<SWidget> ConnectionStatus();
  TSharedRef<SWidget>
  OnGenerateServerEntry(TObjectPtr<UCesiumIonServer> pServer);
  FText GetServerValueAsText() const;
  void OnServerSelectionChanged(
      TObjectPtr<UCesiumIonServer> InItem,
      ESelectInfo::Type InSeletionInfo);

  void addFromIon();
  void uploadToIon();
  void visitIon();
  void signOut();
  void openDocumentation();
  void openSupport();
  void openTokenSelector();
};

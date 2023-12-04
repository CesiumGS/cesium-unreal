// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;
class UCesiumIonServer;
class IonQuickAddPanel;

class CesiumPanel : public SCompoundWidget {
  SLATE_BEGIN_ARGS(CesiumPanel) {}
  SLATE_END_ARGS()

  CesiumPanel();
  virtual ~CesiumPanel();
  void Construct(const FArguments& InArgs);

  virtual void Tick(
      const FGeometry& AllottedGeometry,
      const double InCurrentTime,
      const float InDeltaTime) override;

  void Refresh();

  void Subscribe(UCesiumIonServer* pNewServer);
  void OnServerChanged();

private:
  TSharedRef<SWidget> Toolbar();
  TSharedRef<SWidget> LoginPanel();
  TSharedRef<SWidget> MainIonQuickAddPanel();
  TSharedRef<SWidget> BasicQuickAddPanel();
  TSharedRef<SWidget> Version();

  void addFromIon();
  void uploadToIon();
  void visitIon();
  void signOut();
  void openDocumentation();
  void openSupport();
  void openTokenSelector();

  TSharedPtr<IonQuickAddPanel> _pQuickAddPanel;
  TObjectPtr<UCesiumIonServer> _pLastServer;
  FDelegateHandle _serverChangedDelegateHandle;
};

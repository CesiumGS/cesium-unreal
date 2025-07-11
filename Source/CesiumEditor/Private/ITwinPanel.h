// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;
class UCesiumIonServer;
class IonQuickAddPanel;

class ITwinPanel : public SCompoundWidget {
  SLATE_BEGIN_ARGS(ITwinPanel) {}
  SLATE_END_ARGS()

  ITwinPanel();
  virtual ~ITwinPanel();
  void Construct(const FArguments& InArgs);

  virtual void Tick(
      const FGeometry& AllottedGeometry,
      const double InCurrentTime,
      const float InDeltaTime) override;

  void Refresh();
private:
  TSharedRef<SWidget> Toolbar();
  TSharedRef<SWidget> LoginPanel();
  TSharedRef<SWidget> MainIonQuickAddPanel();
  TSharedRef<SWidget> BasicQuickAddPanel();
  TSharedRef<SWidget> Version();

  void OnConnectionUpdated();
  void OnDefaultsUpdated();

  void addFromIon();
  void uploadToIon();
  void visitIon();
  void signOut();
  void openDocumentation();
  void openSupport();

  TSharedPtr<IonQuickAddPanel> _pQuickAddPanel;
  FDelegateHandle _serverChangedDelegateHandle;
};

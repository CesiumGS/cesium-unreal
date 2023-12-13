// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Widgets/SWindow.h"
#include <optional>
#include <variant>
#include <vector>

class ACesium3DTileset;
class UCesiumRasterOverlay;

using CesiumIonObject = std::variant<
    TWeakObjectPtr<ACesium3DTileset>,
    TWeakObjectPtr<UCesiumRasterOverlay>>;

class CesiumIonTokenTroubleshooting : public SWindow {
  SLATE_BEGIN_ARGS(CesiumIonTokenTroubleshooting) {}
  /**
   * The tileset being troubleshooted.
   */
  SLATE_ARGUMENT(CesiumIonObject, IonObject)

  /** Whether this troubleshooting panel was opened in response to an error,
   * versus opened manually by the user. */
  SLATE_ARGUMENT(bool, TriggeredByError)
  SLATE_END_ARGS()

public:
  static void Open(CesiumIonObject ionObject, bool triggeredByError);
  void Construct(const FArguments& InArgs);

private:
  struct ExistingPanel {
    CesiumIonObject pObject;
    TSharedRef<CesiumIonTokenTroubleshooting> pPanel;
  };

  static std::vector<ExistingPanel> _existingPanels;

  struct TokenState {
    FString name;
    FString token;
    std::optional<bool> isValid;
    std::optional<bool> allowsAccessToAsset;
    std::optional<bool> associatedWithUserAccount;
  };

  CesiumIonObject _pIonObject{};
  TokenState _assetTokenState{};
  TokenState _projectDefaultTokenState{};
  std::optional<bool> _assetExistsInUserAccount;

  TSharedRef<SWidget> createDiagnosticPanel(
      const FString& name,
      const TArray<TSharedRef<SWidget>>& diagnostics);

  TSharedRef<SWidget>
  createTokenPanel(const CesiumIonObject& pIonObject, TokenState& state);

  void addRemedyButton(
      const TSharedRef<SVerticalBox>& pParent,
      const FString& name,
      bool (CesiumIonTokenTroubleshooting::*isAvailableCallback)() const,
      void (CesiumIonTokenTroubleshooting::*clickCallback)());

  bool canConnectToCesiumIon() const;
  void connectToCesiumIon();

  bool canUseProjectDefaultToken() const;
  void useProjectDefaultToken();

  bool canAuthorizeAssetToken() const;
  void authorizeAssetToken();

  bool canAuthorizeProjectDefaultToken() const;
  void authorizeProjectDefaultToken();

  bool canSelectNewProjectDefaultToken() const;
  void selectNewProjectDefaultToken();

  bool canOpenCesiumIon() const;
  void openCesiumIon();

  void authorizeToken(const FString& token, bool removeObjectToken);
};

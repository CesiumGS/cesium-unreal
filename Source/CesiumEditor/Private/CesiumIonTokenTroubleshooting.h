// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Widgets/SWindow.h"
#include <optional>

class ACesium3DTileset;

class CesiumIonTokenTroubleshooting : public SWindow {
  SLATE_BEGIN_ARGS(CesiumIonTokenTroubleshooting) {}
  /**
   * The tileset being troubleshooted.
   */
  SLATE_ARGUMENT(ACesium3DTileset*, Tileset)

  /** Whether this troubleshooting panel was opened in response to an error,
   * versus opened manually by the user. */
  SLATE_ARGUMENT(bool, TriggeredByError)
  SLATE_END_ARGS()

public:
  static void Open(ACesium3DTileset* pTileset, bool triggeredByError);
  void Construct(const FArguments& InArgs);

private:
  static TMap<ACesium3DTileset*, TSharedRef<CesiumIonTokenTroubleshooting>>
      _existingPanels;

  struct TokenState {
    FString name;
    FString token;
    std::optional<bool> isValid;
    std::optional<bool> allowsAccessToAsset;
    std::optional<bool> associatedWithUserAccount;
  };

  ACesium3DTileset* _pTileset = nullptr;
  TokenState _assetTokenState{};
  TokenState _projectDefaultTokenState{};
  std::optional<bool> _assetExistsInUserAccount;

  TSharedRef<SWidget>
  createTokenPanel(ACesium3DTileset* pTileset, TokenState& state);

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

  bool canCreateNewProjectDefaultToken() const;
  void createNewProjectDefaultToken();

  void authorizeToken(const FString& token);
};

// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumAsync/Promise.h"
#include "CesiumAsync/SharedFuture.h"
#include "CesiumIonClient/Token.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SWindow.h"
#include <optional>
#include <string>
#include <variant>

class SelectCesiumIonToken : public SWindow {
  SLATE_BEGIN_ARGS(SelectCesiumIonToken) {}
  SLATE_END_ARGS()

public:
  /**
   * Opens a panel to allow the user to select a new token.
   *
   * @return A future that resolves when the panel is closed. It resolves to the
   * selected token if there was one, or to std::nullopt if the panel was closed
   * without selecting a token.
   */
  static CesiumAsync::SharedFuture<std::optional<CesiumIonClient::Token>>
  SelectNewToken();

  /**
   * Opens a panel to allow the user to select a new token if a project default
   * token is not already set. If the project defalut token _is_ set, the future
   * immediately resolves to the previously-set token.
   *
   * @return A future that resolves when the panel is closed or when it does not
   * need to be opened in the first place. It resolves to the selected token if
   * there was one, or to std::nullopt if the panel was closed without selecting
   * a token.
   */
  static CesiumAsync::SharedFuture<std::optional<CesiumIonClient::Token>>
  SelectTokenIfNecessary();

  SelectCesiumIonToken();
  virtual ~SelectCesiumIonToken();
  void Construct(const FArguments& InArgs);

private:
  enum class TokenSource { Create, UseExisting, Specify };

  struct CreateNewToken {
    FString name;
  };

  struct UseExistingToken {
    Token token;
  };

  struct SpecifyToken {
    FString token;
  };

  void createRadioButton(
      const TSharedRef<SVerticalBox>& pVertical,
      TokenSource& tokenSource,
      TokenSource thisValue,
      const FString& label,
      const TSharedRef<SWidget>& pWidget);
  FReply UseOrCreate();
  void RefreshTokens();
  TSharedRef<SWidget>
  OnGenerateTokenComboBoxEntry(TSharedPtr<CesiumIonClient::Token> pToken);
  FText GetNewTokenName() const;
  void SetNewTokenName(const FText& text);
  void OnSelectExistingToken(
      TSharedPtr<CesiumIonClient::Token> pToken,
      ESelectInfo::Type type);
  FText GetSpecifiedToken() const;
  void SetSpecifiedToken(const FText& text);

  static TSharedPtr<SelectCesiumIonToken> _pExistingPanel;

  std::optional<CesiumAsync::Promise<std::optional<CesiumIonClient::Token>>>
      _promise;
  std::optional<
      CesiumAsync::SharedFuture<std::optional<CesiumIonClient::Token>>>
      _future;

  TokenSource _tokenSource = TokenSource::Create;
  CreateNewToken _createNewToken;
  UseExistingToken _useExistingToken;
  SpecifyToken _specifyToken;
  FDelegateHandle _tokensUpdatedDelegateHandle;
  TArray<TSharedPtr<CesiumIonClient::Token>> _tokens;
  TSharedPtr<SComboBox<TSharedPtr<CesiumIonClient::Token>>> _pTokensCombo;
};

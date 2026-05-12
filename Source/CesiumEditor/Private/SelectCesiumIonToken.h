// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumAsync/AsyncSystem.h"
#include "CesiumIonClient/Token.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SWindow.h"
#include <optional>
#include <string>
#include <variant>

class UCesiumIonServer;
class CesiumIonSession;

class SelectCesiumIonToken : public SWindow {
  SLATE_BEGIN_ARGS(SelectCesiumIonToken) {}
  SLATE_ARGUMENT(UCesiumIonServer*, Server)
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
  SelectNewToken(UCesiumIonServer* pServer);

  /**
   * Opens a panel to allow the user to select a new token if a project default
   * token is not already set. If the project default token _is_ set, the future
   * immediately resolves to the previously-set token.
   *
   * @return A future that resolves when the panel is closed or when it does not
   * need to be opened in the first place. It resolves to the selected token if
   * there was one, or to std::nullopt if the panel was closed without selecting
   * a token.
   */
  static CesiumAsync::Future<std::optional<CesiumIonClient::Token>>
  SelectTokenIfNecessary(UCesiumIonServer* pServer);

  /**
   * Authorizes the project default token to access a list of asset IDs. If the
   * project default token is not set, a panel is opened to allow the token to
   * be selected. Then, if possible, the token is modified to allow access to
   * the list of asset IDs.
   *
   * @param assetIDs The asset IDs to be accessed.
   * @return A future that resolves when the panel is closed or when it does not
   * need to be opened in the first place. It resolves to the selected token if
   * there was one, or to std::nullopt if the panel was closed without selecting
   * a token.
   */
  static CesiumAsync::Future<std::optional<CesiumIonClient::Token>>
  SelectAndAuthorizeToken(
      UCesiumIonServer* pServer,
      const std::vector<int64_t>& assetIDs);

  void Construct(const FArguments& InArgs);

private:
  enum class TokenSource { Create, UseExisting, Specify };

  struct CreateNewToken {
    FString name;
  };

  struct UseExistingToken {
    CesiumIonClient::Token token;
  };

  struct SpecifyToken {
    FString token;
  };

  void createRadioButton(
      const std::shared_ptr<CesiumIonSession>& pSession,
      const TSharedRef<SVerticalBox>& pVertical,
      TokenSource& tokenSource,
      TokenSource thisValue,
      const FString& label,
      bool requiresIonConnection,
      const TSharedRef<SWidget>& pWidget);
  FReply UseOrCreate(std::shared_ptr<CesiumIonSession> pSession);
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
  TWeakObjectPtr<UCesiumIonServer> _pServer;
};

// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "SelectCesiumIonToken.h"
#include "Cesium3DTileset.h"
#include "CesiumEditor.h"
#include "CesiumIonRasterOverlay.h"
#include "CesiumIonServerDisplay.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"
#include "CesiumSourceControl.h"
#include "CesiumUtility/joinToString.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "PropertyCustomizationHelpers.h"
#include "Runtime/Launch/Resources/Version.h"
#include "ScopedTransaction.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

using namespace CesiumAsync;
using namespace CesiumIonClient;
using namespace CesiumUtility;

/*static*/ TSharedPtr<SelectCesiumIonToken>
    SelectCesiumIonToken::_pExistingPanel{};

/*static*/ SharedFuture<std::optional<Token>>
SelectCesiumIonToken::SelectNewToken(UCesiumIonServer* pServer) {
  if (SelectCesiumIonToken::_pExistingPanel.IsValid()) {
    SelectCesiumIonToken::_pExistingPanel->BringToFront();
  } else {
    TSharedRef<SelectCesiumIonToken> Panel =
        SNew(SelectCesiumIonToken).Server(pServer);
    SelectCesiumIonToken::_pExistingPanel = Panel;

    Panel->_promise = getAsyncSystem().createPromise<std::optional<Token>>();
    Panel->_future = Panel->_promise->getFuture().share();

    Panel->GetOnWindowClosedEvent().AddLambda(
        [Panel](const TSharedRef<SWindow>& pWindow) {
          if (Panel->_promise) {
            // Promise is still outstanding, so resolve it now (no token was
            // selected).
            Panel->_promise->resolve(std::nullopt);
          }
          SelectCesiumIonToken::_pExistingPanel.Reset();
        });
    FSlateApplication::Get().AddWindow(Panel);
  }

  return *SelectCesiumIonToken::_pExistingPanel->_future;
}

Future<std::optional<Token>>
SelectCesiumIonToken::SelectTokenIfNecessary(UCesiumIonServer* pServer) {
  return FCesiumEditorModule::serverManager()
      .GetSession(pServer)
      ->getProjectDefaultTokenDetails()
      .thenInMainThread([pServer](const Token& token) {
        if (token.token.empty()) {
          return SelectCesiumIonToken::SelectNewToken(pServer).thenImmediately(
              [](const std::optional<Token>& maybeToken) {
                return maybeToken;
              });
        } else {
          return getAsyncSystem().createResolvedFuture(
              std::make_optional(token));
        }
      });
}

namespace {

std::vector<int64_t> findUnauthorizedAssets(
    const std::vector<int64_t>& authorizedAssets,
    const std::vector<int64_t>& requiredAssets) {
  std::vector<int64_t> missingAssets;

  for (int64_t assetID : requiredAssets) {
    auto it =
        std::find(authorizedAssets.begin(), authorizedAssets.end(), assetID);
    if (it == authorizedAssets.end()) {
      missingAssets.emplace_back(assetID);
    }
  }

  return missingAssets;
}

} // namespace

Future<std::optional<Token>> SelectCesiumIonToken::SelectAndAuthorizeToken(
    UCesiumIonServer* pServer,
    const std::vector<int64_t>& assetIDs) {
  std::shared_ptr<CesiumIonSession> pSession =
      FCesiumEditorModule::serverManager().GetSession(pServer);
  return SelectTokenIfNecessary(pServer).thenInMainThread([pSession, assetIDs](
                                                              const std::optional<
                                                                  Token>&
                                                                  maybeToken) {
    const std::optional<Connection>& maybeConnection =
        pSession->getConnection();
    if (maybeConnection && maybeToken && !maybeToken->id.empty() &&
        maybeToken->assetIds) {
      std::vector<int64_t> missingAssets =
          findUnauthorizedAssets(*maybeToken->assetIds, assetIDs);
      if (!missingAssets.empty()) {
        // Refresh the token details. We don't want to update the token based
        // on stale information.
        return maybeConnection->token(maybeToken->id)
            .thenInMainThread([pSession, maybeToken, assetIDs](
                                  Response<Token>&& response) {
              if (response.value) {
                std::vector<int64_t> missingAssets =
                    findUnauthorizedAssets(*maybeToken->assetIds, assetIDs);
                if (!missingAssets.empty()) {
                  std::vector<std::string> idStrings(missingAssets.size());
                  std::transform(
                      missingAssets.begin(),
                      missingAssets.end(),
                      idStrings.begin(),
                      [](int64_t id) { return std::to_string(id); });
                  UE_LOG(
                      LogCesiumEditor,
                      Warning,
                      TEXT(
                          "Authorizing the project's default Cesium ion token to access the following asset IDs: %s"),
                      UTF8_TO_TCHAR(joinToString(idStrings, ", ").c_str()));

                  Token newToken = *maybeToken;
                  size_t destinationIndex = newToken.assetIds->size();
                  newToken.assetIds->resize(
                      newToken.assetIds->size() + missingAssets.size());
                  std::copy(
                      missingAssets.begin(),
                      missingAssets.end(),
                      newToken.assetIds->begin() + destinationIndex);

                  return pSession->getConnection()
                      ->modifyToken(
                          newToken.id,
                          newToken.name,
                          newToken.assetIds,
                          newToken.scopes,
                          newToken.allowedUrls)
                      .thenImmediately([maybeToken](Response<NoValue>&&) {
                        return maybeToken;
                      });
                }
              }

              return getAsyncSystem().createResolvedFuture(
                  std::optional<Token>(maybeToken));
            });
      }
    }

    return getAsyncSystem().createResolvedFuture(
        std::optional<Token>(maybeToken));
  });
}

void SelectCesiumIonToken::Construct(const FArguments& InArgs) {
  UCesiumIonServer* pServer = InArgs._Server;

  if (this->_pServer.IsValid() &&
      this->_tokensUpdatedDelegateHandle.IsValid()) {
    FCesiumEditorModule::serverManager()
        .GetSession(this->_pServer.Get())
        ->TokensUpdated.Remove(this->_tokensUpdatedDelegateHandle);
  }

  this->_pServer = pServer;
  std::shared_ptr<CesiumIonSession> pSession =
      FCesiumEditorModule::serverManager().GetSession(this->_pServer.Get());

  this->_tokensUpdatedDelegateHandle = pSession->TokensUpdated.AddRaw(
      this,
      &SelectCesiumIonToken::RefreshTokens);

  TSharedRef<SVerticalBox> pLoaderOrContent = SNew(SVerticalBox);

  pLoaderOrContent->AddSlot().AutoHeight()
      [SNew(STextBlock)
           .AutoWrapText(true)
           .Text(FText::FromString(TEXT(
               "Cesium for Unreal embeds a Cesium ion token in your project in order to allow it to access the assets you add to your levels. Select the Cesium ion token to use.")))];

  pLoaderOrContent->AddSlot().AutoHeight().Padding(
      5.0f)[SNew(CesiumIonServerDisplay).Server(pServer)];

  pLoaderOrContent->AddSlot()
      .Padding(0.0f, 10.0f, 0.0, 10.0f)
      .AutoHeight()
          [SNew(STextBlock)
               .Visibility_Lambda([pSession]() {
                 return pSession->isConnected() ? EVisibility::Collapsed
                                                : EVisibility::Visible;
               })
               .AutoWrapText(true)
               .Text(FText::FromString(TEXT(
                   "Please connect to Cesium ion to select a token from your account or to create a new token.")))];

  pLoaderOrContent->AddSlot()
      .AutoHeight()[SNew(SThrobber).Visibility_Lambda([pSession]() {
        return pSession->isLoadingTokenList() ? EVisibility::Visible
                                              : EVisibility::Collapsed;
      })];

  TSharedRef<SVerticalBox> pMainVerticalBox =
      SNew(SVerticalBox).Visibility_Lambda([pSession]() {
        return pSession->isLoadingTokenList() ? EVisibility::Collapsed
                                              : EVisibility::Visible;
      });
  pLoaderOrContent->AddSlot().AutoHeight()[pMainVerticalBox];

  this->_createNewToken.name =
      FString(FApp::GetProjectName()) + TEXT(" (Created by Cesium for Unreal)");
  this->_useExistingToken.token.id =
      TCHAR_TO_UTF8(*pServer->DefaultIonAccessTokenId);
  this->_useExistingToken.token.token =
      TCHAR_TO_UTF8(*pServer->DefaultIonAccessToken);
  this->_specifyToken.token = pServer->DefaultIonAccessToken;
  this->_tokenSource =
      pServer->DefaultIonAccessToken.IsEmpty() && pSession->isConnected()
          ? TokenSource::Create
          : TokenSource::Specify;

  this->createRadioButton(
      pSession,
      pMainVerticalBox,
      this->_tokenSource,
      TokenSource::Create,
      TEXT("Create a new token"),
      true,
      SNew(SHorizontalBox) +
          SHorizontalBox::Slot()
              .VAlign(EVerticalAlignment::VAlign_Center)
              .AutoWidth()
              .Padding(5.0f)[SNew(STextBlock)
                                 .Text(FText::FromString(TEXT("Name:")))] +
          SHorizontalBox::Slot()
              .VAlign(EVerticalAlignment::VAlign_Center)
              .AutoWidth()
              .MaxWidth(500.0f)
              .Padding(
                  5.0f)[SNew(SEditableTextBox)
                            .Text(this, &SelectCesiumIonToken::GetNewTokenName)
                            .MinDesiredWidth(200.0f)
                            .OnTextChanged(
                                this,
                                &SelectCesiumIonToken::SetNewTokenName)]);

  SAssignNew(this->_pTokensCombo, SComboBox<TSharedPtr<Token>>)
      .OptionsSource(&this->_tokens)
      .OnGenerateWidget(
          this,
          &SelectCesiumIonToken::OnGenerateTokenComboBoxEntry)
      .OnSelectionChanged(this, &SelectCesiumIonToken::OnSelectExistingToken)
      .Content()[SNew(STextBlock).MinDesiredWidth(200.0f).Text_Lambda([this]() {
        return this->_pTokensCombo.IsValid() &&
                       this->_pTokensCombo->GetSelectedItem().IsValid()
                   ? FText::FromString(UTF8_TO_TCHAR(
                         this->_pTokensCombo->GetSelectedItem()->name.c_str()))
                   : FText::FromString(TEXT(""));
      })];

  this->createRadioButton(
      pSession,
      pMainVerticalBox,
      this->_tokenSource,
      TokenSource::UseExisting,
      TEXT("Use an existing token"),
      true,
      SNew(SHorizontalBox) +
          SHorizontalBox::Slot()
              .VAlign(EVerticalAlignment::VAlign_Center)
              .AutoWidth()
              .MaxWidth(500.0f)
              .Padding(5.0f)[SNew(STextBlock)
                                 .Text(FText::FromString(TEXT("Token:")))] +
          SHorizontalBox::Slot()
              .VAlign(EVerticalAlignment::VAlign_Center)
              .Padding(5.0f)
              .AutoWidth()[this->_pTokensCombo.ToSharedRef()]);

  this->createRadioButton(
      pSession,
      pMainVerticalBox,
      this->_tokenSource,
      TokenSource::Specify,
      TEXT("Specify a token"),
      false,
      SNew(SHorizontalBox) +
          SHorizontalBox::Slot()
              .VAlign(EVerticalAlignment::VAlign_Center)
              .AutoWidth()
              .Padding(5.0f)[SNew(STextBlock)
                                 .Text(FText::FromString(TEXT("Token:")))] +
          SHorizontalBox::Slot()
              .VAlign(EVerticalAlignment::VAlign_Center)
              .Padding(5.0f)
              .AutoWidth()
              .MaxWidth(500.0f)
                  [SNew(SEditableTextBox)
                       .Text(this, &SelectCesiumIonToken::GetSpecifiedToken)
                       .OnTextChanged(
                           this,
                           &SelectCesiumIonToken::SetSpecifiedToken)
                       .MinDesiredWidth(500.0f)]);

  pMainVerticalBox->AddSlot().AutoHeight().Padding(
      5.0f,
      20.0f,
      5.0f,
      5.0f)[SNew(SButton)
                .ButtonStyle(FCesiumEditorModule::GetStyle(), "CesiumButton")
                .TextStyle(FCesiumEditorModule::GetStyle(), "CesiumButtonText")
                .Visibility_Lambda([this]() {
                  return this->_tokenSource == TokenSource ::Create
                             ? EVisibility::Collapsed
                             : EVisibility::Visible;
                })
                .OnClicked(this, &SelectCesiumIonToken::UseOrCreate, pSession)
                .Text(FText::FromString(TEXT("Use as Project Default Token")))];

  pMainVerticalBox->AddSlot().AutoHeight().Padding(
      5.0f,
      20.0f,
      5.0f,
      5.0f)[SNew(SButton)
                .ButtonStyle(FCesiumEditorModule::GetStyle(), "CesiumButton")
                .TextStyle(FCesiumEditorModule::GetStyle(), "CesiumButtonText")
                .Visibility_Lambda([this]() {
                  return this->_tokenSource == TokenSource ::Create
                             ? EVisibility::Visible
                             : EVisibility::Collapsed;
                })
                .OnClicked(this, &SelectCesiumIonToken::UseOrCreate, pSession)
                .Text(FText::FromString(
                    TEXT("Create New Project Default Token")))];

  SWindow::Construct(
      SWindow::FArguments()
          .Title(FText::FromString(TEXT("Select a Cesium ion Token")))
          .AutoCenter(EAutoCenter::PreferredWorkArea)
          .SizingRule(ESizingRule::UserSized)
          .ClientSize(FVector2D(635, 500))
              [SNew(SBorder)
                   .Visibility(EVisibility::Visible)
                   .Padding(
                       FMargin(10.0f, 10.0f, 10.0f, 10.0f))[pLoaderOrContent]]);

  pSession->refreshTokens();
}

void SelectCesiumIonToken::createRadioButton(
    const std::shared_ptr<CesiumIonSession>& pSession,
    const TSharedRef<SVerticalBox>& pVertical,
    TokenSource& tokenSource,
    TokenSource thisValue,
    const FString& label,
    bool requiresIonConnection,
    const TSharedRef<SWidget>& pWidget) {
  auto visibility = [pSession, requiresIonConnection]() {
    if (!requiresIonConnection) {
      return EVisibility::Visible;
    } else if (pSession->isConnected()) {
      return EVisibility::Visible;
    } else {
      return EVisibility::Collapsed;
    }
  };

  pVertical->AddSlot().AutoHeight().Padding(5.0f, 10.0f, 5.0f, 10.0f)
      [SNew(SCheckBox)
           .Visibility_Lambda(visibility)
           .Padding(5.0f)
           .Style(FAppStyle::Get(), "RadioButton")
           .IsChecked_Lambda([&tokenSource, thisValue]() {
             return tokenSource == thisValue ? ECheckBoxState::Checked
                                             : ECheckBoxState::Unchecked;
           })
           .OnCheckStateChanged_Lambda([&tokenSource,
                                        thisValue](ECheckBoxState newState) {
             if (newState == ECheckBoxState::Checked) {
               tokenSource = thisValue;
             }
           })[SNew(SBorder)
                  [SNew(SVerticalBox) +
                   SVerticalBox::Slot().Padding(5.0f).AutoHeight()
                       [SNew(STextBlock)
                            .TextStyle(
                                FCesiumEditorModule::GetStyle(),
                                "BodyBold")
                            .Text(FText::FromString(label))] +
                   SVerticalBox::Slot().Padding(5.0f).AutoHeight()[pWidget]]]];
}

FReply
SelectCesiumIonToken::UseOrCreate(std::shared_ptr<CesiumIonSession> pSession) {
  if (!this->_promise || !this->_future) {
    return FReply::Handled();
  }

  Promise<std::optional<Token>> promise = std::move(*this->_promise);
  this->_promise.reset();

  TSharedRef<SelectCesiumIonToken> pPanel =
      StaticCastSharedRef<SelectCesiumIonToken>(this->AsShared());

  auto getToken = [pPanel, pSession]() {
    const AsyncSystem& asyncSystem = getAsyncSystem();

    if (pPanel->_tokenSource == TokenSource::Create) {
      if (pPanel->_createNewToken.name.IsEmpty()) {
        return asyncSystem.createResolvedFuture(Response<Token>());
      }

      // Create a new token, initially only with access to asset ID 1 (Cesium
      // World Terrain).
      return pSession->getConnection()->createToken(
          TCHAR_TO_UTF8(*pPanel->_createNewToken.name),
          {"assets:read"},
          std::vector<int64_t>{1},
          std::nullopt);
    } else if (pPanel->_tokenSource == TokenSource::UseExisting) {
      return asyncSystem.createResolvedFuture(
          Response<Token>(Token(pPanel->_useExistingToken.token), 200, "", ""));
    } else if (pPanel->_tokenSource == TokenSource::Specify) {
      // Check if this is a known token, and use it if so.
      return pSession->findToken(pPanel->_specifyToken.token)
          .thenInMainThread([pPanel](Response<Token>&& response) {
            if (response.value) {
              return std::move(response);
            } else {
              Token t;
              t.token = TCHAR_TO_UTF8(*pPanel->_specifyToken.token);
              return Response(std::move(t), 200, "", "");
            }
          });
    } else {
      return asyncSystem.createResolvedFuture(
          Response<Token>(0, "UNKNOWNSOURCE", "The token source is unknown."));
    }
  };

  getToken().thenInMainThread([pPanel, pSession, promise = std::move(promise)](
                                  Response<Token>&& response) {
    if (response.value) {
      pSession->invalidateProjectDefaultTokenDetails();

      UCesiumIonServer* pServer = pPanel->_pServer.Get();

      FScopedTransaction transaction(
          FText::FromString("Set Project Default Token"));
      pServer->DefaultIonAccessTokenId =
          UTF8_TO_TCHAR(response.value->id.c_str());
      pServer->DefaultIonAccessToken =
          UTF8_TO_TCHAR(response.value->token.c_str());
      pServer->Modify();

      // Refresh all tilesets and overlays that are using the project
      // default token.
      UWorld* pWorld = GEditor->GetEditorWorldContext().World();
      for (auto it = TActorIterator<ACesium3DTileset>(pWorld); it; ++it) {
        if (it->GetTilesetSource() == ETilesetSource::FromCesiumIon &&
            it->GetIonAccessToken().IsEmpty() &&
            it->GetCesiumIonServer() == pServer) {
          it->RefreshTileset();
        } else {
          // Tileset itself does not need to be refreshed, but maybe some
          // overlays do.
          TArray<UCesiumIonRasterOverlay*> rasterOverlays;
          it->GetComponents<UCesiumIonRasterOverlay>(rasterOverlays);

          for (UCesiumIonRasterOverlay* pOverlay : rasterOverlays) {
            if (pOverlay->IonAccessToken.IsEmpty() &&
                pOverlay->CesiumIonServer == pServer) {
              pOverlay->Refresh();
            }
          }
        }
      }
    } else {
      UE_LOG(
          LogCesiumEditor,
          Error,
          TEXT("An error occurred while selecting a token: %s"),
          UTF8_TO_TCHAR(response.errorMessage.c_str()));
    }

    promise.resolve(std::move(response.value));

    pPanel->RequestDestroyWindow();
  });

  while (!this->_future->isReady()) {
    getAssetAccessor()->tick();
    getAsyncSystem().dispatchMainThreadTasks();
  }

  return FReply::Handled();
}

void SelectCesiumIonToken::RefreshTokens() {
  const std::vector<Token>& tokens = FCesiumEditorModule::serverManager()
                                         .GetSession(this->_pServer.Get())
                                         ->getTokens();
  this->_tokens.SetNum(tokens.size());

  std::string createName = TCHAR_TO_UTF8(*this->_createNewToken.name);
  std::string specifiedToken = TCHAR_TO_UTF8(*this->_specifyToken.token);

  for (size_t i = 0; i < tokens.size(); ++i) {
    if (this->_tokens[i]) {
      *this->_tokens[i] = std::move(tokens[i]);
    } else {
      this->_tokens[i] = MakeShared<Token>(std::move(tokens[i]));
    }

    if (this->_tokens[i]->id == this->_useExistingToken.token.id) {
      this->_pTokensCombo->SetSelectedItem(this->_tokens[i]);
      this->_tokenSource = TokenSource::UseExisting;
    }

    // If there's already a token with the default name we would use to create a
    // new one, default to selecting that rather than creating a new one.
    if (this->_tokenSource == TokenSource::Create &&
        this->_tokens[i]->name == createName) {
      this->_pTokensCombo->SetSelectedItem(this->_tokens[i]);
      this->_tokenSource = TokenSource::UseExisting;
    }

    // If this happens to be the specified token, select it.
    if (this->_tokenSource == TokenSource::Specify &&
        this->_tokens[i]->token == specifiedToken) {
      this->_pTokensCombo->SetSelectedItem(this->_tokens[i]);
      this->_tokenSource = TokenSource::UseExisting;
    }
  }

  this->_pTokensCombo->RefreshOptions();
}

TSharedRef<SWidget> SelectCesiumIonToken::OnGenerateTokenComboBoxEntry(
    TSharedPtr<CesiumIonClient::Token> pToken) {
  return SNew(STextBlock)
      .Text(FText::FromString(UTF8_TO_TCHAR(pToken->name.c_str())));
}

FText SelectCesiumIonToken::GetNewTokenName() const {
  return FText::FromString(this->_createNewToken.name);
}

void SelectCesiumIonToken::SetNewTokenName(const FText& text) {
  this->_createNewToken.name = text.ToString();
}

void SelectCesiumIonToken::OnSelectExistingToken(
    TSharedPtr<CesiumIonClient::Token> pToken,
    ESelectInfo::Type type) {
  if (pToken) {
    this->_useExistingToken.token = *pToken;
  }
}

FText SelectCesiumIonToken::GetSpecifiedToken() const {
  return FText::FromString(this->_specifyToken.token);
}

void SelectCesiumIonToken::SetSpecifiedToken(const FText& text) {
  this->_specifyToken.token = text.ToString();
}

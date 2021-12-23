// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "SelectCesiumIonToken.h"
#include "CesiumEditor.h"
#include "CesiumRuntimeSettings.h"
#include "EditorStyleSet.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "ScopedTransaction.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

using namespace CesiumAsync;
using namespace CesiumIonClient;

/*static*/ TSharedPtr<SelectCesiumIonToken>
    SelectCesiumIonToken::_pExistingPanel{};

/*static*/ SharedFuture<std::optional<Token>>
SelectCesiumIonToken::SelectNewToken() {
  if (SelectCesiumIonToken::_pExistingPanel.IsValid()) {
    SelectCesiumIonToken::_pExistingPanel->BringToFront();
  } else {
    TSharedRef<SelectCesiumIonToken> Panel = SNew(SelectCesiumIonToken);
    SelectCesiumIonToken::_pExistingPanel = Panel;

    Panel->_promise = FCesiumEditorModule::ion()
                          .getAsyncSystem()
                          .createPromise<std::optional<Token>>();
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

CesiumAsync::SharedFuture<std::optional<Token>>
SelectCesiumIonToken::SelectTokenIfNecessary() {
  return FCesiumEditorModule::ion()
      .getProjectDefaultTokenDetails()
      .thenInMainThread([](const Token& token) {
        if (token.token.empty()) {
          return SelectCesiumIonToken::SelectNewToken().thenImmediately(
              [](const std::optional<Token>& maybeToken) {
                return maybeToken;
              });
        } else {
          return FCesiumEditorModule::ion()
              .getAsyncSystem()
              .createResolvedFuture(std::make_optional(token));
        }
      })
      .share();
}

SelectCesiumIonToken::SelectCesiumIonToken() {
  this->_tokensUpdatedDelegateHandle =
      FCesiumEditorModule::ion().TokensUpdated.AddRaw(
          this,
          &SelectCesiumIonToken::RefreshTokens);
}

SelectCesiumIonToken::~SelectCesiumIonToken() {
  FCesiumEditorModule::ion().TokensUpdated.Remove(
      this->_tokensUpdatedDelegateHandle);
}

void SelectCesiumIonToken::Construct(const FArguments& InArgs) {
  TSharedRef<SVerticalBox> pLoaderOrContent = SNew(SVerticalBox);

  pLoaderOrContent->AddSlot()
      .AutoHeight()[SNew(SThrobber).Visibility_Lambda([]() {
        return FCesiumEditorModule::ion().isLoadingTokenList()
                   ? EVisibility::Visible
                   : EVisibility::Collapsed;
      })];

  TSharedRef<SVerticalBox> pMainVerticalBox =
      SNew(SVerticalBox).Visibility_Lambda([]() {
        return FCesiumEditorModule::ion().isLoadingTokenList()
                   ? EVisibility::Collapsed
                   : EVisibility::Visible;
      });
  pLoaderOrContent->AddSlot().AutoHeight()[pMainVerticalBox];

  this->_createNewToken.name =
      FString(FApp::GetProjectName()) + TEXT(" (Created by Cesium for Unreal)");
  this->_useExistingToken.token.id = TCHAR_TO_UTF8(
      *GetDefault<UCesiumRuntimeSettings>()->DefaultIonAccessTokenId);
  this->_useExistingToken.token.token = TCHAR_TO_UTF8(
      *GetDefault<UCesiumRuntimeSettings>()->DefaultIonAccessToken);
  this->_specifyToken.token =
      GetDefault<UCesiumRuntimeSettings>()->DefaultIonAccessToken;
  this->_tokenSource =
      GetDefault<UCesiumRuntimeSettings>()->DefaultIonAccessToken.IsEmpty()
          ? TokenSource::Create
          : TokenSource::Specify;

  this->createRadioButton(
      pMainVerticalBox,
      this->_tokenSource,
      TokenSource::Create,
      TEXT("Create a new token"),
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
      pMainVerticalBox,
      this->_tokenSource,
      TokenSource::UseExisting,
      TEXT("Use an existing token"),
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
      pMainVerticalBox,
      this->_tokenSource,
      TokenSource::Specify,
      TEXT("Specify a token"),
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
                .OnClicked(this, &SelectCesiumIonToken::UseOrCreate)
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
                .OnClicked(this, &SelectCesiumIonToken::UseOrCreate)
                .Text(FText::FromString(
                    TEXT("Create New Project Default Token")))];

  SWindow::Construct(
      SWindow::FArguments()
          .Title(FText::FromString(TEXT("Select a Cesium ion Token")))
          .AutoCenter(EAutoCenter::PreferredWorkArea)
          .SizingRule(ESizingRule::UserSized)
          .ClientSize(FVector2D(635, 400))
              [SNew(SBorder)
                   .Visibility(EVisibility::Visible)
                   .BorderImage(FEditorStyle::GetBrush("Menu.Background"))
                   .Padding(
                       FMargin(10.0f, 10.0f, 10.0f, 10.0f))[pLoaderOrContent]]);

  FCesiumEditorModule::ion().refreshTokens();
}

void SelectCesiumIonToken::createRadioButton(
    const TSharedRef<SVerticalBox>& pVertical,
    TokenSource& tokenSource,
    TokenSource thisValue,
    const FString& label,
    const TSharedRef<SWidget>& pWidget) {
  pVertical->AddSlot().AutoHeight().Padding(5.0f, 10.0f, 5.0f, 10.0f)
      [SNew(SCheckBox)
           .Padding(5.0f)
           .Style(FCoreStyle::Get(), "RadioButton")
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

FReply SelectCesiumIonToken::UseOrCreate() {
  if (!this->_promise) {
    return FReply::Handled();
  }

  Promise<std::optional<Token>> promise = std::move(*this->_promise);
  this->_promise.reset();

  TSharedRef<SelectCesiumIonToken> pPanel =
      StaticCastSharedRef<SelectCesiumIonToken>(this->AsShared());

  auto getToken = [this]() {
    const AsyncSystem& asyncSystem =
        FCesiumEditorModule::ion().getAsyncSystem();

    if (this->_tokenSource == TokenSource::Create) {
      if (this->_createNewToken.name.IsEmpty()) {
        return asyncSystem.createResolvedFuture(Response<Token>());
      }

      return FCesiumEditorModule::ion().getConnection()->createToken(
          TCHAR_TO_UTF8(*this->_createNewToken.name),
          {"assets:read"},
          std::nullopt,
          std::nullopt);
    } else if (this->_tokenSource == TokenSource::UseExisting) {
      return asyncSystem.createResolvedFuture(
          Response<Token>(Token(this->_useExistingToken.token), 200, "", ""));
    } else if (this->_tokenSource == TokenSource::Specify) {
      // Check if this is a known token, and use it if so.
      std::string token = TCHAR_TO_UTF8(*this->_specifyToken.token);
      const std::vector<Token>& tokens = FCesiumEditorModule::ion().getTokens();
      auto it = std::find_if(
          tokens.begin(),
          tokens.end(),
          [&token](const Token& candidate) {
            return candidate.token == token;
          });

      Token t{};
      if (it == tokens.end()) {
        t.token = std::move(token);
      } else {
        t = *it;
      }

      return asyncSystem.createResolvedFuture(
          Response<Token>(std::move(t), 200, "", ""));
    } else {
      return asyncSystem.createResolvedFuture(
          Response<Token>(0, "UNKNOWNSOURCE", "The token source is unknown."));
    }
  };

  getToken().thenInMainThread(
      [pPanel, promise = std::move(promise)](Response<Token>&& response) {
        if (response.value) {
          FCesiumEditorModule::ion().invalidateProjectDefaultTokenDetails();

          UCesiumRuntimeSettings* pSettings =
              GetMutableDefault<UCesiumRuntimeSettings>();

          FScopedTransaction transaction(
              FText::FromString("Set Project Default Token"));
          pSettings->DefaultIonAccessTokenId =
              UTF8_TO_TCHAR(response.value->id.c_str());
          pSettings->DefaultIonAccessToken =
              UTF8_TO_TCHAR(response.value->token.c_str());
          pSettings->Modify();
        } else {
          UE_LOG(
              LogCesiumEditor,
              Error,
              TEXT("An error occurred while selecting a token: %s"),
              response.errorMessage.c_str());
        }

        promise.resolve(std::move(response.value));

        pPanel->RequestDestroyWindow();
      });

  return FReply::Handled();
}

void SelectCesiumIonToken::RefreshTokens() {
  const std::vector<Token>& tokens = FCesiumEditorModule::ion().getTokens();
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

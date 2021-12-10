// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonTokenTroubleshooting.h"
#include "Cesium3DTileset.h"
#include "CesiumEditor.h"
#include "CesiumIonClient/Connection.h"
#include "CesiumRuntimeSettings.h"
#include "EditorStyleSet.h"
#include "ScopedTransaction.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Text/STextBlock.h"

using namespace CesiumIonClient;

/*static*/ TMap<ACesium3DTileset*, TSharedRef<CesiumIonTokenTroubleshooting>>
    CesiumIonTokenTroubleshooting::_existingPanels{};

/*static*/ void CesiumIonTokenTroubleshooting::Open(
    ACesium3DTileset* pTileset,
    bool triggeredByError) {
  const TSharedRef<CesiumIonTokenTroubleshooting>* ppExisting =
      CesiumIonTokenTroubleshooting::_existingPanels.Find(pTileset);
  if (ppExisting) {
    FSlateApplication::Get().RequestDestroyWindow(*ppExisting);
    CesiumIonTokenTroubleshooting::_existingPanels.Remove(pTileset);
  }

  TSharedRef<CesiumIonTokenTroubleshooting> Troubleshooting =
      SNew(CesiumIonTokenTroubleshooting)
          .Tileset(pTileset)
          .TriggeredByError(triggeredByError);

  Troubleshooting->GetOnWindowClosedEvent().AddLambda(
      [pTileset](const TSharedRef<SWindow>& pWindow) {
        CesiumIonTokenTroubleshooting::_existingPanels.Remove(pTileset);
      });
  FSlateApplication::Get().AddWindow(Troubleshooting);

  CesiumIonTokenTroubleshooting::_existingPanels.Add(pTileset, Troubleshooting);
}

void CesiumIonTokenTroubleshooting::Construct(const FArguments& InArgs) {
  TSharedRef<SVerticalBox> pMainVerticalBox = SNew(SVerticalBox);

  ACesium3DTileset* pTileset = InArgs._Tileset;
  if (!pTileset) {
    return;
  }

  this->_pTileset = pTileset;

  if (InArgs._TriggeredByError) {
    FString preamble = FString::Format(
        TEXT(
            "Actor \"{0}\" ({1}) tried to access Cesium ion for asset ID {2}, but it didn't work, probably due to a problem with the access token. This panel will help you fix it!"),
        {*pTileset->GetActorLabel(),
         *pTileset->GetName(),
         pTileset->GetIonAssetID()});

    pMainVerticalBox->AddSlot().AutoHeight()
        [SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(preamble))];
  }

  TSharedRef<SHorizontalBox> pDiagnosticColumns = SNew(SHorizontalBox);

  if (!pTileset->GetIonAccessToken().IsEmpty()) {
    this->_assetTokenState.name = TEXT("This Tileset's Access Token");
    this->_assetTokenState.token = pTileset->GetIonAccessToken();
    pDiagnosticColumns->AddSlot()
        .Padding(5.0f, 20.0f, 5.0f, 5.0f)
        .AutoWidth()
        .FillWidth(0.5f)[createTokenPanel(pTileset, this->_assetTokenState)];
  }

  this->_projectDefaultTokenState.name = TEXT("Project Default Access Token");
  this->_projectDefaultTokenState.token =
      GetDefault<UCesiumRuntimeSettings>()->DefaultIonAccessToken;

  pDiagnosticColumns->AddSlot()
      .Padding(5.0f, 20.0f, 5.0f, 5.0f)
      .AutoWidth()
      .FillWidth(0.5f)
          [this->createTokenPanel(pTileset, this->_projectDefaultTokenState)];

  pMainVerticalBox->AddSlot().AutoHeight()[pDiagnosticColumns];

  this->addRemedyButton(
      pMainVerticalBox,
      TEXT("Use the project default token for this Actor"),
      &CesiumIonTokenTroubleshooting::canUseProjectDefaultToken,
      &CesiumIonTokenTroubleshooting::useProjectDefaultToken);

  pMainVerticalBox->AddSlot().AutoHeight().Padding(
      0.0f,
      10.0f,
      0.0f,
      5.0f)[SNew(SButton)
                .ButtonStyle(FCesiumEditorModule::GetStyle(), "CesiumButton")
                .TextStyle(FCesiumEditorModule::GetStyle(), "CesiumButtonText")
                //.OnClicked(this, &IonLoginPanel::SignIn)
                .Text(FText::FromString(
                    TEXT("Authorize the tileset's token to access this asset")))
                .Visibility_Lambda([this]() {
                  return this->_assetTokenState.isValid.value_or(false) &&
                                 !this->_assetTokenState.allowsAccessToAsset
                                      .value_or(true) &&
                                 this->_assetTokenState
                                     .associatedWithUserAccount.value_or(false)
                             ? EVisibility::Visible
                             : EVisibility::Collapsed;
                })];

  pMainVerticalBox->AddSlot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 5.0f)
      [SNew(SButton)
           .ButtonStyle(FCesiumEditorModule::GetStyle(), "CesiumButton")
           .TextStyle(FCesiumEditorModule::GetStyle(), "CesiumButtonText")
           //.OnClicked(this, &IonLoginPanel::SignIn)
           .Text(FText::FromString(TEXT(
               "Authorize the project default token to access this asset")))
           .Visibility_Lambda([this]() {
             return this->_projectDefaultTokenState.isValid.value_or(false) &&
                            !this->_projectDefaultTokenState.allowsAccessToAsset
                                 .value_or(true) &&
                            this->_projectDefaultTokenState
                                .associatedWithUserAccount.value_or(false)
                        ? EVisibility::Visible
                        : EVisibility::Collapsed;
           })];

  pMainVerticalBox->AddSlot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 5.0f)
      [SNew(SButton)
           .ButtonStyle(FCesiumEditorModule::GetStyle(), "CesiumButton")
           .TextStyle(FCesiumEditorModule::GetStyle(), "CesiumButtonText")
           //.OnClicked(this, &IonLoginPanel::SignIn)
           .Text(FText::FromString(TEXT("Create a new project default token")))
           .Visibility_Lambda([this]() {
             return !this->_projectDefaultTokenState.isValid.value_or(true) ||
                            (!this->_projectDefaultTokenState
                                  .allowsAccessToAsset.value_or(true) &&
                             !this->_projectDefaultTokenState
                                  .associatedWithUserAccount.value_or(true))
                        ? EVisibility::Visible
                        : EVisibility::Collapsed;
           })];

  SWindow::Construct(
      SWindow::FArguments()
          .Title(FText::FromString(FString::Format(
              TEXT("{0}: Cesium ion Token Troubleshooting"),
              {*pTileset->GetActorLabel()})))
          .AutoCenter(EAutoCenter::PreferredWorkArea)
          .SizingRule(ESizingRule::UserSized)
          .ClientSize(FVector2D(800, 600))
              [SNew(SBorder)
                   .Visibility(EVisibility::Visible)
                   .BorderImage(FEditorStyle::GetBrush("Menu.Background"))
                   .Padding(
                       FMargin(10.0f, 10.0f, 10.0f, 10.0f))[pMainVerticalBox]]);
}

namespace {

TSharedRef<SWidget>
addTokenCheck(const FString& label, std::optional<bool>& state) {
  return SNew(SHorizontalBox) +
         SHorizontalBox::Slot().AutoWidth().Padding(
             3.0f,
             0.0f,
             3.0f,
             0.0f)[SNew(SThrobber)
                       .Visibility_Lambda([&state]() {
                         CesiumIonSession& ion = FCesiumEditorModule::ion();
                         ion.getAssetAccessor()->tick();
                         ion.getAsyncSystem().dispatchMainThreadTasks();
                         return state.has_value() ? EVisibility::Collapsed
                                                  : EVisibility::Visible;
                       })
                       .NumPieces(1)
                       .Animate(SThrobber::All)] +
         SHorizontalBox::Slot().AutoWidth().Padding(
             5.0f,
             0.0f,
             5.0f,
             0.0f)[SNew(SImage)
                       .Visibility_Lambda([&state]() {
                         return state.has_value() ? EVisibility::Visible
                                                  : EVisibility::Collapsed;
                       })
                       .Image_Lambda([&state]() {
                         return state.has_value() && *state
                                    ? FCesiumEditorModule::GetStyle()->GetBrush(
                                          TEXT("Cesium.Common.GreenTick"))
                                    : FCesiumEditorModule::GetStyle()->GetBrush(
                                          TEXT("Cesium.Common.RedX"));
                       })] +
         SHorizontalBox::Slot()
             .AutoWidth()[SNew(STextBlock).Text(FText::FromString(label))];
}

} // namespace

TSharedRef<SWidget> CesiumIonTokenTroubleshooting::createTokenPanel(
    ACesium3DTileset* pTileset,
    TokenState& state) {
  CesiumIonSession& ionSession = FCesiumEditorModule::ion();

  int64 assetID = pTileset->GetIonAssetID();

  auto pConnection = std::make_shared<Connection>(
      ionSession.getAsyncSystem(),
      ionSession.getAssetAccessor(),
      TCHAR_TO_UTF8(*state.token));

  // Don't let this panel be destroyed while the async operations below are in
  // progress.
  TSharedRef<CesiumIonTokenTroubleshooting> pPanel =
      StaticCastSharedRef<CesiumIonTokenTroubleshooting>(this->AsShared());

  pConnection->me()
      .thenInMainThread(
          [pPanel, pConnection, assetID, &state](Response<Profile>&& profile) {
            state.isValid = profile.value.has_value();
            if (pPanel->IsVisible()) {
              return pConnection->asset(assetID);
            } else {
              return pConnection->getAsyncSystem().createResolvedFuture(
                  Response<Asset>{});
            }
          })
      .thenInMainThread([pPanel, pConnection, &state](Response<Asset>&& asset) {
        state.allowsAccessToAsset = asset.value.has_value();

        if (pPanel->IsVisible()) {
          // Query the tokens using the user's connection (_not_ the token
          // connection created above).
          CesiumIonSession& ionSession = FCesiumEditorModule::ion();
          ionSession.resume();

          const std::optional<Connection>& userConnection =
              ionSession.getConnection();
          if (!userConnection) {
            Response<std::vector<Token>> result{};
            return ionSession.getAsyncSystem().createResolvedFuture(
                std::move(result));
          }
          return userConnection->tokens();
        } else {
          return pConnection->getAsyncSystem().createResolvedFuture(
              Response<std::vector<Token>>{});
        }
      })
      .thenInMainThread(
          [pPanel, pConnection, &state](Response<std::vector<Token>>&& tokens) {
            state.associatedWithUserAccount = false;
            if (tokens.value.has_value()) {
              auto it = std::find_if(
                  tokens.value->begin(),
                  tokens.value->end(),
                  [&pConnection](const Token& token) {
                    return token.token == pConnection->getAccessToken();
                  });
              state.associatedWithUserAccount = it != tokens.value->end();
            }
          });

  TSharedRef<SVerticalBox> pRows =
      SNew(SVerticalBox) +
      SVerticalBox::Slot().Padding(
          0.0f,
          5.0f,
          0.0f,
          5.0f)[SNew(SHeader).Content()
                    [SNew(STextBlock)
                         .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                         .Text(FText::FromString(state.name))]];

  pRows->AddSlot().Padding(
      0.0f,
      5.0f,
      0.0f,
      5.0f)[addTokenCheck(TEXT("Is a valid Cesium ion token"), state.isValid)];
  pRows->AddSlot().Padding(0.0f, 5.0f, 0.0f, 5.0f)[addTokenCheck(
      TEXT("Allows access to this asset"),
      state.allowsAccessToAsset)];
  pRows->AddSlot().Padding(0.0f, 5.0f, 0.0f, 5.0f)[addTokenCheck(
      TEXT("Is associated with your user account"),
      state.associatedWithUserAccount)];

  return pRows;
}

void CesiumIonTokenTroubleshooting::addRemedyButton(
    const TSharedRef<SVerticalBox>& pParent,
    const FString& name,
    bool (CesiumIonTokenTroubleshooting::*isAvailableCallback)() const,
    FReply (CesiumIonTokenTroubleshooting::*clickCallback)()) {
  pParent->AddSlot().AutoHeight().Padding(
      0.0f,
      20.0f,
      0.0f,
      5.0f)[SNew(SButton)
                .ButtonStyle(FCesiumEditorModule::GetStyle(), "CesiumButton")
                .TextStyle(FCesiumEditorModule::GetStyle(), "CesiumButtonText")
                .OnClicked(this, clickCallback)
                .Text(FText::FromString(name))
                .Visibility_Lambda([this, isAvailableCallback]() {
                  return std::invoke(isAvailableCallback, *this)
                             ? EVisibility::Visible
                             : EVisibility::Collapsed;
                })];
}

bool CesiumIonTokenTroubleshooting::canUseProjectDefaultToken() const {
  bool isValid = this->_projectDefaultTokenState.isValid.value_or(false);
  bool allowsAccess =
      this->_projectDefaultTokenState.allowsAccessToAsset.value_or(false);
  return isValid && allowsAccess;
}

FReply CesiumIonTokenTroubleshooting::useProjectDefaultToken() {
  this->RequestDestroyWindow();

  if (!this->_pTileset) {
    return FReply::Handled();
  }

  FScopedTransaction transaction(
      FText::FromString("Use Project Default Token"));
  this->_pTileset->Modify();
  this->_pTileset->SetIonAccessToken(TEXT(""));

  return FReply::Handled();
}

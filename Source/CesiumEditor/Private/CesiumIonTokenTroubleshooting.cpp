// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonTokenTroubleshooting.h"
#include "Cesium3DTileset.h"
#include "CesiumEditor.h"
#include "CesiumIonClient/Connection.h"
#include "CesiumRuntimeSettings.h"
#include "EditorStyleSet.h"
#include "LevelEditor.h"
#include "ScopedTransaction.h"
#include "SelectCesiumIonToken.h"
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
             3.0f)[SNew(SImage)
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
        .VAlign(EVerticalAlignment::VAlign_Top)
        .AutoWidth()
        .FillWidth(
            0.5f)[this->createTokenPanel(pTileset, this->_assetTokenState)];
  }

  this->_projectDefaultTokenState.name = TEXT("Project Default Access Token");
  this->_projectDefaultTokenState.token =
      GetDefault<UCesiumRuntimeSettings>()->DefaultIonAccessToken;

  pDiagnosticColumns->AddSlot()
      .Padding(5.0f, 20.0f, 5.0f, 5.0f)
      .VAlign(EVerticalAlignment::VAlign_Top)
      .AutoWidth()
      .FillWidth(0.5f)
          [this->createTokenPanel(pTileset, this->_projectDefaultTokenState)];

  if (FCesiumEditorModule::ion().isConnected()) {
    // Don't let this panel be destroyed while the async operations below are in
    // progress.
    TSharedRef<CesiumIonTokenTroubleshooting> pPanel =
        StaticCastSharedRef<CesiumIonTokenTroubleshooting>(this->AsShared());

    FCesiumEditorModule::ion()
        .getConnection()
        ->asset(pTileset->GetIonAssetID())
        .thenInMainThread([pPanel](Response<Asset>&& asset) {
          pPanel->_assetExistsInUserAccount = asset.value.has_value();
        });

    // Start a new row if we have more than two columns.
    if (pDiagnosticColumns->NumSlots() >= 2) {
      pMainVerticalBox->AddSlot().AutoHeight()[pDiagnosticColumns];
      pDiagnosticColumns = SNew(SHorizontalBox);
    }

    pDiagnosticColumns->AddSlot()
        .Padding(5.0f, 20.0f, 5.0f, 5.0f)
        .VAlign(EVerticalAlignment::VAlign_Top)
        .AutoWidth()
        .FillWidth(0.5f)[this->createDiagnosticPanel(
            TEXT("Asset"),
            {addTokenCheck(
                TEXT("Asset ID exists in your user account"),
                this->_assetExistsInUserAccount)})];
  }

  pMainVerticalBox->AddSlot().AutoHeight()[pDiagnosticColumns];

  this->addRemedyButton(
      pMainVerticalBox,
      TEXT("Connect to Cesium ion"),
      &CesiumIonTokenTroubleshooting::canConnectToCesiumIon,
      &CesiumIonTokenTroubleshooting::connectToCesiumIon);

  this->addRemedyButton(
      pMainVerticalBox,
      TEXT("Use the project default token for this Actor"),
      &CesiumIonTokenTroubleshooting::canUseProjectDefaultToken,
      &CesiumIonTokenTroubleshooting::useProjectDefaultToken);

  this->addRemedyButton(
      pMainVerticalBox,
      TEXT("Authorize the tileset's token to access this asset"),
      &CesiumIonTokenTroubleshooting::canAuthorizeAssetToken,
      &CesiumIonTokenTroubleshooting::authorizeAssetToken);

  this->addRemedyButton(
      pMainVerticalBox,
      TEXT("Authorize the project default token to access this asset"),
      &CesiumIonTokenTroubleshooting::canAuthorizeProjectDefaultToken,
      &CesiumIonTokenTroubleshooting::authorizeProjectDefaultToken);

  this->addRemedyButton(
      pMainVerticalBox,
      TEXT("Select or create a new project default token"),
      &CesiumIonTokenTroubleshooting::canSelectNewProjectDefaultToken,
      &CesiumIonTokenTroubleshooting::selectNewProjectDefaultToken);

  pMainVerticalBox->AddSlot().AutoHeight().Padding(0.0f, 20.0f, 0.0f, 0.0f)
      [SNew(STextBlock)
           .Visibility_Lambda([this]() {
             return (this->_assetTokenState.token.IsEmpty() ||
                     this->_assetTokenState.allowsAccessToAsset == false) &&
                            (this->_projectDefaultTokenState.token.IsEmpty() ||
                             this->_projectDefaultTokenState
                                     .allowsAccessToAsset == false) &&
                            this->_assetExistsInUserAccount == false
                        ? EVisibility::Visible
                        : EVisibility::Collapsed;
           })
           .AutoWrapText(true)
           .Text(FText::FromString(FString::Format(
               TEXT(
                   "No automatic remedies are possible for Asset ID {0}, because:\n"
                   " - The current token does not authorize access to the specified asset ID, and\n"
                   " - The asset ID does not exist in your Cesium ion account.\n"
                   "\n"
                   "Please click the button below to open Cesium ion and check:\n"
                   " - The Tileset's \"Ion Asset ID\" property is correct.\n"
                   " - If the asset is from the \"Asset Depot\", verify that it has been added to \"My Assets\"."),
               {pTileset->GetIonAssetID()})))];

  this->addRemedyButton(
      pMainVerticalBox,
      TEXT("Open Cesium ion on the Web"),
      &CesiumIonTokenTroubleshooting::canOpenCesiumIon,
      &CesiumIonTokenTroubleshooting::openCesiumIon);

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

TSharedRef<SWidget> CesiumIonTokenTroubleshooting::createDiagnosticPanel(
    const FString& name,
    const TArray<TSharedRef<SWidget>>& diagnostics) {
  TSharedRef<SVerticalBox> pRows =
      SNew(SVerticalBox) +
      SVerticalBox::Slot().Padding(
          0.0f,
          5.0f,
          0.0f,
          5.0f)[SNew(SHeader).Content()
                    [SNew(STextBlock)
                         .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                         .Text(FText::FromString(name))]];

  for (const TSharedRef<SWidget>& diagnostic : diagnostics) {
    pRows->AddSlot().Padding(0.0f, 5.0f, 0.0f, 5.0f)[diagnostic];
  }

  return pRows;
}

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
            Response<TokenList> result{};
            return ionSession.getAsyncSystem().createResolvedFuture(
                std::move(result));
          }
          return userConnection->tokens();
        } else {
          return pConnection->getAsyncSystem().createResolvedFuture(
              Response<TokenList>{});
        }
      })
      .thenInMainThread(
          [pPanel, pConnection, &state](Response<TokenList>&& tokens) {
            state.associatedWithUserAccount = false;
            if (tokens.value.has_value()) {
              auto it = std::find_if(
                  tokens.value->items.begin(),
                  tokens.value->items.end(),
                  [&pConnection](const Token& token) {
                    return token.token == pConnection->getAccessToken();
                  });
              state.associatedWithUserAccount = it != tokens.value->items.end();
            }
          });

  return this->createDiagnosticPanel(
      state.name,
      {addTokenCheck(TEXT("Is a valid Cesium ion token"), state.isValid),
       addTokenCheck(
           TEXT("Allows access to this asset"),
           state.allowsAccessToAsset),
       addTokenCheck(
           TEXT("Is associated with your user account"),
           state.associatedWithUserAccount)});
}

void CesiumIonTokenTroubleshooting::addRemedyButton(
    const TSharedRef<SVerticalBox>& pParent,
    const FString& name,
    bool (CesiumIonTokenTroubleshooting::*isAvailableCallback)() const,
    void (CesiumIonTokenTroubleshooting::*clickCallback)()) {
  pParent->AddSlot().AutoHeight().Padding(
      0.0f,
      20.0f,
      0.0f,
      5.0f)[SNew(SButton)
                .ButtonStyle(FCesiumEditorModule::GetStyle(), "CesiumButton")
                .TextStyle(FCesiumEditorModule::GetStyle(), "CesiumButtonText")
                .OnClicked_Lambda([this, clickCallback]() {
                  std::invoke(clickCallback, *this);
                  this->RequestDestroyWindow();
                  return FReply::Handled();
                })
                .Text(FText::FromString(name))
                .Visibility_Lambda([this, isAvailableCallback]() {
                  return std::invoke(isAvailableCallback, *this)
                             ? EVisibility::Visible
                             : EVisibility::Collapsed;
                })];
}

bool CesiumIonTokenTroubleshooting::canConnectToCesiumIon() const {
  return !FCesiumEditorModule::ion().isConnected();
}

void CesiumIonTokenTroubleshooting::connectToCesiumIon() {
  // Pop up the Cesium panel to show the status.
  FLevelEditorModule* pLevelEditorModule =
      FModuleManager::GetModulePtr<FLevelEditorModule>(
          FName(TEXT("LevelEditor")));
  TSharedPtr<FTabManager> pTabManager =
      pLevelEditorModule ? pLevelEditorModule->GetLevelEditorTabManager()
                         : FGlobalTabmanager::Get();
  pTabManager->TryInvokeTab(FTabId(TEXT("Cesium")));

  // Pop up a browser window to sign in to ion.
  FCesiumEditorModule::ion().connect();
}

bool CesiumIonTokenTroubleshooting::canUseProjectDefaultToken() const {
  const TokenState& state = this->_projectDefaultTokenState;
  return this->_pTileset && !this->_pTileset->GetIonAccessToken().IsEmpty() &&
         state.isValid == true && state.allowsAccessToAsset == true;
}

void CesiumIonTokenTroubleshooting::useProjectDefaultToken() {
  if (!this->_pTileset) {
    return;
  }

  if (!this->_pTileset->GetIonAccessToken().IsEmpty()) {
    FScopedTransaction transaction(
        FText::FromString("Use Project Default Token"));
    this->_pTileset->Modify();
    this->_pTileset->SetIonAccessToken(FString());
  } else {
    // Already using project default token, just refresh.
    this->_pTileset->RefreshTileset();
  }
}

bool CesiumIonTokenTroubleshooting::canAuthorizeAssetToken() const {
  const TokenState& state = this->_assetTokenState;
  return this->_assetExistsInUserAccount == true && state.isValid == true &&
         state.allowsAccessToAsset == false &&
         state.associatedWithUserAccount == true;
}

void CesiumIonTokenTroubleshooting::authorizeAssetToken() {
  if (!this->_pTileset) {
    return;
  }
  this->authorizeToken(this->_pTileset->GetIonAccessToken(), false);
}

bool CesiumIonTokenTroubleshooting::canAuthorizeProjectDefaultToken() const {
  const TokenState& state = this->_projectDefaultTokenState;
  return this->_assetExistsInUserAccount == true && state.isValid == true &&
         state.allowsAccessToAsset == false &&
         state.associatedWithUserAccount == true;
}

void CesiumIonTokenTroubleshooting::authorizeProjectDefaultToken() {
  this->authorizeToken(
      GetDefault<UCesiumRuntimeSettings>()->DefaultIonAccessToken,
      true);
}

bool CesiumIonTokenTroubleshooting::canSelectNewProjectDefaultToken() const {
  if (this->_assetExistsInUserAccount == false) {
    return false;
  }

  const TokenState& state = this->_projectDefaultTokenState;
  return FCesiumEditorModule::ion().isConnected() &&
         (state.isValid == false || (state.allowsAccessToAsset == false &&
                                     state.associatedWithUserAccount == false));
}

void CesiumIonTokenTroubleshooting::selectNewProjectDefaultToken() {
  if (!this->_pTileset) {
    return;
  }

  CesiumIonSession& session = FCesiumEditorModule::ion();
  const std::optional<Connection>& maybeConnection = session.getConnection();
  if (!session.isConnected() || !maybeConnection) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "Cannot create a new project default token because you are not signed in to Cesium ion."));
    return;
  }

  // Don't let this panel be destroyed while the async operations below are in
  // progress.
  TSharedRef<CesiumIonTokenTroubleshooting> pPanel =
      StaticCastSharedRef<CesiumIonTokenTroubleshooting>(this->AsShared());

  SelectCesiumIonToken::SelectNewToken().thenInMainThread(
      [pPanel](const std::optional<Token>& newToken) {
        if (!newToken) {
          return;
        }

        GetMutableDefault<UCesiumRuntimeSettings>()->DefaultIonAccessToken =
            UTF8_TO_TCHAR(newToken->token.c_str());

        pPanel->useProjectDefaultToken();
      });
}

bool CesiumIonTokenTroubleshooting::canOpenCesiumIon() const {
  return FCesiumEditorModule::ion().isConnected();
}

void CesiumIonTokenTroubleshooting::openCesiumIon() {
  FPlatformProcess::LaunchURL(
      TEXT("https://cesium.com/ion/tokens"),
      NULL,
      NULL);
}

void CesiumIonTokenTroubleshooting::authorizeToken(
    const FString& token,
    bool removeTilesetToken) {
  if (!this->_pTileset) {
    return;
  }

  CesiumIonSession& session = FCesiumEditorModule::ion();
  const std::optional<Connection>& maybeConnection = session.getConnection();
  if (!session.isConnected() || !maybeConnection) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "Cannot grant a token access to an asset because you are not signed in to Cesium ion."));
    return;
  }

  TWeakObjectPtr<ACesium3DTileset> pTileset = this->_pTileset;

  session.findToken(token).thenInMainThread([pTileset,
                                             removeTilesetToken,
                                             connection = *maybeConnection](
                                                std::optional<Token>&&
                                                    maybeToken) {
    if (!pTileset.IsValid()) {
      // Tileset Actor has been destroyed
      return connection.getAsyncSystem().createResolvedFuture();
    }

    if (!maybeToken) {
      UE_LOG(
          LogCesiumEditor,
          Error,
          TEXT(
              "Cannot grant a token access to an asset because the token was not found in the signed-in Cesium ion account."));
      return connection.getAsyncSystem().createResolvedFuture();
    }

    if (!maybeToken->assetIds) {
      UE_LOG(
          LogCesiumEditor,
          Warning,
          TEXT(
              "Cannot grant a token access to an asset because the token appears to already have access to all assets."));
      return connection.getAsyncSystem().createResolvedFuture();
    }

    auto it = std::find(
        maybeToken->assetIds->begin(),
        maybeToken->assetIds->end(),
        pTileset->GetIonAssetID());
    if (it != maybeToken->assetIds->end()) {
      UE_LOG(
          LogCesiumEditor,
          Warning,
          TEXT(
              "Cannot grant a token access to an asset because the token appears to already have access to the asset."));
      return connection.getAsyncSystem().createResolvedFuture();
    }

    maybeToken->assetIds->emplace_back(pTileset->GetIonAssetID());

    return connection
        .modifyToken(
            maybeToken->id,
            maybeToken->name,
            maybeToken->assetIds,
            maybeToken->scopes,
            maybeToken->allowedUrls)
        .thenInMainThread([pTileset,
                           removeTilesetToken](Response<NoValue>&& result) {
          if (result.value) {
            // Refresh the tileset now that the token is valid (hopefully).
            if (pTileset.IsValid()) {
              if (removeTilesetToken &&
                  !pTileset->GetIonAccessToken().IsEmpty()) {
                pTileset->SetIonAccessToken(FString());
              } else {
                pTileset->RefreshTileset();
              }
            }
          } else {
            UE_LOG(
                LogCesiumEditor,
                Error,
                TEXT(
                    "An error occurred while attempting to modify a token to grant it access to an asset. Please visit https://cesium.com/ion/tokens to modify the token manually."));
          }
        });
  });
}

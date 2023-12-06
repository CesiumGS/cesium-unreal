// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonTokenTroubleshooting.h"
#include "Cesium3DTileset.h"
#include "CesiumEditor.h"
#include "CesiumIonClient/Connection.h"
#include "CesiumIonRasterOverlay.h"
#include "CesiumIonServerDisplay.h"
#include "CesiumRuntimeSettings.h"
#include "CesiumUtility/Uri.h"
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

/*static*/ std::vector<CesiumIonTokenTroubleshooting::ExistingPanel>
    CesiumIonTokenTroubleshooting::_existingPanels{};

/*static*/ void CesiumIonTokenTroubleshooting::Open(
    CesiumIonObject ionObject,
    bool triggeredByError) {
  auto panelMatch = [ionObject](const ExistingPanel& panel) {
    return panel.pObject == ionObject;
  };

  // If a panel is already open for this object, close it.
  auto it = std::find_if(
      CesiumIonTokenTroubleshooting::_existingPanels.begin(),
      CesiumIonTokenTroubleshooting::_existingPanels.end(),
      panelMatch);
  if (it != CesiumIonTokenTroubleshooting::_existingPanels.end()) {
    TSharedRef<CesiumIonTokenTroubleshooting> pPanel = it->pPanel;
    CesiumIonTokenTroubleshooting::_existingPanels.erase(it);
    FSlateApplication::Get().RequestDestroyWindow(pPanel);
  }

  // If this is a tileset, close any already-open panels associated with its
  // overlays. Overlays won't appear until the tileset is working anyway.
  ACesium3DTileset** ppTileset = std::get_if<ACesium3DTileset*>(&ionObject);
  if (ppTileset && *ppTileset) {
    TArray<UCesiumRasterOverlay*> rasterOverlays;
    (*ppTileset)->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

    for (UCesiumRasterOverlay* pOverlay : rasterOverlays) {
      auto rasterIt = std::find_if(
          CesiumIonTokenTroubleshooting::_existingPanels.begin(),
          CesiumIonTokenTroubleshooting::_existingPanels.end(),
          [pOverlay](const ExistingPanel& candidate) {
            return candidate.pObject == CesiumIonObject(pOverlay);
          });
      if (rasterIt != CesiumIonTokenTroubleshooting::_existingPanels.end()) {
        TSharedRef<CesiumIonTokenTroubleshooting> pPanel = rasterIt->pPanel;
        CesiumIonTokenTroubleshooting::_existingPanels.erase(rasterIt);
        FSlateApplication::Get().RequestDestroyWindow(pPanel);
      }
    }
  }

  // If this is a raster overlay and this panel is already open for its attached
  // tileset, don't open the panel for the overlay for the same reason as above.
  UCesiumRasterOverlay** ppRasterOverlay =
      std::get_if<UCesiumRasterOverlay*>(&ionObject);
  if (ppRasterOverlay && *ppRasterOverlay) {
    ACesium3DTileset* pOwner =
        Cast<ACesium3DTileset>((*ppRasterOverlay)->GetOwner());
    if (pOwner) {
      auto tilesetIt = std::find_if(
          CesiumIonTokenTroubleshooting::_existingPanels.begin(),
          CesiumIonTokenTroubleshooting::_existingPanels.end(),
          [pOwner](const ExistingPanel& candidate) {
            return candidate.pObject == CesiumIonObject(pOwner);
          });
      if (tilesetIt != CesiumIonTokenTroubleshooting::_existingPanels.end()) {
        return;
      }
    }
  }

  // Open the panel
  TSharedRef<CesiumIonTokenTroubleshooting> Troubleshooting =
      SNew(CesiumIonTokenTroubleshooting)
          .IonObject(ionObject)
          .TriggeredByError(triggeredByError);

  Troubleshooting->GetOnWindowClosedEvent().AddLambda(
      [panelMatch](const TSharedRef<SWindow>& pWindow) {
        auto it = std::find_if(
            CesiumIonTokenTroubleshooting::_existingPanels.begin(),
            CesiumIonTokenTroubleshooting::_existingPanels.end(),
            panelMatch);
        if (it != CesiumIonTokenTroubleshooting::_existingPanels.end()) {
          CesiumIonTokenTroubleshooting::_existingPanels.erase(it);
        }
      });
  FSlateApplication::Get().AddWindow(Troubleshooting);

  CesiumIonTokenTroubleshooting::_existingPanels.emplace_back(
      ExistingPanel{ionObject, Troubleshooting});
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

bool isNull(const CesiumIonObject& o) {
  return std::visit([](auto p) { return p == nullptr; }, o);
}

FString getLabel(const CesiumIonObject& o) {
  struct Operation {
    FString operator()(ACesium3DTileset* pTileset) {
      return pTileset ? pTileset->GetActorLabel() : TEXT("Unknown");
    }

    FString operator()(UCesiumRasterOverlay* pRasterOverlay) {
      return pRasterOverlay ? pRasterOverlay->GetName() : TEXT("Unknown");
    }
  };

  return std::visit(Operation(), o);
}

FString getName(const CesiumIonObject& o) {
  return std::visit([](auto p) { return p->GetName(); }, o);
}

int64 getIonAssetID(const CesiumIonObject& o) {
  struct Operation {
    int64 operator()(ACesium3DTileset* pTileset) {
      if (pTileset->GetTilesetSource() != ETilesetSource::FromCesiumIon) {
        return 0;
      } else {
        return pTileset->GetIonAssetID();
      }
    }

    int64 operator()(UCesiumRasterOverlay* pRasterOverlay) {
      UCesiumIonRasterOverlay* pIon =
          Cast<UCesiumIonRasterOverlay>(pRasterOverlay);
      if (!pIon) {
        return 0;
      } else {
        return pIon->IonAssetID;
      }
    }
  };

  return std::visit(Operation(), o);
}

FString getIonAccessToken(const CesiumIonObject& o) {
  struct Operation {
    FString operator()(ACesium3DTileset* pTileset) {
      if (pTileset->GetTilesetSource() != ETilesetSource::FromCesiumIon) {
        return FString();
      } else {
        return pTileset->GetIonAccessToken();
      }
    }

    FString operator()(UCesiumRasterOverlay* pRasterOverlay) {
      UCesiumIonRasterOverlay* pIon =
          Cast<UCesiumIonRasterOverlay>(pRasterOverlay);
      if (!pIon) {
        return FString();
      } else {
        return pIon->IonAccessToken;
      }
    }
  };

  return std::visit(Operation(), o);
}

void setIonAccessToken(const CesiumIonObject& o, const FString& newToken) {
  struct Operation {
    const FString& newToken;

    void operator()(ACesium3DTileset* pTileset) {
      if (pTileset->GetIonAccessToken() != newToken) {
        pTileset->Modify();
        pTileset->SetIonAccessToken(newToken);
      } else {
        pTileset->RefreshTileset();
      }
    }

    void operator()(UCesiumRasterOverlay* pRasterOverlay) {
      UCesiumIonRasterOverlay* pIon =
          Cast<UCesiumIonRasterOverlay>(pRasterOverlay);
      if (!pIon) {
        return;
      }

      if (pIon->IonAccessToken != newToken) {
        pIon->Modify();
        pIon->IonAccessToken = newToken;
      }
      pIon->Refresh();
    }
  };

  return std::visit(Operation{newToken}, o);
}

FString getObjectType(const CesiumIonObject& o) {
  struct Operation {
    FString operator()(ACesium3DTileset* pTileset) { return TEXT("Tileset"); }

    FString operator()(UCesiumRasterOverlay* pRasterOverlay) {
      return TEXT("Raster Overlay");
    }
  };

  return std::visit(Operation(), o);
}

UObject* asUObject(const CesiumIonObject& o) {
  return std::visit([](auto p) -> UObject* { return p; }, o);
}

bool isUsingCesiumIon(const CesiumIonObject& o) {
  struct Operation {
    bool operator()(ACesium3DTileset* pTileset) {
      return pTileset->GetTilesetSource() == ETilesetSource::FromCesiumIon;
    }

    bool operator()(UCesiumRasterOverlay* pRasterOverlay) {
      UCesiumIonRasterOverlay* pIon =
          Cast<UCesiumIonRasterOverlay>(pRasterOverlay);
      return pIon != nullptr;
    }
  };

  return std::visit(Operation(), o);
}

UCesiumIonServer* getCesiumIonServer(const CesiumIonObject& o) {
  struct Operation {
    UCesiumIonServer* operator()(const ACesium3DTileset* pTileset) noexcept {
      return pTileset->GetCesiumIonServer();
    }

    UCesiumIonServer*
    operator()(const UCesiumRasterOverlay* pRasterOverlay) noexcept {
      const UCesiumIonRasterOverlay* pIon =
          Cast<UCesiumIonRasterOverlay>(pRasterOverlay);
      return pIon ? pIon->CesiumIonServer : nullptr;
    }
  };

  UCesiumIonServer* pServer = std::visit(Operation{}, o);
  if (!IsValid(pServer)) {
    pServer = UCesiumIonServer::GetDefaultServer();
  }

  return pServer;
}

CesiumIonSession& getSession(const CesiumIonObject& o) {
  return *FCesiumEditorModule::serverManager().GetSession(
      getCesiumIonServer(o));
}

} // namespace

void CesiumIonTokenTroubleshooting::Construct(const FArguments& InArgs) {
  TSharedRef<SVerticalBox> pMainVerticalBox = SNew(SVerticalBox);

  CesiumIonObject pIonObject = InArgs._IonObject;
  if (isNull(pIonObject)) {
    return;
  }

  if (!isUsingCesiumIon(pIonObject)) {
    SWindow::Construct(
        SWindow::FArguments()
            .Title(FText::FromString(FString::Format(
                TEXT("{0}: Cesium ion Token Troubleshooting"),
                {*getLabel(pIonObject)})))
            .AutoCenter(EAutoCenter::PreferredWorkArea)
            .SizingRule(ESizingRule::UserSized)
            .ClientSize(FVector2D(800, 600))
                [SNew(SBorder)
                     .Visibility(EVisibility::Visible)
                     .BorderImage(FEditorStyle::GetBrush("Menu.Background"))
                     .Padding(FMargin(10.0f, 20.0f, 10.0f, 20.0f))
                         [SNew(STextBlock)
                              .AutoWrapText(true)
                              .Text(FText::FromString(TEXT(
                                  "This object is not configured to connect to Cesium ion.")))]]);
    return;
  }

  this->_pIonObject = pIonObject;

  if (InArgs._TriggeredByError) {
    FString label = getLabel(pIonObject);
    FString name = getName(pIonObject);
    FString descriptor =
        label == name ? FString::Format(TEXT("\"{0}\""), {name})
                      : FString::Format(TEXT("\"{0}\" ({1})"), {label, name});

    FString preamble = FString::Format(
        TEXT(
            "{0} {1} tried to access Cesium ion for asset ID {2}, but it didn't work, probably due to a problem with the access token. This panel will help you fix it!"),
        {getObjectType(pIonObject), *descriptor, getIonAssetID(pIonObject)});

    pMainVerticalBox->AddSlot().AutoHeight()
        [SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(preamble))];
  }

  pMainVerticalBox->AddSlot().AutoHeight().Padding(
      5.0f)[SNew(CesiumIonServerDisplay)
                .Server(getCesiumIonServer(pIonObject))];

  TSharedRef<SHorizontalBox> pDiagnosticColumns = SNew(SHorizontalBox);

  if (!getIonAccessToken(pIonObject).IsEmpty()) {
    this->_assetTokenState.name = FString::Format(
        TEXT("This {0}'s Access Token"),
        {getObjectType(pIonObject)});
    this->_assetTokenState.token = getIonAccessToken(pIonObject);
    pDiagnosticColumns->AddSlot()
        .Padding(5.0f, 20.0f, 5.0f, 5.0f)
        .VAlign(EVerticalAlignment::VAlign_Top)
        .AutoWidth()
        .FillWidth(
            0.5f)[this->createTokenPanel(pIonObject, this->_assetTokenState)];
  }

  this->_projectDefaultTokenState.name = TEXT("Project Default Access Token");
  this->_projectDefaultTokenState.token =
      getCesiumIonServer(pIonObject)->DefaultIonAccessToken;

  pDiagnosticColumns->AddSlot()
      .Padding(5.0f, 20.0f, 5.0f, 5.0f)
      .VAlign(EVerticalAlignment::VAlign_Top)
      .AutoWidth()
      .FillWidth(0.5f)
          [this->createTokenPanel(pIonObject, this->_projectDefaultTokenState)];

  if (getSession(this->_pIonObject).isConnected()) {
    // Don't let this panel be destroyed while the async operations below are in
    // progress.
    TSharedRef<CesiumIonTokenTroubleshooting> pPanel =
        StaticCastSharedRef<CesiumIonTokenTroubleshooting>(this->AsShared());

    getSession(this->_pIonObject)
        .getConnection()
        ->asset(getIonAssetID(pIonObject))
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
      FString::Format(
          TEXT("Use the project default token for this {0}"),
          {getObjectType(pIonObject)}),
      &CesiumIonTokenTroubleshooting::canUseProjectDefaultToken,
      &CesiumIonTokenTroubleshooting::useProjectDefaultToken);

  this->addRemedyButton(
      pMainVerticalBox,
      FString::Format(
          TEXT("Authorize the {0}'s token to access this asset"),
          {getObjectType(pIonObject)}),
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
                   " - The {1}'s \"Ion Asset ID\" property is correct.\n"
                   " - If the asset is from the \"Asset Depot\", verify that it has been added to \"My Assets\"."),
               {getIonAssetID(pIonObject), getObjectType(pIonObject)})))];

  this->addRemedyButton(
      pMainVerticalBox,
      TEXT("Open Cesium ion on the Web"),
      &CesiumIonTokenTroubleshooting::canOpenCesiumIon,
      &CesiumIonTokenTroubleshooting::openCesiumIon);

  SWindow::Construct(
      SWindow::FArguments()
          .Title(FText::FromString(FString::Format(
              TEXT("{0}: Cesium ion Token Troubleshooting"),
              {*getLabel(pIonObject)})))
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
    const CesiumIonObject& pIonObject,
    TokenState& state) {
  CesiumIonSession& ionSession = getSession(pIonObject);

  int64 assetID = getIonAssetID(pIonObject);

  auto pConnection = std::make_shared<Connection>(
      ionSession.getAsyncSystem(),
      ionSession.getAssetAccessor(),
      TCHAR_TO_UTF8(*state.token),
      TCHAR_TO_UTF8(*getCesiumIonServer(pIonObject)->ApiUrl));

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
          CesiumIonSession& ionSession = getSession(pPanel->_pIonObject);
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
  return !getSession(this->_pIonObject).isConnected();
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
  getSession(this->_pIonObject).connect();
}

bool CesiumIonTokenTroubleshooting::canUseProjectDefaultToken() const {
  const TokenState& state = this->_projectDefaultTokenState;
  return !isNull(this->_pIonObject) &&
         !getIonAccessToken(this->_pIonObject).IsEmpty() &&
         state.isValid == true && state.allowsAccessToAsset == true;
}

void CesiumIonTokenTroubleshooting::useProjectDefaultToken() {
  if (isNull(this->_pIonObject)) {
    return;
  }

  FScopedTransaction transaction(
      FText::FromString("Use Project Default Token"));
  setIonAccessToken(this->_pIonObject, FString());
}

bool CesiumIonTokenTroubleshooting::canAuthorizeAssetToken() const {
  const TokenState& state = this->_assetTokenState;
  return this->_assetExistsInUserAccount == true && state.isValid == true &&
         state.allowsAccessToAsset == false &&
         state.associatedWithUserAccount == true;
}

void CesiumIonTokenTroubleshooting::authorizeAssetToken() {
  if (isNull(this->_pIonObject)) {
    return;
  }
  this->authorizeToken(getIonAccessToken(this->_pIonObject), false);
}

bool CesiumIonTokenTroubleshooting::canAuthorizeProjectDefaultToken() const {
  const TokenState& state = this->_projectDefaultTokenState;
  return this->_assetExistsInUserAccount == true && state.isValid == true &&
         state.allowsAccessToAsset == false &&
         state.associatedWithUserAccount == true;
}

void CesiumIonTokenTroubleshooting::authorizeProjectDefaultToken() {
  UCesiumIonServer* pServer = getCesiumIonServer(this->_pIonObject);
  this->authorizeToken(pServer->DefaultIonAccessToken, true);
}

bool CesiumIonTokenTroubleshooting::canSelectNewProjectDefaultToken() const {
  if (this->_assetExistsInUserAccount == false) {
    return false;
  }

  const TokenState& state = this->_projectDefaultTokenState;
  return getSession(this->_pIonObject).isConnected() &&
         (state.isValid == false || (state.allowsAccessToAsset == false &&
                                     state.associatedWithUserAccount == false));
}

void CesiumIonTokenTroubleshooting::selectNewProjectDefaultToken() {
  if (isNull(this->_pIonObject)) {
    return;
  }

  CesiumIonSession& session = getSession(this->_pIonObject);
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

  SelectCesiumIonToken::SelectNewToken(getCesiumIonServer(this->_pIonObject))
      .thenInMainThread([pPanel](const std::optional<Token>& newToken) {
        if (!newToken) {
          return;
        }

        pPanel->useProjectDefaultToken();
      });
}

bool CesiumIonTokenTroubleshooting::canOpenCesiumIon() const {
  return getSession(this->_pIonObject).isConnected();
}

void CesiumIonTokenTroubleshooting::openCesiumIon() {
  UCesiumIonServer* pServer = getCesiumIonServer(this->_pIonObject);
  FPlatformProcess::LaunchURL(
      UTF8_TO_TCHAR(CesiumUtility::Uri::resolve(
                        TCHAR_TO_UTF8(*pServer->ServerUrl),
                        "tokens")
                        .c_str()),
      NULL,
      NULL);
}

void CesiumIonTokenTroubleshooting::authorizeToken(
    const FString& token,
    bool removeObjectToken) {
  if (isNull(this->_pIonObject)) {
    return;
  }

  CesiumIonSession& session = getSession(this->_pIonObject);
  const std::optional<Connection>& maybeConnection = session.getConnection();
  if (!session.isConnected() || !maybeConnection) {
    UE_LOG(
        LogCesiumEditor,
        Error,
        TEXT(
            "Cannot grant a token access to an asset because you are not signed in to Cesium ion."));
    return;
  }

  TWeakObjectPtr<UObject> pStillAlive = asUObject(this->_pIonObject);

  session.findToken(token).thenInMainThread([pStillAlive,
                                             removeObjectToken,
                                             pIonObject = this->_pIonObject,
                                             ionAssetID = getIonAssetID(
                                                 this->_pIonObject),
                                             connection = *maybeConnection](
                                                Response<Token>&& response) {
    if (!pStillAlive.IsValid()) {
      // UObject has been destroyed
      return connection.getAsyncSystem().createResolvedFuture();
    }

    if (!response.value) {
      UE_LOG(
          LogCesiumEditor,
          Error,
          TEXT(
              "Cannot grant a token access to an asset because the token was not found in the signed-in Cesium ion account."));
      return connection.getAsyncSystem().createResolvedFuture();
    }

    if (!response.value->assetIds) {
      UE_LOG(
          LogCesiumEditor,
          Warning,
          TEXT(
              "Cannot grant a token access to an asset because the token appears to already have access to all assets."));
      return connection.getAsyncSystem().createResolvedFuture();
    }

    auto it = std::find(
        response.value->assetIds->begin(),
        response.value->assetIds->end(),
        ionAssetID);
    if (it != response.value->assetIds->end()) {
      UE_LOG(
          LogCesiumEditor,
          Warning,
          TEXT(
              "Cannot grant a token access to an asset because the token appears to already have access to the asset."));
      return connection.getAsyncSystem().createResolvedFuture();
    }

    response.value->assetIds->emplace_back(ionAssetID);

    return connection
        .modifyToken(
            response.value->id,
            response.value->name,
            response.value->assetIds,
            response.value->scopes,
            response.value->allowedUrls)
        .thenInMainThread([pIonObject, pStillAlive, removeObjectToken](
                              Response<NoValue>&& result) {
          if (result.value) {
            // Refresh the object now that the token is valid (hopefully).
            if (pStillAlive.IsValid()) {
              if (removeObjectToken) {
                setIonAccessToken(pIonObject, FString());
              } else {
                // Set the token to the same value to force a refresh.
                setIonAccessToken(pIonObject, getIonAccessToken(pIonObject));
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

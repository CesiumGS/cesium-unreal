// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumIonServerSelector.h"
#include "CesiumEditor.h"
#include "CesiumIonServer.h"
#include "Editor.h"
#include "PropertyCustomizationHelpers.h"

void CesiumIonServerSelector::Construct(const FArguments& InArgs) {
  ChildSlot
      [SNew(SHorizontalBox) +
       SHorizontalBox::Slot().FillWidth(1.0f).VAlign(
           EVerticalAlignment::VAlign_Center)
           [SNew(SComboBox<TObjectPtr<UCesiumIonServer>>)
                .OptionsSource(
                    &FCesiumEditorModule::serverManager().GetServerList())
                .OnGenerateWidget(
                    this,
                    &CesiumIonServerSelector::OnGenerateServerEntry)
                .OnSelectionChanged(
                    this,
                    &CesiumIonServerSelector::OnServerSelectionChanged)
                .Content()
                    [SNew(STextBlock)
                         .Text(
                             this,
                             &CesiumIonServerSelector::GetServerValueAsText)]] +
       SHorizontalBox::Slot().AutoWidth().VAlign(
           EVerticalAlignment::VAlign_Center)
           [PropertyCustomizationHelpers::MakeBrowseButton(
               FSimpleDelegate::
                   CreateSP(this, &CesiumIonServerSelector::OnBrowseForServer),
               FText::FromString(
                   "Show this Cesium ion Server in the Content Browser."),
               true,
               false)]];
}

namespace {

FText GetNameFromCesiumIonServerAsset(
    const TObjectPtr<UCesiumIonServer>& pServer) {
  if (!pServer)
    return FText::FromString("Error: No Cesium ion server configured.");

  std::shared_ptr<CesiumIonSession> pSession =
      FCesiumEditorModule::serverManager().GetSession(pServer);

  // Get the profile here, which will trigger it to load if it hasn't been
  // loaded already.
  const CesiumIonClient::Profile& profile = pSession->getProfile();

  FString prefix;
  FString suffix;

  if (pSession->isConnecting() || pSession->isResuming()) {
    suffix = " (connecting...)";
  } else if (pSession->isLoadingProfile()) {
    suffix = " (loading profile...)";
  } else if (pSession->isConnected() && pSession->isProfileLoaded()) {
    prefix = FString(UTF8_TO_TCHAR(profile.username.c_str()));
    prefix += " @ ";
  } else {
    suffix = " (not connected)";
  }

  return FText::FromString(
      prefix +
      (pServer->DisplayName.IsEmpty() ? pServer->GetPackage()->GetName()
                                      : pServer->DisplayName) +
      suffix);
}

} // namespace

FText CesiumIonServerSelector::GetServerValueAsText() const {
  UCesiumIonServer* pServer =
      FCesiumEditorModule::serverManager().GetCurrentServer();
  return GetNameFromCesiumIonServerAsset(pServer);
}

TSharedRef<SWidget> CesiumIonServerSelector::OnGenerateServerEntry(
    TObjectPtr<UCesiumIonServer> pServerAsset) {
  return SNew(STextBlock).Text_Lambda([pServerAsset]() {
    return GetNameFromCesiumIonServerAsset(pServerAsset);
  });
}

void CesiumIonServerSelector::OnServerSelectionChanged(
    TObjectPtr<UCesiumIonServer> InItem,
    ESelectInfo::Type InSeletionInfo) {
  FCesiumEditorModule::serverManager().SetCurrentServer(InItem);
  FCesiumEditorModule::serverManager().GetCurrentSession()->resume();
}

void CesiumIonServerSelector::OnBrowseForServer() {
  TArray<UObject*> Objects;
  Objects.Add(FCesiumEditorModule::serverManager().GetCurrentServer());
  GEditor->SyncBrowserToObjects(Objects);
}

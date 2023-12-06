// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "IonLoginPanel.h"
#include "CesiumEditor.h"
#include "CesiumIonClient/Connection.h"
#include "CesiumIonClient/Token.h"
#include "CesiumIonServer.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Misc/App.h"
#include "Styling/SlateStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

using namespace CesiumIonClient;

void IonLoginPanel::Construct(const FArguments& InArgs) {

  auto visibleWhenConnecting = [this]() {
    return FCesiumEditorModule::serverManager()
                       .GetCurrentSession()
                       ->isConnecting() &&
                   !FCesiumEditorModule::serverManager()
                        .GetCurrentServer()
                        ->ApiUrl.IsEmpty()
               ? EVisibility::Visible
               : EVisibility::Collapsed;
  };

  auto visibleWhenResuming = [this]() {
    return FCesiumEditorModule::serverManager()
                   .GetCurrentSession()
                   ->isResuming()
               ? EVisibility::Visible
               : EVisibility::Collapsed;
  };

  auto visibleWhenNotConnectingOrResuming = []() {
    return FCesiumEditorModule::serverManager()
                       .GetCurrentSession()
                       ->isConnecting() ||
                   FCesiumEditorModule::serverManager()
                       .GetCurrentSession()
                       ->isResuming()
               ? EVisibility::Collapsed
               : EVisibility::Visible;
  };

  // TODO Format this, and disable clang format here

  TSharedPtr<SVerticalBox> connectionStatusWidget =
      SNew(SVerticalBox).Visibility_Lambda(visibleWhenConnecting) +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .Padding(5, 15, 5, 5)
          .AutoHeight()
              [SNew(STextBlock)
                   .Text(FText::FromString(TEXT(
                       "Waiting for you to sign into Cesium ion with your web browser...")))
                   .AutoWrapText(true)] +
      SVerticalBox::Slot()
          .HAlign(HAlign_Center)
          .Padding(5)[SNew(SThrobber).Animate(SThrobber::Horizontal)] +
      SVerticalBox::Slot()
          .HAlign(HAlign_Center)
          .Padding(5)
          .AutoHeight()
              [SNew(SHyperlink)
                   .OnNavigate(this, &IonLoginPanel::LaunchBrowserAgain)
                   .Text(FText::FromString(TEXT("Open web browser again")))] +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .Padding(5)
          .AutoHeight()[SNew(STextBlock)
                            .Text(FText::FromString(TEXT(
                                "Or copy the URL below into your web browser")))
                            .AutoWrapText(true)] +
      SVerticalBox::Slot()
          .HAlign(HAlign_Center)
          .AutoHeight()
              [SNew(SHorizontalBox) +
               SHorizontalBox::Slot().VAlign(VAlign_Center)[SNew(
                   SBorder)[SNew(SEditableText)
                                .IsReadOnly(true)
                                .Text_Lambda([this]() {
                                  return FText::FromString(UTF8_TO_TCHAR(
                                      FCesiumEditorModule::serverManager()
                                          .GetCurrentSession()
                                          ->getAuthorizeUrl()
                                          .c_str()));
                                })]] +
               SHorizontalBox::Slot()
                   .VAlign(VAlign_Center)
                   .HAlign(HAlign_Right)
                   .AutoWidth()
                       [SNew(SButton)
                            .OnClicked(
                                this,
                                &IonLoginPanel::CopyAuthorizeUrlToClipboard)
                            .Text(
                                FText::FromString(TEXT("Copy to clipboard")))]];

  TSharedPtr<SVerticalBox> connectionWidget =
      SNew(SVerticalBox) +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .HAlign(HAlign_Center)
          .Padding(5)
          .AutoHeight()
              [SNew(SButton)
                   .Visibility_Lambda(visibleWhenNotConnectingOrResuming)
                   .ButtonStyle(FCesiumEditorModule::GetStyle(), "CesiumButton")
                   .TextStyle(
                       FCesiumEditorModule::GetStyle(),
                       "CesiumButtonText")
                   .OnClicked(this, &IonLoginPanel::SignIn)
                   .Text(FText::FromString(TEXT("Connect to Cesium ion")))] +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .HAlign(HAlign_Center)
          .Padding(5)
          .AutoHeight()
              [SNew(SButton)
                   .Visibility_Lambda(visibleWhenConnecting)
                   .ButtonStyle(FCesiumEditorModule::GetStyle(), "CesiumButton")
                   .TextStyle(
                       FCesiumEditorModule::GetStyle(),
                       "CesiumButtonText")
                   .OnClicked(this, &IonLoginPanel::CancelSignIn)
                   .Text(FText::FromString(TEXT("Cancel Connecting")))] +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .Padding(10, 0, 10, 5)
          .AutoHeight()
              [SNew(STextBlock)
                   .Visibility_Lambda([visibleWhenNotConnectingOrResuming]() {
                     // Only show this message for the SaaS server.
                     UCesiumIonServer* Server =
                         FCesiumEditorModule::serverManager()
                             .GetCurrentServer();
                     if (Server->GetName() != TEXT("CesiumIonSaaS"))
                       return EVisibility::Collapsed;

                     return visibleWhenNotConnectingOrResuming();
                   })
                   .AutoWrapText(true)
                   .TextStyle(FCesiumEditorModule::GetStyle(), "BodyBold")
                   .Text(FText::FromString(TEXT(
                       "You can now sign in with your Epic Games account!")))] +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .Padding(5, 15, 5, 5)
          .AutoHeight()[SNew(STextBlock)
                            .Text(FText::FromString(
                                TEXT("Resuming the previous connection...")))
                            .Visibility_Lambda(visibleWhenResuming)
                            .AutoWrapText(true)] +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .AutoHeight()[connectionStatusWidget.ToSharedRef()];

  ChildSlot
      [SNew(SScrollBox) +
       SScrollBox::Slot()
           .VAlign(VAlign_Top)
           .HAlign(HAlign_Center)
           .Padding(20, 0, 20, 5)
               [SNew(SScaleBox)
                    .Stretch(EStretch::ScaleToFit)
                    .HAlign(HAlign_Center)
                    .VAlign(VAlign_Top)[SNew(SImage).Image(
                        FCesiumEditorModule::GetStyle()->GetBrush(
                            TEXT("Cesium.Logo")))]] +
       SScrollBox::Slot()
           .VAlign(VAlign_Top)
           .Padding(30, 10, 30, 10)
               [SNew(STextBlock)
                    .AutoWrapText(true)
                    .Text(FText::FromString(TEXT(
                        "Access global high-resolution 3D content, including photogrammetry, terrain, imagery, and buildings. Bring your own data for tiling, hosting, and streaming to Unreal Engine.")))] +
       SScrollBox::Slot()
           .VAlign(VAlign_Top)
           .HAlign(HAlign_Center)
           .Padding(20)[connectionWidget.ToSharedRef()]];
}

FReply IonLoginPanel::SignIn() {
  FCesiumEditorModule::serverManager().GetCurrentSession()->connect();
  return FReply::Handled();
}

FReply IonLoginPanel::CopyAuthorizeUrlToClipboard() {
  FText url =
      FText::FromString(UTF8_TO_TCHAR(FCesiumEditorModule::serverManager()
                                          .GetCurrentSession()
                                          ->getAuthorizeUrl()
                                          .c_str()));
  FPlatformApplicationMisc::ClipboardCopy(*url.ToString());
  return FReply::Handled();
}

void IonLoginPanel::LaunchBrowserAgain() {
  FPlatformProcess::LaunchURL(
      UTF8_TO_TCHAR(FCesiumEditorModule::serverManager()
                        .GetCurrentSession()
                        ->getAuthorizeUrl()
                        .c_str()),
      NULL,
      NULL);
}

FReply IonLoginPanel::CancelSignIn() {
  TSharedRef<IHttpRequest, ESPMode::ThreadSafe> pRequest =
      FHttpModule::Get().CreateRequest();
  pRequest->SetURL(UTF8_TO_TCHAR(FCesiumEditorModule::serverManager()
                                     .GetCurrentSession()
                                     ->getRedirectUrl()
                                     .c_str()));
  pRequest->ProcessRequest();
  return FReply::Handled();
}

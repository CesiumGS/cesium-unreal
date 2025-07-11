// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "ITwinLoginPanel.h"
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

void ITwinLoginPanel::Construct(const FArguments& InArgs) {

  auto visibleWhenConnecting = [this]() {
    return FCesiumEditorModule::iTwinSession().isConnecting()
               ? EVisibility::Visible
               : EVisibility::Collapsed;
  };

  auto visibleWhenNotConnectingOrResuming = []() {
    return FCesiumEditorModule::iTwinSession().isConnecting()
               ? EVisibility::Collapsed
               : EVisibility::Visible;
  };

  TSharedPtr<SVerticalBox> connectionStatusWidget =
      SNew(SVerticalBox).Visibility_Lambda(visibleWhenConnecting) +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .Padding(5, 15, 5, 5)
          .AutoHeight()
              [SNew(STextBlock)
                   .Text(FText::FromString(TEXT(
                       "Waiting for you to sign into Bentley iTwin with your web browser...")))
                   .AutoWrapText(true)] +
      SVerticalBox::Slot()
          .HAlign(HAlign_Center)
          .Padding(5)[SNew(SThrobber).Animate(SThrobber::Horizontal)] +
      SVerticalBox::Slot()
          .HAlign(HAlign_Center)
          .Padding(5)
          .AutoHeight()
              [SNew(SHyperlink)
                   .OnNavigate(this, &ITwinLoginPanel::LaunchBrowserAgain)
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
                                      FCesiumEditorModule::iTwinSession()
                                          .getAuthorizeUrl()
                                          .c_str()));
                                })]] +
               SHorizontalBox::Slot()
                   .VAlign(VAlign_Center)
                   .HAlign(HAlign_Right)
                   .AutoWidth()
                       [SNew(SButton)
                            .OnClicked(
                                this,
                                &ITwinLoginPanel::CopyAuthorizeUrlToClipboard)
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
                   .OnClicked(this, &ITwinLoginPanel::SignIn)
                   .Text(FText::FromString(TEXT("Connect to Bentley iTwin")))] +
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
                   .OnClicked(this, &ITwinLoginPanel::CancelSignIn)
                   .Text(FText::FromString(TEXT("Cancel Connecting")))] +
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
                            TEXT("ITwin.Logo")))]] +
       SScrollBox::Slot()
           .VAlign(VAlign_Top)
           .Padding(30, 10, 30, 10)
               [SNew(STextBlock)
                    .AutoWrapText(true)
                    .Text(FText::FromString(TEXT(
                        "Load iModels, Reality Data, and Cesium Curated Content from your Bentley account by signing in.")))] +
       SScrollBox::Slot()
           .VAlign(VAlign_Top)
           .HAlign(HAlign_Center)
           .Padding(20)[connectionWidget.ToSharedRef()]];
}

FReply ITwinLoginPanel::SignIn() {
  FCesiumEditorModule::iTwinSession().connect();
  return FReply::Handled();
}

FReply ITwinLoginPanel::CopyAuthorizeUrlToClipboard() {
  FText url = FText::FromString(UTF8_TO_TCHAR(
      FCesiumEditorModule::iTwinSession().getAuthorizeUrl().c_str()));
  FPlatformApplicationMisc::ClipboardCopy(*url.ToString());
  return FReply::Handled();
}

void ITwinLoginPanel::LaunchBrowserAgain() {
  FPlatformProcess::LaunchURL(
      UTF8_TO_TCHAR(
          FCesiumEditorModule::iTwinSession().getAuthorizeUrl().c_str()),
      NULL,
      NULL);
}

FReply ITwinLoginPanel::CancelSignIn() {
  TSharedRef<IHttpRequest, ESPMode::ThreadSafe> pRequest =
      FHttpModule::Get().CreateRequest();
  pRequest->SetURL(UTF8_TO_TCHAR(
      FCesiumEditorModule::iTwinSession().getRedirectUrl().c_str()));
  pRequest->ProcessRequest();
  return FReply::Handled();
}

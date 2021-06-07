// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "IonLoginPanel.h"
#include "CesiumEditor.h"
#include "CesiumIonClient/Connection.h"
#include "CesiumIonClient/Token.h"
#include "HAL/PlatformApplicationMisc.h"
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
    return FCesiumEditorModule::ion().isConnecting() ? EVisibility::Visible
                                                     : EVisibility::Hidden;
  };

  auto visibleWhenResuming = [this]() {
    return FCesiumEditorModule::ion().isResuming() ? EVisibility::Visible
                                                   : EVisibility::Hidden;
  };

  // TODO Format this, and disable clang format here

  TSharedPtr<SVerticalBox> connectionWidget =
      SNew(SVerticalBox) +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .HAlign(HAlign_Center)
          .Padding(5)
          .AutoHeight()[SNew(SButton)
                            .OnClicked(this, &IonLoginPanel::SignIn)
                            .Text(FText::FromString(TEXT("Connect")))
                            .IsEnabled_Lambda([this]() {
                              return !FCesiumEditorModule::ion()
                                          .isConnecting() &&
                                     !FCesiumEditorModule::ion().isResuming();
                            })] +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .HAlign(HAlign_Fill)
          .Padding(5, 15, 5, 5)
          .AutoHeight()
              [SNew(STextBlock)
                   .Text(FText::FromString(TEXT(
                       "Waiting for you to sign into Cesium ion with your web browser...")))
                   .Visibility_Lambda(visibleWhenConnecting)
                   .AutoWrapText(true)] +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .HAlign(HAlign_Fill)
          .Padding(5, 15, 5, 5)
          .AutoHeight()[SNew(STextBlock)
                            .Text(FText::FromString(
                                TEXT("Resuming the previous connection...")))
                            .Visibility_Lambda(visibleWhenResuming)
                            .AutoWrapText(true)] +
      SVerticalBox::Slot()
          .HAlign(HAlign_Center)
          .Padding(5)[SNew(SThrobber)
                          .Animate(SThrobber::Horizontal)
                          .Visibility_Lambda(visibleWhenConnecting)] +
      SVerticalBox::Slot()
          .HAlign(HAlign_Center)
          .Padding(5)
          .AutoHeight()
              [SNew(SHyperlink)
                   .OnNavigate(this, &IonLoginPanel::LaunchBrowserAgain)
                   .Text(FText::FromString(TEXT("Open web browser again")))
                   .Visibility_Lambda(visibleWhenConnecting)] +
      SVerticalBox::Slot()
          .VAlign(VAlign_Top)
          .HAlign(HAlign_Fill)
          .Padding(5)
          .AutoHeight()[SNew(STextBlock)
                            .Text(FText::FromString(TEXT(
                                "Or copy the URL below into your web browser")))
                            .Visibility_Lambda(visibleWhenConnecting)
                            .AutoWrapText(true)] +
      SVerticalBox::Slot()
          .HAlign(HAlign_Center)
          .AutoHeight()
              [SNew(SHorizontalBox).Visibility_Lambda(visibleWhenConnecting) +
               SHorizontalBox::Slot()
                   .HAlign(HAlign_Fill)
                   .VAlign(VAlign_Center)[SNew(
                       SBorder)[SNew(SEditableText)
                                    .IsReadOnly(true)
                                    .Text_Lambda([this]() {
                                      return FText::FromString(UTF8_TO_TCHAR(
                                          FCesiumEditorModule::ion()
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
                                &IonLoginPanel::CopyAuthorizeUrlToClipboard)
                            .Text(
                                FText::FromString(TEXT("Copy to clipboard")))]];

  ChildSlot
      [SNew(SScrollBox) +
       SScrollBox::Slot()
           .VAlign(VAlign_Top)
           .HAlign(HAlign_Center)
           .Padding(20)[SNew(SScaleBox)
                            .Stretch(EStretch::ScaleToFit)
                            .HAlign(HAlign_Center)
                            .VAlign(VAlign_Top)[SNew(SImage).Image(
                                FCesiumEditorModule::GetStyle()->GetBrush(
                                    TEXT("Cesium.Logo")))]] +
       SScrollBox::Slot()
           .VAlign(VAlign_Top)
           .HAlign(HAlign_Fill)
           .Padding(10)
               [SNew(STextBlock)
                    .AutoWrapText(true)
                    .Text(FText::FromString(TEXT(
                        "Sign in to Cesium ion to access global high-resolution 3D content, including photogrammetry, terrain, imagery, and buildings. Bring your own data for tiling, hosting, and streaming to Unreal Engine.")))] +
       SScrollBox::Slot()
           .VAlign(VAlign_Top)
           .HAlign(HAlign_Center)
           .Padding(20)[connectionWidget.ToSharedRef()]];
}

FReply IonLoginPanel::SignIn() {
  FCesiumEditorModule::ion().connect();
  return FReply::Handled();
}

FReply IonLoginPanel::CopyAuthorizeUrlToClipboard() {
  FText url = FText::FromString(
      UTF8_TO_TCHAR(FCesiumEditorModule::ion().getAuthorizeUrl().c_str()));
  FPlatformApplicationMisc::ClipboardCopy(*url.ToString());
  return FReply::Handled();
}

void IonLoginPanel::LaunchBrowserAgain() {
  FPlatformProcess::LaunchURL(
      UTF8_TO_TCHAR(FCesiumEditorModule::ion().getAuthorizeUrl().c_str()),
      NULL,
      NULL);
}
#include "IonLoginPanel.h"
#include "CesiumEditor.h"
#include "Styling/SlateStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "CesiumIonClient/CesiumIonConnection.h"
#include "UnrealConversions.h"

using namespace CesiumIonClient;

void IonLoginPanel::Construct(const FArguments& InArgs)
{
    ChildSlot [
        SNew(SScrollBox)
        + SScrollBox::Slot()
            .VAlign(VAlign_Top)
            .HAlign(HAlign_Center)
            .Padding(20)
        [
            SNew(STextBlock)
                .TextStyle(FCesiumEditorModule::GetStyle(), TEXT("WelcomeText"))
                .Text(FText::FromString(TEXT("Welcome to Cesium for Unreal")))
        ]
        + SScrollBox::Slot()
            .VAlign(VAlign_Top)
            .HAlign(HAlign_Center)
            .Padding(5)
        [
            SNew(STextBlock)
                .AutoWrapText(true)
                .Text(FText::FromString(TEXT("Cesium for Unreal is better with Cesium ion. Access global terrain, imagery, and buildings. Upload your own data for efficient streaming.")))
        ]
        + SScrollBox::Slot()
            .VAlign(VAlign_Top)
            .HAlign(HAlign_Center)
            .Padding(20)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
                .VAlign(VAlign_Top)
                .HAlign(HAlign_Center)
                .Padding(5)
                .AutoHeight()
            [
                SNew(SButton)
                    .Text(FText::FromString(TEXT("Create a Cesium ion Account")))
            ]
            + SVerticalBox::Slot()
                .VAlign(VAlign_Top)
                .HAlign(HAlign_Center)
                .Padding(20)
            [
                SNew(STextBlock)
                    .Text(FText::FromString(TEXT("-or sign in-")))
            ]
            + SVerticalBox::Slot()
                .VAlign(VAlign_Top)
                .HAlign(HAlign_Left)
                .Padding(5)
            [
                SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Username or email")))
            ]
            + SVerticalBox::Slot()
                .VAlign(VAlign_Top)
                .HAlign(HAlign_Left)
                .Padding(5)
            [
                SNew(SEditableTextBox)
                    .MinDesiredWidth(200.0f)
                    .OnTextCommitted(this, &IonLoginPanel::setUsername)
                    
            ]
            + SVerticalBox::Slot()
                .VAlign(VAlign_Top)
                .HAlign(HAlign_Left)
                .Padding(5)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Password")))
            ]
            + SVerticalBox::Slot()
                .VAlign(VAlign_Top)
                .HAlign(HAlign_Left)
                .Padding(5)
            [
                SNew(SEditableTextBox)
                    .MinDesiredWidth(200.0f)
                    .IsPassword(true)
                    .OnTextCommitted(this, &IonLoginPanel::setPassword)
            ]
            + SVerticalBox::Slot()
                .VAlign(VAlign_Top)
                .HAlign(HAlign_Center)
                .Padding(5)
                .AutoHeight()
            [
                SNew(SButton)
                    .OnClicked(this, &IonLoginPanel::SignIn)
                    .Text(FText::FromString(TEXT("Sign in")))
                    .IsEnabled_Lambda([this]() { return !this->_signInInProgress; })
            ]
            + SVerticalBox::Slot()
                .HAlign(HAlign_Center)
            [
                SNew(SThrobber)
                    .Animate(SThrobber::Horizontal)
                    .Visibility_Lambda([this]() { return this->_signInInProgress ? EVisibility::Visible : EVisibility::Hidden; })
            ]
        ]
    ];
}

void IonLoginPanel::setUsername(const FText& value, ETextCommit::Type commitInfo) {
    this->_username = value;
}

void IonLoginPanel::setPassword(const FText& value, ETextCommit::Type commitInfo) {
    this->_password = value;

    if (commitInfo == ETextCommit::OnEnter) {
        this->SignIn();
    }
}

FReply IonLoginPanel::SignIn() {
    this->_signInInProgress = true;

    CesiumIonConnection::connect(
        *FCesiumEditorModule::ion().pAsyncSystem,
        wstr_to_utf8(this->_username.ToString()),
        wstr_to_utf8(this->_password.ToString())
    ).thenInMainThread([](CesiumIonConnection::Response<CesiumIonConnection>&& response) {
        FCesiumEditorModule::ion().connection = std::move(response.value);
        if (FCesiumEditorModule::ion().connection) {
            return FCesiumEditorModule::ion().connection.value().me();
        } else {
            return FCesiumEditorModule::ion().pAsyncSystem->createResolvedFuture(CesiumIonConnection::Response<CesiumIonProfile>{});
        }
    }).thenInMainThread([this](CesiumIonConnection::Response<CesiumIonProfile>&& profile) {
        FCesiumEditorModule::ion().profile = std::move(profile.value);
        this->_signInInProgress = false;
        FCesiumEditorModule::ion().ionConnectionChanged.Broadcast();
    }).catchInMainThread([this](const std::exception& /*e*/) {
        this->_signInInProgress = false;
    });

    return FReply::Handled();
}
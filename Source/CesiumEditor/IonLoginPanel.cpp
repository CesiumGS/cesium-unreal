#include "IonLoginPanel.h"
#include "CesiumEditor.h"
#include "Styling/SlateStyle.h"
#include "Misc/App.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "CesiumIonClient/CesiumIonConnection.h"
#include "CesiumIonClient/CesiumIonToken.h"
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
            SNew(SScaleBox)
            .Stretch(EStretch::ScaleToFit)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Top)
            [
                SNew(SImage)
                    .Image(FCesiumEditorModule::GetStyle()->GetBrush(TEXT("Cesium.Logo")))
            ]
        ]
        + SScrollBox::Slot()
            .VAlign(VAlign_Top)
            .HAlign(HAlign_Fill)
            .Padding(10)
        [
            SNew(STextBlock)
                //.WrapTextAt(300.0f)
                .AutoWrapText(true)
                .Text(FText::FromString(TEXT("Sign in to Cesium ion to access global high-resolution 3D content, including photogrammetry, terrain, imagery, and buildings. Bring your own data for tiling, hosting, and streaming to Unreal Engine.")))
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
    ).thenInMainThread([this](CesiumIonConnection::Response<CesiumIonConnection>&& response) -> CesiumAsync::Future<void> {
        FCesiumEditorModule::ion().connection = std::move(response.value);
        FCesiumEditorModule::ion().profile = {};
        FCesiumEditorModule::ion().token = "";

        if (!FCesiumEditorModule::ion().connection) {
            return FCesiumEditorModule::ion().pAsyncSystem->createResolvedFuture();
        }

        return FCesiumEditorModule::ion().connection.value().me().thenInMainThread([this](CesiumIonConnection::Response<CesiumIonProfile>&& profile) {
            this->_signInInProgress = false;
            FCesiumEditorModule::ion().profile = std::move(profile.value);
            FCesiumEditorModule::ion().ionConnectionChanged.Broadcast();
            return FCesiumEditorModule::ion().connection.value().tokens();
        }).thenInMainThread([this](CesiumIonConnection::Response<std::vector<CesiumIonToken>>&& tokens) -> CesiumAsync::Future<void> {
            if (!tokens.value) {
                return FCesiumEditorModule::ion().pAsyncSystem->createResolvedFuture();
            }

            // Super-hackily add all the "premium assets" that we need for quick add to our assets.
            // TODO: less hackily maybe?
            FCesiumEditorModule::ion().pAsyncSystem->post("https://api.cesium.com/premiumAssets/1/add"); // Cesium World Terrain
            FCesiumEditorModule::ion().pAsyncSystem->post("https://api.cesium.com/premiumAssets/2/add"); // Bing Maps Aerial
            FCesiumEditorModule::ion().pAsyncSystem->post("https://api.cesium.com/premiumAssets/3/add"); // Bing Maps Aerial with Labels
            FCesiumEditorModule::ion().pAsyncSystem->post("https://api.cesium.com/premiumAssets/4/add"); // Bing Maps Road
            FCesiumEditorModule::ion().pAsyncSystem->post("https://api.cesium.com/premiumAssets/3954/add"); // Sentinel-2
            FCesiumEditorModule::ion().pAsyncSystem->post("https://api.cesium.com/premiumAssets/96188/add"); // Cesium World Terrain

            std::string tokenName = wstr_to_utf8(FApp::GetProjectName()) + " (Created by Cesium for Unreal)";

            // TODO: rather than find a token by name, it would be better to store the token ID in the UE project somewhere.
            const std::vector<CesiumIonToken>& tokenList = tokens.value.value();
            const CesiumIonToken* pFound = nullptr;

            for (auto& token : tokenList) {
                if (token.name == tokenName) {
                    pFound = &token;
                }
            }

            if (pFound) {
                FCesiumEditorModule::ion().token = pFound->token;
                return FCesiumEditorModule::ion().pAsyncSystem->createResolvedFuture();
            } else {
                // TODO: grant access to individual assets.
                return FCesiumEditorModule::ion().connection.value().createToken(
                    tokenName,
                    { "assets:read" },
                    std::nullopt
                ).thenInMainThread([this](CesiumIonConnection::Response<CesiumIonToken>&& token) {
                    if (token.value) {
                        FCesiumEditorModule::ion().token = token.value.value().token;
                    }
                });
            }
        });
    }).catchInMainThread([this](const std::exception& /*e*/) {
        this->_signInInProgress = false;
    });

    return FReply::Handled();
}
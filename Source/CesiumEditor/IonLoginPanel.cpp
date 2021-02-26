#include "IonLoginPanel.h"
#include "CesiumEditor.h"
#include "Styling/SlateStyle.h"
#include "Misc/App.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "CesiumIonClient/Token.h"
#include "CesiumIonClient/Connection.h"
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
                    .OnClicked(this, &IonLoginPanel::SignIn)
                    .Text(FText::FromString(TEXT("Connect")))
                    .IsEnabled_Lambda([this]() { return !this->_signInInProgress; })
            ]
            + SVerticalBox::Slot()
                .VAlign(VAlign_Top)
                .HAlign(HAlign_Fill)
                .Padding(5, 15, 5, 5)
                .AutoHeight()
            [
                SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Waiting for you to sign into Cesium ion with your web browser...")))
                    .Visibility_Lambda([this]() { return this->_signInInProgress ? EVisibility::Visible : EVisibility::Hidden; })
                    .AutoWrapText(true)
            ]
            + SVerticalBox::Slot()
                .HAlign(HAlign_Center)
                .Padding(5)
            [
                SNew(SThrobber)
                    .Animate(SThrobber::Horizontal)
                    .Visibility_Lambda([this]() { return this->_signInInProgress ? EVisibility::Visible : EVisibility::Hidden; })
            ]
            + SVerticalBox::Slot()
                .HAlign(HAlign_Center)
                .Padding(5)
                .AutoHeight()
            [
                SNew(SHyperlink)
                    .OnNavigate(this, &IonLoginPanel::LaunchBrowserAgain)
                    .Text(FText::FromString(TEXT("Open web browser again")))
                    .Visibility_Lambda([this]() { return this->_signInInProgress ? EVisibility::Visible : EVisibility::Hidden; })
            ]
            + SVerticalBox::Slot()
                .VAlign(VAlign_Top)
                .HAlign(HAlign_Fill)
                .Padding(5)
                .AutoHeight()
            [
                SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Or copy the URL below into your web browser")))
                    .Visibility_Lambda([this]() { return this->_signInInProgress ? EVisibility::Visible : EVisibility::Hidden; })
                    .AutoWrapText(true)
            ]
            + SVerticalBox::Slot()
                .HAlign(HAlign_Center)
            [
                SNew(SBorder)
                    .Visibility_Lambda([this]() { return this->_signInInProgress ? EVisibility::Visible : EVisibility::Hidden; })
                [
                    SNew(SEditableText)
                        .IsReadOnly(true)
                        .Text_Lambda([this]() { return FText::FromString(this->_authorizeUrl); })
                ]
            ]
        ]
    ];
}

FReply IonLoginPanel::SignIn() {
    this->_signInInProgress = true;

    CesiumIonClient::Connection::authorize(
        *FCesiumEditorModule::ion().pAsyncSystem,
        FCesiumEditorModule::ion().pAssetAccessor,
        "Unreal Engine",
        190,
        "/cesium-for-unreal/oauth2/callback",
        {
            "assets:list",
            "assets:read",
            "profile:read",
            "tokens:read",
            "tokens:write",
            "geocode"
        },
        [this](const std::string& url) {
            this->_authorizeUrl = utf8_to_wstr(url);
            FPlatformProcess::LaunchURL(*this->_authorizeUrl, NULL, NULL);
        }
    ).thenInMainThread([this](CesiumIonClient::Connection&& connection) {
        FCesiumEditorModule::ion().connection = std::move(connection);
        FCesiumEditorModule::ion().profile = {};
        FCesiumEditorModule::ion().token = "";

        return FCesiumEditorModule::ion().connection.value().me().thenInMainThread([this](CesiumIonClient::Response<Profile>&& profile) {
            this->_signInInProgress = false;
            FCesiumEditorModule::ion().profile = std::move(profile.value);
            FCesiumEditorModule::ion().ionConnectionChanged.Broadcast();
            return FCesiumEditorModule::ion().connection.value().tokens();
        }).thenInMainThread([this](CesiumIonClient::Response<std::vector<CesiumIonClient::Token>>&& tokens) -> CesiumAsync::Future<void> {
            if (!tokens.value) {
                return FCesiumEditorModule::ion().pAsyncSystem->createResolvedFuture();
            }

            std::string tokenName = wstr_to_utf8(FApp::GetProjectName()) + " (Created by Cesium for Unreal)";

            // TODO: rather than find a token by name, it would be better to store the token ID in the UE project somewhere.
            const std::vector<Token>& tokenList = tokens.value.value();
            const Token* pFound = nullptr;

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
                ).thenInMainThread([this](CesiumIonClient::Response<Token>&& token) {
                    if (token.value) {
                        FCesiumEditorModule::ion().token = token.value.value().token;
                    }
                });
            }
        });
    }).catchInMainThread([this](const std::exception& /*e*/) {
        FCesiumEditorModule::ion().connection = std::nullopt;
        FCesiumEditorModule::ion().profile = {};
        FCesiumEditorModule::ion().token = "";
        this->_signInInProgress = false;
    });

    return FReply::Handled();
}

void IonLoginPanel::LaunchBrowserAgain() {
    FPlatformProcess::LaunchURL(*this->_authorizeUrl, NULL, NULL);
}
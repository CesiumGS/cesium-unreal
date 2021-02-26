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
#include "CesiumIonClient/CesiumIonToken.h"
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

        return FCesiumEditorModule::ion().connection.value().me().thenInMainThread([this](CesiumIonClient::Response<CesiumIonProfile>&& profile) {
            this->_signInInProgress = false;
            FCesiumEditorModule::ion().profile = std::move(profile.value);
            FCesiumEditorModule::ion().ionConnectionChanged.Broadcast();
            return FCesiumEditorModule::ion().connection.value().tokens();
        }).thenInMainThread([this](CesiumIonClient::Response<std::vector<CesiumIonClient::CesiumIonToken>>&& tokens) -> CesiumAsync::Future<void> {
            if (!tokens.value) {
                return FCesiumEditorModule::ion().pAsyncSystem->createResolvedFuture();
            }

            // Super-hackily add all the "premium assets" that we need for quick add to our assets.
            // TODO: less hackily maybe?
            FCesiumEditorModule::ion().pAssetAccessor->post(*FCesiumEditorModule::ion().pAsyncSystem, "https://api.cesium.com/premiumAssets/1/add"); // Cesium World Terrain
            FCesiumEditorModule::ion().pAssetAccessor->post(*FCesiumEditorModule::ion().pAsyncSystem, "https://api.cesium.com/premiumAssets/2/add"); // Bing Maps Aerial
            FCesiumEditorModule::ion().pAssetAccessor->post(*FCesiumEditorModule::ion().pAsyncSystem, "https://api.cesium.com/premiumAssets/3/add"); // Bing Maps Aerial with Labels
            FCesiumEditorModule::ion().pAssetAccessor->post(*FCesiumEditorModule::ion().pAsyncSystem, "https://api.cesium.com/premiumAssets/4/add"); // Bing Maps Road
            FCesiumEditorModule::ion().pAssetAccessor->post(*FCesiumEditorModule::ion().pAsyncSystem, "https://api.cesium.com/premiumAssets/3954/add"); // Sentinel-2
            FCesiumEditorModule::ion().pAssetAccessor->post(*FCesiumEditorModule::ion().pAsyncSystem, "https://api.cesium.com/premiumAssets/96188/add"); // Cesium World Terrain

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
                ).thenInMainThread([this](CesiumIonClient::Response<CesiumIonToken>&& token) {
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
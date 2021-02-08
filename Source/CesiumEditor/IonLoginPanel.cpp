#include "IonLoginPanel.h"
#include "CesiumEditor.h"
#include "Styling/SlateStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

void IonLoginPanel::Construct(const FArguments& InArgs)
{
    ChildSlot [
        SNew(SScrollBox)
        + SScrollBox::Slot()
            .VAlign(VAlign_Top)
            .HAlign(HAlign_Center)
            .Padding(10)
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
            ]
            + SVerticalBox::Slot()
                .VAlign(VAlign_Top)
                .HAlign(HAlign_Center)
                .Padding(5)
                .AutoHeight()
            [
                SNew(SButton)
                    .Text(FText::FromString(TEXT("Sign in")))
            ]
        ]
    ];
}

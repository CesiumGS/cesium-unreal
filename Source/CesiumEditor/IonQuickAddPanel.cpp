#include "IonQuickAddPanel.h"
#include "CesiumEditor.h"
#include "Styling/SlateStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "CesiumIonClient/CesiumIonConnection.h"
#include "UnrealConversions.h"
#include "ACesium3DTileset.h"
#include "Editor.h"
#include "CesiumIonRasterOverlay.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Layout/SHeader.h"

using namespace CesiumIonClient;

void IonQuickAddPanel::Construct(const FArguments& InArgs)
{
    ChildSlot [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
            .VAlign(VAlign_Top)
            .HAlign(HAlign_Fill)
            .AutoHeight()
            .Padding(FMargin(5.0f, 20.0f, 5.0f, 10.0f))
        [
            SNew(SHeader).Content()
            [
                SNew(STextBlock)
                    .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
                    .Text(FText::FromString(TEXT("Quick Add")))
            ]
        ]
        + SVerticalBox::Slot()
            .VAlign(VAlign_Top)
            .HAlign(HAlign_Fill)
            .Padding(FMargin(5.0f, 0.0f, 5.0f, 20.0f))
        [
            this->QuickAddList()
        ]
    ];
}

TSharedRef<SWidget> IonQuickAddPanel::QuickAddList() {
    static const TArray<TSharedRef<QuickAddItem>> quickAddItems = {
        MakeShared<QuickAddItem>(QuickAddItem {
            "Cesium World Terrain + Bing Maps Aerial imagery",
            1,
            2
        }),
        MakeShared<QuickAddItem>(QuickAddItem {
            "Cesium World Terrain + Bing Maps Aerial with Labels imagery",
            1,
            3
        }),
        MakeShared<QuickAddItem>(QuickAddItem {
            "Cesium World Terrain + Bing Maps Road imagery",
            1,
            4
        }),
        MakeShared<QuickAddItem>(QuickAddItem {
            "Cesium World Terrain + Sentinel-2 imagery",
            1,
            3954
        }),
        MakeShared<QuickAddItem>(QuickAddItem {
            "Cesium OSM Buildings",
            96188,
            -1
        })
    };

    return SNew(SListView<TSharedRef<QuickAddItem>>)
        .SelectionMode(ESelectionMode::None)
        .ListItemsSource(&quickAddItems)
        .OnMouseButtonDoubleClick(this, &IonQuickAddPanel::AddItemToLevel)
        .OnGenerateRow(this, &IonQuickAddPanel::CreateQuickAddItemRow);
}

TSharedRef<ITableRow> IonQuickAddPanel::CreateQuickAddItemRow(TSharedRef<QuickAddItem> item, const TSharedRef<STableViewBase>& list) {
    return SNew(STableRow<TSharedRef<QuickAddItem>>, list)
        .Content()
        [
            SNew(SBox)
                .HAlign(EHorizontalAlignment::HAlign_Fill)
                .HeightOverride(40.0f)
                .Content()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(5.0f)
                    .VAlign(EVerticalAlignment::VAlign_Center)
                [
                    SNew(STextBlock)
                        .AutoWrapText(true)
                        .Text(FText::FromString(utf8_to_wstr(item->name)))
                ]
                + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(EVerticalAlignment::VAlign_Center)
                [
                    PropertyCustomizationHelpers::MakeNewBlueprintButton(
                        FSimpleDelegate::CreateLambda(
                            [this, item]() { this->AddItemToLevel(item); }
                        ),
                        FText::FromString(TEXT("Add this dataset to the level"))
                    )
                ]
            ]
        ];
}

void IonQuickAddPanel::AddItemToLevel(TSharedRef<QuickAddItem> item) {
    UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
    ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

    AActor* pNewActor = GEditor->AddActor(pCurrentLevel, ACesium3DTileset::StaticClass(), FTransform(), false, RF_Public | RF_Transactional);
    ACesium3DTileset* pTileset = Cast<ACesium3DTileset>(pNewActor);
    pTileset->SetActorLabel(utf8_to_wstr(item->name));
    pTileset->IonAssetID = item->tilesetID;
    
    // TODO: use a configured token rather than the connection's token
    pTileset->IonAccessToken = utf8_to_wstr(FCesiumEditorModule::ion().token);

    if (item->overlayID > 0) {
        UCesiumIonRasterOverlay* pOverlay = NewObject<UCesiumIonRasterOverlay>(pTileset);
        pOverlay->IonAssetID = item->overlayID;
        pOverlay->IonAccessToken = pTileset->IonAccessToken;
        pOverlay->SetActive(true);
        pTileset->AddInstanceComponent(pOverlay);
    }

    pTileset->RerunConstructionScripts();
}
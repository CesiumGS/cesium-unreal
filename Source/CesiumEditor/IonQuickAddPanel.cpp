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

using namespace CesiumIonClient;

void IonQuickAddPanel::Construct(const FArguments& InArgs)
{
    ChildSlot [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
            .VAlign(VAlign_Top)
            .HAlign(HAlign_Left)
            .AutoHeight()
            .Padding(FMargin(5.0f, 20.0f))
        [
            SNew(STextBlock)
                .Text(FText::FromString(TEXT("Quick Add:")))
        ]
        + SVerticalBox::Slot()
            .VAlign(VAlign_Top)
            .HAlign(HAlign_Left)
            .Padding(FMargin(5.0f, 20.0f))
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
        .ListItemsSource(&quickAddItems)
        .OnMouseButtonDoubleClick(this, &IonQuickAddPanel::AddItemToLevel)
        .OnGenerateRow(this, &IonQuickAddPanel::CreateQuickAddItemRow);
}

TSharedRef<ITableRow> IonQuickAddPanel::CreateQuickAddItemRow(TSharedRef<QuickAddItem> item, const TSharedRef<STableViewBase>& list) {
    return SNew(STableRow<TSharedRef<QuickAddItem>>, list)
        .Content()
        [
            SNew(STextBlock)
                .AutoWrapText(true)
                .Text(FText::FromString(utf8_to_wstr(item->name)))
        ];
}

void IonQuickAddPanel::AddItemToLevel(TSharedRef<QuickAddItem> item) {
    UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
    ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

    AActor* pNewActor = GEditor->AddActor(pCurrentLevel, ACesium3DTileset::StaticClass(), FTransform(), false, RF_Public | RF_Standalone | RF_Transactional);
    ACesium3DTileset* pTileset = Cast<ACesium3DTileset>(pNewActor);
    pTileset->SetActorLabel(utf8_to_wstr(item->name));
    pTileset->IonAssetID = item->tilesetID;
    
    // TODO: use a configured token rather than the connection's token
    pTileset->IonAccessToken = utf8_to_wstr(FCesiumEditorModule::ion().connection.value().getAccessToken());

    if (item->overlayID > 0) {
        UCesiumIonRasterOverlay* pOverlay = NewObject<UCesiumIonRasterOverlay>(pTileset);
        pOverlay->IonAssetID = item->overlayID;
        pOverlay->IonAccessToken = pTileset->IonAccessToken;
        pOverlay->SetActive(true);
    }

    pTileset->RerunConstructionScripts();
}
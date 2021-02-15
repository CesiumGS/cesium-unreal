#include "CesiumIonPanel.h"
#include "IonLoginPanel.h"
#include "CesiumEditor.h"
#include "CesiumCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SButton.h"
#include "Editor.h"
#include "ACesium3DTileset.h"
#include "UnrealConversions.h"
#include "IonQuickAddPanel.h"

using namespace CesiumIonClient;

void CesiumIonPanel::Construct(const FArguments& InArgs)
{
    this->_pListView = SNew(SListView<TSharedPtr<CesiumIonAsset>>)
        .ListItemsSource(&this->_assets)
        .OnMouseButtonDoubleClick(this, &CesiumIonPanel::AddAssetToLevel)
        .OnGenerateRow(this, &CesiumIonPanel::CreateAssetRow)
        .OnSelectionChanged(this, &CesiumIonPanel::AssetSelected)
        .HeaderRow(
            SNew(SHeaderRow)
            + SHeaderRow::Column("Name").DefaultLabel(FText::FromString(TEXT("Name")))
            + SHeaderRow::Column("Type").DefaultLabel(FText::FromString(TEXT("Type")))
            + SHeaderRow::Column("DateAdded").DefaultLabel(FText::FromString(TEXT("Date added")))
            + SHeaderRow::Column("Size").DefaultLabel(FText::FromString(TEXT("Size")))
        );

    this->_pDetails = this->AssetDetails();

    ChildSlot
    [
        SNew(SSplitter)
            .Orientation(EOrientation::Orient_Horizontal)
        + SSplitter::Slot()
            .Value(0.66f)
        [
            this->_pListView.ToSharedRef()
        ]
        + SSplitter::Slot()
            .Value(0.34f)
        [
            SNew(SBorder).Padding(10)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                [
                    this->_pDetails.ToSharedRef()
                ]
                + SVerticalBox::Slot()
                [
                    SNew(STextBlock).Visibility_Lambda([this]() { return this->_pSelection ? EVisibility::Collapsed : EVisibility::Visible; })
                ]
            ]
        ]
    ];

    this->Refresh();
}

static bool isSupported(const TSharedPtr<CesiumIonAsset>& pAsset) {
    return
        pAsset &&
        (
            pAsset->type == "3DTILES" ||
            pAsset->type == "IMAGERY" ||
            pAsset->type == "TERRAIN"
        );
}

TSharedRef<SWidget> CesiumIonPanel::AssetDetails()
{
    return SNew(SScrollBox)
        .Visibility_Lambda([this]() { return this->_pSelection ? EVisibility::Visible : EVisibility::Collapsed; })
        + SScrollBox::Slot()
        .Padding(10, 10, 10, 0)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .TextStyle(FCesiumEditorModule::GetStyle(), "Heading")
            .Text_Lambda([this]() { return FText::FromString(utf8_to_wstr(this->_pSelection->name)); })
        ]
        + SScrollBox::Slot()
        .Padding(10, 5, 10, 10)
        .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(STextBlock)
            .Text_Lambda([this]() { return FText::FromString(utf8_to_wstr(std::string("(ID: ") + std::to_string(this->_pSelection->id) + ")")); })
        ]
        + SScrollBox::Slot()
        .Padding(10)
        .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(SButton)
            .Visibility_Lambda([this]() { return isSupported(this->_pSelection) ? EVisibility::Visible : EVisibility::Collapsed; })
            .HAlign(EHorizontalAlignment::HAlign_Center)
            .Text(FText::FromString(TEXT("Add to Level")))
            .OnClicked_Lambda([this]() { this->AddAssetToLevel(this->_pSelection); return FReply::Handled(); })
        ]
        + SScrollBox::Slot()
        .Padding(10)
        .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(SButton)
            .Visibility_Lambda([this]() { return isSupported(this->_pSelection) ? EVisibility::Collapsed : EVisibility::Visible; })
            .HAlign(EHorizontalAlignment::HAlign_Center)
            .Text(FText::FromString(TEXT("This type of dataset is not currently supported")))
            .IsEnabled(false)
        ]
        + SScrollBox::Slot()
        .Padding(10)
        .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(STextBlock)
            .TextStyle(FCesiumEditorModule::GetStyle(), "AssetDetailsFieldHeader")
            .Text(FText::FromString(TEXT("Description")))
        ]
        + SScrollBox::Slot()
        .Padding(10, 0)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .TextStyle(FCesiumEditorModule::GetStyle(), "AssetDetailsFieldValue")
            .Text_Lambda([this]() { return FText::FromString(utf8_to_wstr(this->_pSelection->description)); })
        ]
        + SScrollBox::Slot()
        .Padding(10)
        .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(STextBlock)
            .TextStyle(FCesiumEditorModule::GetStyle(), "AssetDetailsFieldHeader")
            .Text(FText::FromString(TEXT("Attribution")))
        ]
        + SScrollBox::Slot()
        .Padding(10, 0)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .TextStyle(FCesiumEditorModule::GetStyle(), "AssetDetailsFieldValue")
            .Text_Lambda([this]() { return FText::FromString(utf8_to_wstr(this->_pSelection->attribution)); })
        ];
}

void CesiumIonPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) {
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    if (this->_refreshNeeded && !this->_refreshInProgress) {
        this->_refreshNeeded = false;
        this->Refresh();
    }
}

void CesiumIonPanel::Refresh() {
    if (this->_refreshInProgress) {
        this->_refreshNeeded = true;
        return;
    }

    auto& maybeConnection = FCesiumEditorModule::ion().connection;
    if (!maybeConnection) {
        // Can't refresh while disconnected.
        return;
    }

    auto& connection = maybeConnection.value();
    this->_refreshInProgress = true;

    connection.assets().thenInMainThread([this](CesiumIonConnection::Response<CesiumIonAssets>&& response) {
        if (response.value) {
            std::vector<CesiumIonAsset>& items = response.value.value().items;
            this->_assets.SetNum(items.size());
            for (size_t i = 0; i < items.size(); ++i) {
                TSharedPtr<CesiumIonAsset>& pAsset = this->_assets[i];
                if (pAsset) {
                    *pAsset = items[i];
                } else {
                    pAsset = MakeShared<CesiumIonAsset>(std::move(items[i]));
                }
            }
        }
        this->_refreshInProgress = false;
        this->_pListView->RequestListRefresh();
    }).catchInMainThread([this](const std::exception& e) {
        UE_LOG(LogActor, Error, TEXT("Error getting list of assets from Cesium ion: %s"), e.what());
        this->_refreshInProgress = false;
    });
}

void CesiumIonPanel::AssetSelected(TSharedPtr<CesiumIonClient::CesiumIonAsset> item, ESelectInfo::Type selectionType)
{
    this->_pSelection = item;
}

void CesiumIonPanel::AddAssetToLevel(TSharedPtr<CesiumIonClient::CesiumIonAsset> item)
{
    UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
    ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

    AActor* pNewActor = GEditor->AddActor(pCurrentLevel, ACesium3DTileset::StaticClass(), FTransform(), false, RF_Public | RF_Transactional);
    ACesium3DTileset* pTileset = Cast<ACesium3DTileset>(pNewActor);
    pTileset->SetActorLabel(utf8_to_wstr(item->name));
    pTileset->IonAssetID = item->id;

    // TODO: use a configured token rather than the connection's token
    pTileset->IonAccessToken = utf8_to_wstr(FCesiumEditorModule::ion().connection.value().getAccessToken());

    pTileset->RerunConstructionScripts();
}

namespace {

    class AssetsTableRow : public SMultiColumnTableRow<TSharedPtr<CesiumIonAsset>> {
    public:
        void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, const TSharedPtr<CesiumIonAsset>& pItem) {
            this->_pItem = pItem;
            SMultiColumnTableRow<TSharedPtr<CesiumIonAsset>>::Construct(InArgs, InOwnerTableView);
        }

        virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override {
            if (InColumnName == "Name") {
                return SNew(STextBlock)
                    .Text(FText::FromString(utf8_to_wstr(_pItem->name)));
            } else if (InColumnName == "Type") {
                return SNew(STextBlock)
                    .Text(FText::FromString(utf8_to_wstr(_pItem->type)));
            } else if (InColumnName == "DateAdded") {
                return SNew(STextBlock)
                    .Text(FText::FromString(utf8_to_wstr(_pItem->dateAdded)));
            } else if (InColumnName == "Size") {
                return SNew(STextBlock)
                    .Text(FText::FromString(utf8_to_wstr(_pItem->bytes > 0 ? std::to_string(_pItem->bytes) : "-")));
            } else {
                return SNew(STextBlock);
            }
        }

    private:
        TSharedPtr<CesiumIonAsset> _pItem;
    };

}

TSharedRef<ITableRow> CesiumIonPanel::CreateAssetRow(TSharedPtr<CesiumIonAsset> item, const TSharedRef<STableViewBase>& list) {
    return SNew(AssetsTableRow, list, item);
}
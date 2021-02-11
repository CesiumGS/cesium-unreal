#include "CesiumIonPanel.h"
#include "IonLoginPanel.h"
#include "CesiumEditor.h"
#include "CesiumCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Views/SListView.h"
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
        .HeaderRow(
            SNew(SHeaderRow)
            + SHeaderRow::Column("Name").DefaultLabel(FText::FromString(TEXT("Name")))
            + SHeaderRow::Column("Type").DefaultLabel(FText::FromString(TEXT("Type")))
            + SHeaderRow::Column("DateAdded").DefaultLabel(FText::FromString(TEXT("Date added")))
            + SHeaderRow::Column("Size").DefaultLabel(FText::FromString(TEXT("Size")))
        );

    ChildSlot
    [
        this->_pListView.ToSharedRef()
    ];

    this->Refresh();
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

void CesiumIonPanel::AddAssetToLevel(TSharedPtr<CesiumIonClient::CesiumIonAsset> item)
{
    UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
    ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

    AActor* pNewActor = GEditor->AddActor(pCurrentLevel, ACesium3DTileset::StaticClass(), FTransform(), false, RF_Public | RF_Standalone | RF_Transactional);
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
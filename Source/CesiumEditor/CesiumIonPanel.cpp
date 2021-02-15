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
#include "EditorModeManager.h"
#include "EngineUtils.h"
#include "ACesium3DTileset.h"
#include "UnrealConversions.h"
#include "IonQuickAddPanel.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "CesiumIonRasterOverlay.h"

using namespace CesiumIonClient;

CesiumIonPanel::CesiumIonPanel() :
    _connectionChangedDelegateHandle(),
    _pListView(nullptr),
    _assets(),
    _refreshInProgress(false),
    _refreshNeeded(false),
    _pDetails(nullptr),
    _pSelection(nullptr)
{
    this->_connectionChangedDelegateHandle = FCesiumEditorModule::ion().ionConnectionChanged.AddRaw(this, &CesiumIonPanel::Refresh);
}

CesiumIonPanel::~CesiumIonPanel() {
    FCesiumEditorModule::ion().ionConnectionChanged.Remove(this->_connectionChangedDelegateHandle);
}

void CesiumIonPanel::Construct(const FArguments& InArgs)
{
    this->_pListView = SNew(SListView<TSharedPtr<CesiumIonAsset>>)
        .ListItemsSource(&this->_assets)
        .OnMouseButtonDoubleClick(this, &CesiumIonPanel::AddAsset)
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

static bool isSupportedTileset(const TSharedPtr<CesiumIonAsset>& pAsset) {
    return
        pAsset &&
        (
            pAsset->type == "3DTILES" ||
            pAsset->type == "TERRAIN"
        );
}

static bool isSupportedImagery(const TSharedPtr<CesiumIonAsset>& pAsset) {
    return
        pAsset &&
        pAsset->type == "IMAGERY";
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
            .Visibility_Lambda([this]() { return isSupportedTileset(this->_pSelection) ? EVisibility::Visible : EVisibility::Collapsed; })
            .HAlign(EHorizontalAlignment::HAlign_Center)
            .Text(FText::FromString(TEXT("Add to Level")))
            .OnClicked_Lambda([this]() { this->AddAssetToLevel(this->_pSelection); return FReply::Handled(); })
        ]
        + SScrollBox::Slot()
            .Padding(10)
            .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(SButton)
            .Visibility_Lambda([this]() { return isSupportedImagery(this->_pSelection) ? EVisibility::Visible : EVisibility::Collapsed; })
            .HAlign(EHorizontalAlignment::HAlign_Center)
            .Text(FText::FromString(TEXT("Drape Over Terrain Tileset")))
            .OnClicked_Lambda([this]() { this->AddOverlayToTerrain(this->_pSelection); return FReply::Handled(); })
        ]
        + SScrollBox::Slot()
        .Padding(10)
        .HAlign(EHorizontalAlignment::HAlign_Fill)
        [
            SNew(SButton)
            .Visibility_Lambda([this]() { return !isSupportedTileset(this->_pSelection) && !isSupportedImagery(this->_pSelection) ? EVisibility::Visible : EVisibility::Collapsed; })
            .HAlign(EHorizontalAlignment::HAlign_Center)
            .Text(FText::FromString(TEXT("This type of asset is not currently supported")))
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
        // Can't refresh while disconnected, but we can clear out previous results.
        this->_assets.Empty();
        this->_pListView->RequestListRefresh();
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

void CesiumIonPanel::AddAsset(TSharedPtr<CesiumIonClient::CesiumIonAsset> item) {
    if (item->type == "IMAGERY") {
        this->AddOverlayToTerrain(item);
    } else {
        this->AddAssetToLevel(item);
    }
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
    pTileset->IonAccessToken = utf8_to_wstr(FCesiumEditorModule::ion().token);

    pTileset->RerunConstructionScripts();
}

void CesiumIonPanel::AddOverlayToTerrain(TSharedPtr<CesiumIonClient::CesiumIonAsset> item) {
    UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
    ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

    bool found = false;

    for (TActorIterator<ACesium3DTileset> it(pCurrentWorld); !found && it; ++it) {
        const Cesium3DTiles::Tileset* pTileset = it->GetTileset();
        if (!pTileset) {
            continue;
        }

        if (!pTileset->supportsRasterOverlays()) {
            continue;
        }

        // Looks like something we can hang an overlay on!
        // Remove any existing overlays and add the new one.
        // TODO: ideally we wouldn't remove the old overlays but the number of overlay textures we can support
        // is currently very limited.
        TArray<UCesiumRasterOverlay*> rasterOverlays;
        it->GetComponents<UCesiumRasterOverlay>(rasterOverlays);

        for (UCesiumRasterOverlay* pOverlay : rasterOverlays) {
            pOverlay->DestroyComponent(false);
        }

        UCesiumIonRasterOverlay* pOverlay = NewObject<UCesiumIonRasterOverlay>(*it, FName(utf8_to_wstr(item->name)), RF_Public | RF_Transactional);

        pOverlay->IonAssetID = item->id;
        pOverlay->IonAccessToken = utf8_to_wstr(FCesiumEditorModule::ion().token);
        pOverlay->SetActive(true);
        
        it->AddInstanceComponent(pOverlay);
        pOverlay->OnComponentCreated();

        GEditor->SelectNone(true, false);
        GEditor->SelectActor(*it, true, true, true, true);
        GEditor->SelectComponent(pOverlay, true, true, true);

        found = true;
    }

    if (!found) {
        // Failed to find a tileset to attach to, report an error.
        // TODO: might be better to automatically add Cesium World Terain instead.
        UE_LOG(LogActor, Error, TEXT("The level does not contain a Terrain tileset. Please add one before attempting to add imagery."));
    }
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
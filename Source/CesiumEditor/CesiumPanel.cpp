#include "CesiumPanel.h"
#include "IonLoginPanel.h"
#include "CesiumEditor.h"
#include "CesiumCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateStyleRegistry.h"
#include "Editor.h"
#include "ACesium3DTileset.h"

TSharedPtr<FSlateStyleSet> CesiumPanel::Style = nullptr;

void CesiumPanel::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight() [ Toolbar() ]
        + SVerticalBox::Slot() [ LoginPanel() ]
    ];
}

void CesiumPanel::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) {
    FCesiumEditorModule* pModule = FCesiumEditorModule::get();
    if (pModule) {
        CesiumAsync::AsyncSystem* pAsync = pModule->getAsyncSystem();
        if (pAsync) {
            pAsync->dispatchMainThreadTasks();
        }
    }

    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

static bool isSignedIn() {
    return FCesiumEditorModule::get()->getIonConnection().has_value();
}

TSharedRef<SWidget> CesiumPanel::Toolbar() {
    TSharedRef<FUICommandList> commandList = MakeShared<FUICommandList>();

    commandList->MapAction(FCesiumCommands::Get().AddFromIon, FExecuteAction::CreateSP(this, &CesiumPanel::signOut), FCanExecuteAction::CreateStatic(isSignedIn));
    commandList->MapAction(FCesiumCommands::Get().UploadToIon, FExecuteAction::CreateSP(this, &CesiumPanel::signOut), FCanExecuteAction::CreateStatic(isSignedIn));
    commandList->MapAction(FCesiumCommands::Get().AddBlankTileset, FExecuteAction::CreateSP(this, &CesiumPanel::addBlankTileset));
    commandList->MapAction(FCesiumCommands::Get().AccessToken, FExecuteAction::CreateSP(this, &CesiumPanel::signOut), FCanExecuteAction::CreateStatic(isSignedIn));
    commandList->MapAction(FCesiumCommands::Get().SignOut, FExecuteAction::CreateSP(this, &CesiumPanel::signOut), FCanExecuteAction::CreateStatic(isSignedIn));
    
    FToolBarBuilder builder(commandList, FMultiBoxCustomization::None);

    builder.AddToolBarButton(FCesiumCommands::Get().AddFromIon);
    builder.AddToolBarButton(FCesiumCommands::Get().UploadToIon);
    builder.AddToolBarButton(FCesiumCommands::Get().AddBlankTileset);
    builder.AddToolBarButton(FCesiumCommands::Get().AccessToken);
    builder.AddToolBarButton(FCesiumCommands::Get().SignOut);

    return builder.MakeWidget();
}

TSharedRef<SWidget> CesiumPanel::LoginPanel() {
    return SNew(IonLoginPanel)
        .Visibility_Lambda([]() { return isSignedIn() ? EVisibility::Collapsed : EVisibility::Visible; });
}

void CesiumPanel::addBlankTileset() {
    UWorld* pCurrentWorld = GEditor->GetEditorWorldContext().World();
    ULevel* pCurrentLevel = pCurrentWorld->GetCurrentLevel();

    GEditor->AddActor(pCurrentLevel, ACesium3DTileset::StaticClass(), FTransform(), false, RF_Public | RF_Standalone | RF_Transactional);
}

void CesiumPanel::signOut() {
    FCesiumEditorModule::get()->setIonConnection(std::nullopt);
}

void CesiumPanel::RegisterStyle()
{
    if (Style.IsValid()) {
        return;
    }

    Style = MakeShareable(new FSlateStyleSet("CesiumPanelStyleSet"));

    Style->Set("WelcomeText", FTextBlockStyle()
        .SetColorAndOpacity(FSlateColor::UseForeground())
        .SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 14))
    );

    FSlateStyleRegistry::RegisterSlateStyle(*Style.Get());
}

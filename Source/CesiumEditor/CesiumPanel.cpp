#include "CesiumPanel.h"
#include "IonLoginPanel.h"
#include "CesiumCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateStyleRegistry.h"

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

TSharedRef<SWidget> CesiumPanel::Toolbar() {
    TSharedRef<FUICommandList> commandList = MakeShared<FUICommandList>();
    
    FToolBarBuilder builder(commandList, FMultiBoxCustomization::None);

    builder.AddToolBarButton(FCesiumCommands::Get().AddFromIon);
    builder.AddToolBarButton(FCesiumCommands::Get().UploadToIon);
    builder.AddToolBarButton(FCesiumCommands::Get().AddBlankTileset);
    builder.AddToolBarButton(FCesiumCommands::Get().AccessToken);
    builder.AddToolBarButton(FCesiumCommands::Get().SignOut);

    return builder.MakeWidget();
}

TSharedRef<SWidget> CesiumPanel::LoginPanel() {
    return SNew(IonLoginPanel);
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

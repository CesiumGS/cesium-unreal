#include "CesiumIonPanel.h"
#include "IonLoginPanel.h"
#include "CesiumEditor.h"
#include "CesiumCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Layout/SHeader.h"
#include "Editor.h"
#include "ACesium3DTileset.h"
#include "UnrealConversions.h"
#include "IonQuickAddPanel.h"

void CesiumIonPanel::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(STextBlock)
            .Text(FText::FromString(TEXT("Test")))
    ];
}

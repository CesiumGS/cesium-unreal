// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeoreferenceModeWidget.h"
#include "CesiumGeoreferenceMode.h" 
#include "CesiumEditor.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"

void SCesiumGeoreferenceModeWidget::Construct(const FArguments& InArgs) {
    
    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        .VAlign(VAlign_Top)
        .Padding(5.f)
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("This is a editor mode example.")))
        ]
    ];
}

FCesiumGeoreferenceMode* SCesiumGeoreferenceModeWidget::GetEdMode() const {
    return (FCesiumGeoreferenceMode*)GLevelEditorModeTools()
        .GetActiveMode(FCesiumGeoreferenceMode::EM_CesiumGeoreferenceMode);
}
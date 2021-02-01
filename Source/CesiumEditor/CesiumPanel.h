#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;

class CesiumPanel : public SCompoundWidget {
    SLATE_BEGIN_ARGS(CesiumPanel)
    {}
    //SLATE_ARGUMENT(TWeakPtr<class TabTool>, Tool)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
};

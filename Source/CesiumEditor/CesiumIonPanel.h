#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;

class CesiumIonPanel : public SCompoundWidget {
    SLATE_BEGIN_ARGS(CesiumIonPanel)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
};

#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;

class IonLoginPanel : public SCompoundWidget {
    SLATE_BEGIN_ARGS(IonLoginPanel)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    void LaunchBrowserAgain();

    FReply SignIn();
};

#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;

class IonLoginPanel : public SCompoundWidget {
    SLATE_BEGIN_ARGS(IonLoginPanel)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    void setUsername(const FText& value, ETextCommit::Type commitInfo);
    void setPassword(const FText& value, ETextCommit::Type commitInfo);

    bool _signInInProgress = false;
    FText _username;
    FText _password;

    FReply SignIn();
};

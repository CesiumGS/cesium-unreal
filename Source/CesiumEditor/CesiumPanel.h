// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Widgets/SCompoundWidget.h"

class FArguments;

class CesiumPanel : public SCompoundWidget {
    SLATE_BEGIN_ARGS(CesiumPanel)
    {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    TSharedRef<SWidget> Toolbar();
    TSharedRef<SWidget> LoginPanel();
    TSharedRef<SWidget> MainPanel();
    TSharedRef<SWidget> ConnectionStatus();

    void addFromIon();
    void uploadToIon();
    void addBlankTileset();
    void accessToken();
    void signOut();
};

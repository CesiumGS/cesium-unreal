// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Framework/Application/SlateApplication.h"

class SCesiumGeoreferenceModeWidget : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(SCesiumGeoreferenceModeWidget) {}
  SLATE_END_ARGS();

  void Construct(const FArguments& InArgs);

  class FCesiumGeoreferenceMode* GetEdMode() const;
};
// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumDmsEditor.h"
#include "IDetailCustomization.h"

class FCesiumGeoreferenceCustomization : public IDetailCustomization {
public:
  virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

  static TSharedRef<IDetailCustomization> MakeInstance();

private:
  TSharedPtr<CesiumDmsEditor> LongitudeEditor;
  TSharedPtr<CesiumDmsEditor> LatitudeEditor;
};

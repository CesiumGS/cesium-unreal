// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumDegreesMinutesSecondsEditor.h"
#include "IDetailCustomization.h"
#include "CesiumGlobeAnchorCustomization.generated.h"

class IDetailCategoryBuilder;

UCLASS()
class UCesiumGlobeAnchorRotationEastSouthUp : public UObject {
  GENERATED_BODY()

public:
  UPROPERTY()
  class UCesiumGlobeAnchorComponent* Component;

  UPROPERTY(EditAnywhere)
  double Pitch = 1.0;

  UPROPERTY(EditAnywhere)
  double Yaw = 2.0;

  UPROPERTY(EditAnywhere)
  double Roll = 3.0;

  virtual void PostEditChangeProperty(
      struct FPropertyChangedEvent& PropertyChangedEvent) override;
};

/**
 * An implementation of the IDetailCustomization interface that customizes
 * the Details View of a UCesiumGlobeAnchorComponent. It is registered in
 * FCesiumEditorModule::StartupModule.
 */
class FCesiumGlobeAnchorCustomization : public IDetailCustomization {
public:
  virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

  static void Register(FPropertyEditorModule& PropertyEditorModule);
  static void Unregister(FPropertyEditorModule& PropertyEditorModule);

  static TSharedRef<IDetailCustomization> MakeInstance();

private:
  void CreateRotationEastSouthUp(
      IDetailLayoutBuilder& DetailBuilder,
      IDetailCategoryBuilder& Category);
  void UpdateEastSouthUpValues();
  FReply
  OnPlaceGeoreferenceOriginHere(TWeakObjectPtr<UFunction> WeakFunctionPtr);

  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LongitudeEditor;
  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LatitudeEditor;
  TArray<TWeakObjectPtr<UObject>> SelectedObjects;
  TArray<TObjectPtr<UCesiumGlobeAnchorRotationEastSouthUp>> EastSouthUpObjects;
  TArray<UObject*> EastSouthUpPointers;
};

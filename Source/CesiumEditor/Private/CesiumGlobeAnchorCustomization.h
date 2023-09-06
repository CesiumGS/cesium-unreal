// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumDegreesMinutesSecondsEditor.h"
#include "IDetailCustomization.h"
#include "TickableEditorObject.h"
#include "CesiumGlobeAnchorCustomization.generated.h"

class IDetailCategoryBuilder;

UCLASS()
class UCesiumGlobeAnchorRotationEastSouthUp : public UObject,
                                              public FTickableEditorObject {
  GENERATED_BODY()

public:
  UPROPERTY()
  class UCesiumGlobeAnchorComponent* GlobeAnchor;

  /**
   * The rotation around the right (Y) axis. Zero pitch means the look direction
   * (+X) is level with the horizon. Positive pitch is looking up, negative
   * pitch is looking down.
   */
  UPROPERTY(EditAnywhere, Meta = (ClampMin = -89.9999, ClampMax = 89.9999))
  double Pitch = 0.0;

  /**
   * The rotation around the up (Z) axis. Zero yaw means the look direction (+X)
   * points East. Positive yaw rotates right toward South, while negative yaw
   * rotates left toward North.
   */
  UPROPERTY(EditAnywhere)
  double Yaw = 0.0;

  /**
   * The rotation around the forward (X) axis. Zero roll is upright. Positive
   * roll is like tilting your head to the right (clockwise), while negative
   * roll is tilting to the left (counter-clockwise).
   */
  UPROPERTY(EditAnywhere)
  double Roll = 0.0;

  virtual void PostEditChangeProperty(
      struct FPropertyChangedEvent& PropertyChangedEvent) override;

  void Initialize(UCesiumGlobeAnchorComponent* GlobeAnchor);

  // Inherited via FTickableEditorObject
  void Tick(float DeltaTime) override;
  TStatId GetStatId() const override;
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

  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LongitudeEditor;
  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LatitudeEditor;
  TArray<TWeakObjectPtr<UObject>> SelectedObjects;
  TArray<TObjectPtr<UCesiumGlobeAnchorRotationEastSouthUp>> EastSouthUpObjects;
  TArray<UObject*> EastSouthUpPointers;
};

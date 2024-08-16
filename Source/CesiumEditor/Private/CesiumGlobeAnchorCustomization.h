// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumDegreesMinutesSecondsEditor.h"
#include "IDetailCustomization.h"
#include "TickableEditorObject.h"
#include "CesiumGlobeAnchorCustomization.generated.h"

class IDetailCategoryBuilder;
class UCesiumGlobeAnchorDerivedProperties;

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
  void CreatePositionEarthCenteredEarthFixed(
      IDetailLayoutBuilder& DetailBuilder,
      IDetailCategoryBuilder& Category);
  void CreatePositionLongitudeLatitudeHeight(
      IDetailLayoutBuilder& DetailBuilder,
      IDetailCategoryBuilder& Category);
  void CreateRotationEastSouthUp(
      IDetailLayoutBuilder& DetailBuilder,
      IDetailCategoryBuilder& Category);
  void UpdateDerivedProperties();

  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LongitudeEditor;
  TSharedPtr<CesiumDegreesMinutesSecondsEditor> LatitudeEditor;
  TArray<TWeakObjectPtr<UObject>> SelectedObjects;
  TArray<TObjectPtr<UCesiumGlobeAnchorDerivedProperties>> DerivedObjects;
  TArray<UObject*> DerivedPointers;

  static FName RegisteredLayoutName;
};

UCLASS()
class UCesiumGlobeAnchorDerivedProperties : public UObject,
                                            public FTickableEditorObject {
  GENERATED_BODY()

public:
  UPROPERTY()
  class UCesiumGlobeAnchorComponent* GlobeAnchor;

  /**
   * The Earth-Centered Earth-Fixed (ECEF) X-coordinate of this component in
   * meters. The ECEF coordinate system's origin is at the center of the Earth
   * and +X points to the intersection of the Equator (zero degrees latitude)
   * and Prime Meridian (zero degrees longitude).
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double X = 0.0;

  /**
   * The Earth-Centered Earth-Fixed (ECEF) Y-coordinate of this component in
   * meters. The ECEF coordinate system's origin is at the center of the Earth
   * and +Y points to the intersection of the Equator (zero degrees latitude)
   * and +90 degrees longitude.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double Y = 0.0;

  /**
   * The Earth-Centered Earth-Fixed (ECEF) Z-coordinate of this component in
   * meters. The ECEF coordinate system's origin is at the center of the Earth
   * and +Z points up through the North Pole.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double Z = 0.0;

  /**
   * The latitude in degrees.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta = (ClampMin = -90.0, ClampMax = 90.0))
  double Latitude = 0.0;

  /**
   * The longitude in degrees.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta = (ClampMin = -180.0, ClampMax = 180.0))
  double Longitude = 0.0;

  /**
   * The height in meters above the ellipsoid.
   *
   * Do not confuse the ellipsoid height with a geoid height or height above
   * mean sea level, which can be tens of meters higher or lower depending on
   * where in the world the object is located.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double Height = 0.0;

  /**
   * The rotation around the right (Y) axis. Zero pitch means the look direction
   * (+X) is level with the horizon. Positive pitch is looking up, negative
   * pitch is looking down.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta = (ClampMin = -89.9999, ClampMax = 89.9999))
  double Pitch = 0.0;

  /**
   * The rotation around the up (Z) axis. Zero yaw means the look direction (+X)
   * points East. Positive yaw rotates right toward South, while negative yaw
   * rotates left toward North.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double Yaw = 0.0;

  /**
   * The rotation around the forward (X) axis. Zero roll is upright. Positive
   * roll is like tilting your head to the right (clockwise), while negative
   * roll is tilting to the left (counter-clockwise).
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  double Roll = 0.0;

  virtual void PostEditChangeProperty(
      struct FPropertyChangedEvent& PropertyChangedEvent) override;
  virtual bool CanEditChange(const FProperty* InProperty) const override;

  void Initialize(UCesiumGlobeAnchorComponent* GlobeAnchor);

  // Inherited via FTickableEditorObject
  void Tick(float DeltaTime) override;
  TStatId GetStatId() const override;
};

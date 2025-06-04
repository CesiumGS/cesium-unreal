#pragma once

#include "CesiumGeoJsonObject.h"

#include "CesiumGeoJsonObjectIterator.generated.h"

USTRUCT(BlueprintType)
struct FCesiumGeoJsonObjectIterator {
  GENERATED_BODY()

  FCesiumGeoJsonObjectIterator() : _object(), _iterator() {}

  FCesiumGeoJsonObjectIterator(const FCesiumGeoJsonObject& object)
      : _object(object), _iterator(*_object.getObject()) {}

private:
  FCesiumGeoJsonObject _object;
  CesiumVectorData::ConstGeoJsonObjectIterator _iterator;

  friend class UCesiumGeoJsonObjectIteratorFunctionLibrary;
};

UCLASS()
class UCesiumGeoJsonObjectIteratorFunctionLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Object")
  static FCesiumGeoJsonObjectIterator
  Iterate(const FCesiumGeoJsonObject& Object);

  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Iterator")
  static FCesiumGeoJsonObject Next(UPARAM(Ref)
                                       FCesiumGeoJsonObjectIterator& Iterator);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Iterator")
  static bool IsEnded(const FCesiumGeoJsonObjectIterator& Iterator);

  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Iterator")
  static FCesiumGeoJsonFeature
  GetFeature(UPARAM(Ref) FCesiumGeoJsonObjectIterator& Iterator) {
    const CesiumVectorData::GeoJsonObject* pFeatureObject =
        Iterator._iterator.getFeature();
    if (pFeatureObject) {
      const CesiumVectorData::GeoJsonFeature* pFeature =
          pFeatureObject->getIf<CesiumVectorData::GeoJsonFeature>();
      if (pFeature) {
        return FCesiumGeoJsonFeature(Iterator._object.getDocument(), pFeature);
      }
    }

    return FCesiumGeoJsonFeature();
  }
};

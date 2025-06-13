#include "CesiumGeoJsonObjectIterator.h"

#include "VecMath.h"

FCesiumGeoJsonObjectIterator::FCesiumGeoJsonObjectIterator()
    : _object(), _iterator() {}

FCesiumGeoJsonObjectIterator::FCesiumGeoJsonObjectIterator(
    const FCesiumGeoJsonObject& object)
    : _object(object), _iterator(*_object.getObject()) {}

FCesiumGeoJsonObject UCesiumGeoJsonObjectIteratorFunctionLibrary::Next(
    UPARAM(Ref) FCesiumGeoJsonObjectIterator& Iterator) {
  if (Iterator._iterator.isEnded()) {
    return FCesiumGeoJsonObject();
  }

  FCesiumGeoJsonObject object(
      Iterator._object.getDocument(),
      &*Iterator._iterator);

  ++Iterator._iterator;
  return object;
}

bool UCesiumGeoJsonObjectIteratorFunctionLibrary::IsEnded(
    const FCesiumGeoJsonObjectIterator& Iterator) {
  return Iterator._iterator.isEnded();
}

FCesiumGeoJsonObjectIterator
UCesiumGeoJsonObjectIteratorFunctionLibrary::Iterate(
    const FCesiumGeoJsonObject& Object) {
  return FCesiumGeoJsonObjectIterator(Object);
}

FCesiumGeoJsonFeature UCesiumGeoJsonObjectIteratorFunctionLibrary::GetFeature(
    UPARAM(Ref) FCesiumGeoJsonObjectIterator& Iterator) {
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

FCesiumGeoJsonPointIterator::FCesiumGeoJsonPointIterator()
    : _object(), _iterator() {}

FCesiumGeoJsonPointIterator::FCesiumGeoJsonPointIterator(
    const FCesiumGeoJsonObject& object)
    : _object(object), _iterator(*object.getObject()) {}

FCesiumGeoJsonPointIterator UCesiumGeoJsonPointIteratorFunctionLibrary::Iterate(
    const FCesiumGeoJsonObject& Object) {
  return FCesiumGeoJsonPointIterator(Object);
}

FVector UCesiumGeoJsonPointIteratorFunctionLibrary::Next(
    UPARAM(Ref) FCesiumGeoJsonPointIterator& Iterator) {
  if (Iterator._iterator == CesiumVectorData::ConstGeoJsonPointIterator()) {
    return FVector::ZeroVector;
  }

  const FVector Vector = VecMath::createVector(*Iterator._iterator);
  ++Iterator._iterator;
  return Vector;
}

bool UCesiumGeoJsonPointIteratorFunctionLibrary::IsEnded(
    const FCesiumGeoJsonPointIterator& Iterator) {
  return Iterator._iterator == CesiumVectorData::ConstGeoJsonPointIterator();
}

FCesiumGeoJsonLineStringIterator::FCesiumGeoJsonLineStringIterator()
    : _object(), _iterator() {}

FCesiumGeoJsonLineStringIterator::FCesiumGeoJsonLineStringIterator(
    const FCesiumGeoJsonObject& object)
    : _object(object), _iterator(*object.getObject()) {}

FCesiumGeoJsonLineStringIterator
UCesiumGeoJsonLineStringIteratorFunctionLibrary::Iterate(
    const FCesiumGeoJsonObject& Object) {
  return FCesiumGeoJsonLineStringIterator(Object);
}

FCesiumGeoJsonLineString UCesiumGeoJsonLineStringIteratorFunctionLibrary::Next(
    UPARAM(Ref) FCesiumGeoJsonLineStringIterator& Iterator) {
  if (Iterator._iterator ==
      CesiumVectorData::ConstGeoJsonLineStringIterator()) {
    return FCesiumGeoJsonLineString();
  }

  TArray<FVector> Points;
  Points.Reserve(Iterator._iterator->size());
  for (const glm::dvec3& point : *Iterator._iterator) {
    Points.Emplace(VecMath::createVector(point));
  }

  ++Iterator._iterator;
  return FCesiumGeoJsonLineString(MoveTemp(Points));
}

bool UCesiumGeoJsonLineStringIteratorFunctionLibrary::IsEnded(
    const FCesiumGeoJsonLineStringIterator& Iterator) {
  return Iterator._iterator ==
         CesiumVectorData::ConstGeoJsonLineStringIterator();
}

FCesiumGeoJsonPolygonIterator::FCesiumGeoJsonPolygonIterator()
    : _object(), _iterator() {}

FCesiumGeoJsonPolygonIterator::FCesiumGeoJsonPolygonIterator(
    const FCesiumGeoJsonObject& object)
    : _object(object), _iterator(*object.getObject()) {}

FCesiumGeoJsonPolygonIterator
UCesiumGeoJsonPolygonIteratorFunctionLibrary::Iterate(
    const FCesiumGeoJsonObject& Object) {
  return FCesiumGeoJsonPolygonIterator(Object);
}

FCesiumGeoJsonPolygon UCesiumGeoJsonPolygonIteratorFunctionLibrary::Next(
    UPARAM(Ref) FCesiumGeoJsonPolygonIterator& Iterator) {
  if (Iterator._iterator == CesiumVectorData::ConstGeoJsonPolygonIterator()) {
    return FCesiumGeoJsonPolygon();
  }

  FCesiumGeoJsonPolygon Polygon(
      Iterator._object.getDocument(),
      &*Iterator._iterator);

  ++Iterator._iterator;
  return Polygon;
}

bool UCesiumGeoJsonPolygonIteratorFunctionLibrary::IsEnded(
    const FCesiumGeoJsonPolygonIterator& Iterator) {
  return Iterator._iterator == CesiumVectorData::ConstGeoJsonPolygonIterator();
}

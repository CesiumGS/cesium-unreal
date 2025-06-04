#include "CesiumGeoJsonObjectIterator.h"

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

#include "CesiumGeoJsonObject.h"

#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/CompositeCartographicPolygon.h"
#include "Dom/JsonObject.h"

#include <utility>
#include <variant>
#include <vector>

int64 UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsInteger(
    const FCesiumGeoJsonFeature& InFeature) {
  if (!InFeature._document || !InFeature._feature) {
    return -1;
  }
  const int64_t* pId = std::get_if<int64_t>(&InFeature._feature->id);
  if (!pId) {
    return -1;
  }

  return *pId;
}

FString UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsString(
    const FCesiumGeoJsonFeature& InFeature) {
  if (!InFeature._document || !InFeature._feature) {
    return {};
  }
  struct GetIdVisitor {
    FString operator()(const int64_t& id) { return FString::FromInt(id); }
    FString operator()(const std::string& id) {
      return UTF8_TO_TCHAR(id.c_str());
    }
    FString operator()(const std::monostate& id) { return {}; }
  };

  return std::visit(GetIdVisitor{}, InFeature._feature->id);
}

namespace {
TSharedPtr<FJsonValue>
jsonValueToUnrealJsonValue(const CesiumUtility::JsonValue& value) {
  struct JsonValueVisitor {
    TSharedPtr<FJsonValue> operator()(const CesiumUtility::JsonValue::Null&) {
      return MakeShared<FJsonValueNull>();
    }
    TSharedPtr<FJsonValue>
    operator()(const CesiumUtility::JsonValue::Bool& value) {
      return MakeShared<FJsonValueBoolean>(value);
    }
    TSharedPtr<FJsonValue> operator()(const std::string& value) {
      return MakeShared<FJsonValueString>(UTF8_TO_TCHAR(value.c_str()));
    }
    TSharedPtr<FJsonValue> operator()(const double& value) {
      return MakeShared<FJsonValueNumber>(value);
    }
    TSharedPtr<FJsonValue> operator()(const std::uint64_t& value) {
      return MakeShared<FJsonValueNumberString>(FString::FromInt(value));
    }
    TSharedPtr<FJsonValue> operator()(const std::int64_t& value) {
      return MakeShared<FJsonValueNumberString>(FString::FromInt(value));
    }
    TSharedPtr<FJsonValue>
    operator()(const CesiumUtility::JsonValue::Array& value) {
      TArray<TSharedPtr<FJsonValue>> values;
      values.Reserve(value.size());
      for (const CesiumUtility::JsonValue& v : value) {
        values.Emplace(jsonValueToUnrealJsonValue(v));
      }
      return MakeShared<FJsonValueArray>(MoveTemp(values));
    }
    TSharedPtr<FJsonValue>
    operator()(const CesiumUtility::JsonValue::Object& value) {
      TSharedPtr<FJsonObject> obj = MakeShared<FJsonObject>();
      for (const auto& [k, v] : value) {
        obj->SetField(UTF8_TO_TCHAR(k.c_str()), jsonValueToUnrealJsonValue(v));
      }
      return MakeShared<FJsonValueObject>(MoveTemp(obj));
    };
  };

  return std::visit(JsonValueVisitor{}, value.value);
}
} // namespace

FJsonObjectWrapper UCesiumGeoJsonFeatureBlueprintLibrary::GetProperties(
    const FCesiumGeoJsonFeature& InFeature) {
  if (!InFeature._document || !InFeature._feature) {
    return {};
  }
  TSharedPtr<FJsonObject> object = MakeShared<FJsonObject>();
  if (InFeature._feature->properties) {
    for (const auto& [k, v] : *InFeature._feature->properties) {
      object->SetField(UTF8_TO_TCHAR(k.c_str()), jsonValueToUnrealJsonValue(v));
    }
  }

  FJsonObjectWrapper wrapper;
  wrapper.JsonObject = object;
  return wrapper;
}

FCesiumGeoJsonObject UCesiumGeoJsonFeatureBlueprintLibrary::GetGeometry(
    const FCesiumGeoJsonFeature& InFeature) {
  if (!InFeature._document || !InFeature._feature ||
      !InFeature._feature->geometry) {
    return {};
  }

  return FCesiumGeoJsonObject(
      InFeature._document,
      InFeature._feature->geometry.get());
}

bool UCesiumGeoJsonFeatureBlueprintLibrary::IsValid(
    const FCesiumGeoJsonFeature& InFeature) {
  return InFeature._document != nullptr && InFeature._feature != nullptr;
}

bool UCesiumGeoJsonObjectBlueprintLibrary::IsValid(
    const FCesiumGeoJsonObject& InObject) {
  return InObject._document != nullptr && InObject._object != nullptr;
}

ECesiumGeoJsonObjectType UCesiumGeoJsonObjectBlueprintLibrary::GetObjectType(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._document || !InObject._object) {
    return {};
  }

  return (ECesiumGeoJsonObjectType)InObject._object->getType();
}

namespace {
FVector
cartographicToVector(const CesiumGeospatial::Cartographic& coordinates) {
  return FVector(
      CesiumUtility::Math::radiansToDegrees(coordinates.longitude),
      CesiumUtility::Math::radiansToDegrees(coordinates.latitude),
      coordinates.height);
}
} // namespace

FVector UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsPoint(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._document || !InObject._object) {
    return {};
  }

  const CesiumVectorData::GeoJsonPoint* pPoint =
      std::get_if<CesiumVectorData::GeoJsonPoint>(&InObject._object->value);

  if (!pPoint) {
    return FVector::ZeroVector;
  }

  return cartographicToVector(pPoint->coordinates);
}

TArray<FVector> UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsMultiPoint(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._document || !InObject._object) {
    return {};
  }

  const CesiumVectorData::GeoJsonMultiPoint* pMultiPoint =
      std::get_if<CesiumVectorData::GeoJsonMultiPoint>(
          &InObject._object->value);

  if (!pMultiPoint) {
    return TArray<FVector>();
  }

  TArray<FVector> Points;
  Points.Reserve(pMultiPoint->coordinates.size());

  for (size_t i = 0; i < pMultiPoint->coordinates.size(); i++) {
    Points.Emplace(cartographicToVector(pMultiPoint->coordinates[i]));
  }

  return Points;
}

FCesiumGeoJsonLineString
UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsLineString(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._document || !InObject._object) {
    return {};
  }

  const CesiumVectorData::GeoJsonLineString* pLineString =
      std::get_if<CesiumVectorData::GeoJsonLineString>(
          &InObject._object->value);

  if (!pLineString) {
    return TArray<FVector>();
  }

  TArray<FVector> Points;
  Points.Reserve(pLineString->coordinates.size());

  for (size_t i = 0; i < pLineString->coordinates.size(); i++) {
    Points.Emplace(cartographicToVector(pLineString->coordinates[i]));
  }

  return FCesiumGeoJsonLineString(MoveTemp(Points));
}

TArray<FCesiumGeoJsonLineString>
UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsMultiLineString(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._document || !InObject._object) {
    return {};
  }

  const CesiumVectorData::GeoJsonMultiLineString* pMultiLineString =
      std::get_if<CesiumVectorData::GeoJsonMultiLineString>(
          &InObject._object->value);

  if (!pMultiLineString) {
    return TArray<FCesiumGeoJsonLineString>();
  }

  TArray<FCesiumGeoJsonLineString> Lines;
  Lines.Reserve(pMultiLineString->coordinates.size());
  for (size_t i = 0; i < pMultiLineString->coordinates[i].size(); i++) {

    TArray<FVector> Points;
    Points.Reserve(pMultiLineString->coordinates[i].size());

    for (size_t j = 0; j < pMultiLineString->coordinates[i].size(); j++) {
      Points.Emplace(cartographicToVector(pMultiLineString->coordinates[i][j]));
    }

    Lines.Emplace(MoveTemp(Points));
  }

  return Lines;
}

FCesiumGeoJsonPolygon UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsPolygon(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._document || !InObject._object) {
    return {};
  }

  const CesiumVectorData::GeoJsonPolygon* pPolygon =
      std::get_if<CesiumVectorData::GeoJsonPolygon>(&InObject._object->value);

  if (!pPolygon) {
    return FCesiumGeoJsonPolygon();
  }

  return FCesiumGeoJsonPolygon(InObject._document, &pPolygon->coordinates);
}

TArray<FCesiumGeoJsonPolygon>
UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsMultiPolygon(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._document || !InObject._object) {
    return {};
  }

  const CesiumVectorData::GeoJsonMultiPolygon* pMultiPolygon =
      std::get_if<CesiumVectorData::GeoJsonMultiPolygon>(
          &InObject._object->value);

  if (!pMultiPolygon) {
    return TArray<FCesiumGeoJsonPolygon>();
  }

  TArray<FCesiumGeoJsonPolygon> Polygons;
  Polygons.Reserve(pMultiPolygon->coordinates.size());

  for (size_t i = 0; i < pMultiPolygon->coordinates.size(); i++) {
    Polygons.Emplace(InObject._document, &pMultiPolygon->coordinates[i]);
  }

  return Polygons;
}

TArray<FCesiumGeoJsonObject>
UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsGeometryCollection(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._document || !InObject._object) {
    return {};
  }

  const CesiumVectorData::GeoJsonGeometryCollection* pGeometryCollection =
      std::get_if<CesiumVectorData::GeoJsonGeometryCollection>(
          &InObject._object->value);

  if (!pGeometryCollection) {

    return TArray<FCesiumGeoJsonObject>();
  }

  TArray<FCesiumGeoJsonObject> Geometries;
  Geometries.Reserve(pGeometryCollection->geometries.size());

  for (size_t i = 0; i < pGeometryCollection->geometries.size(); i++) {
    Geometries.Emplace(FCesiumGeoJsonObject(
        InObject._document,
        &pGeometryCollection->geometries[i]));
  }

  return Geometries;
}

FCesiumGeoJsonFeature UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsFeature(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._document || !InObject._object) {
    return {};
  }

  const CesiumVectorData::GeoJsonFeature* pFeature =
      std::get_if<CesiumVectorData::GeoJsonFeature>(&InObject._object->value);

  if (!pFeature) {
    return FCesiumGeoJsonFeature();
  }

  return FCesiumGeoJsonFeature(InObject._document, pFeature);
}

TArray<FCesiumGeoJsonFeature>
UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsFeatureCollection(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._document || !InObject._object) {
    return {};
  }

  const CesiumVectorData::GeoJsonFeatureCollection* pFeatureCollection =
      std::get_if<CesiumVectorData::GeoJsonFeatureCollection>(
          &InObject._object->value);

  TArray<FCesiumGeoJsonFeature> Features;
  Features.Reserve(pFeatureCollection->features.size());

  for (size_t i = 0; i < pFeatureCollection->features.size(); i++) {
    Features.Emplace(FCesiumGeoJsonFeature(
        InObject._document,
        &pFeatureCollection->features[i]));
  }

  return Features;
}

FCesiumGeoJsonLineString::FCesiumGeoJsonLineString(TArray<FVector>&& InPoints)
    : Points(MoveTemp(InPoints)) {}

TArray<FCesiumGeoJsonLineString>
UCesiumGeoJsonPolygonBlueprintFunctionLibrary::GetPolygonRings(
    const FCesiumGeoJsonPolygon& InPolygon) {
  TArray<FCesiumGeoJsonLineString> Rings;
  Rings.Reserve(InPolygon._rings->size());

  for (size_t i = 0; i < InPolygon._rings->size(); i++) {
    TArray<FVector> Points;
    Points.Reserve((*InPolygon._rings)[i].size());

    for (size_t j = 0; j < (*InPolygon._rings)[i].size(); j++) {
      Points.Emplace(cartographicToVector((*InPolygon._rings)[i][j]));
    }

    Rings.Emplace(MoveTemp(Points));
  }

  return Rings;
}

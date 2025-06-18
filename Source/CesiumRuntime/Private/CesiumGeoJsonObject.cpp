#include "CesiumGeoJsonObject.h"

#include "CesiumGeospatial/Cartographic.h"
#include "Dom/JsonObject.h"
#include "VecMath.h"

#include <utility>
#include <variant>
#include <vector>

FCesiumGeoJsonFeature::FCesiumGeoJsonFeature()
    : _pDocument(nullptr), _pFeature(nullptr) {}

FCesiumGeoJsonFeature::FCesiumGeoJsonFeature(
    const std::shared_ptr<CesiumVectorData::GeoJsonDocument>& document,
    const CesiumVectorData::GeoJsonFeature* feature)
    : _pDocument(document), _pFeature(feature) {}

ECesiumGeoJsonFeatureIdType UCesiumGeoJsonFeatureBlueprintLibrary::GetIdType(
    const FCesiumGeoJsonFeature& InFeature) {
  if (!InFeature._pDocument || !InFeature._pFeature ||
      std::holds_alternative<std::monostate>(InFeature._pFeature->id)) {
    return ECesiumGeoJsonFeatureIdType::None;
  }

  return std::holds_alternative<int64_t>(InFeature._pFeature->id)
             ? ECesiumGeoJsonFeatureIdType::Integer
             : ECesiumGeoJsonFeatureIdType::String;
}

int64 UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsInteger(
    const FCesiumGeoJsonFeature& InFeature) {
  if (!InFeature._pDocument || !InFeature._pFeature) {
    return -1;
  }
  const int64_t* pId = std::get_if<int64_t>(&InFeature._pFeature->id);
  if (!pId) {
    return -1;
  }

  return *pId;
}

FString UCesiumGeoJsonFeatureBlueprintLibrary::GetIdAsString(
    const FCesiumGeoJsonFeature& InFeature) {
  if (!InFeature._pDocument || !InFeature._pFeature) {
    return {};
  }
  struct GetIdVisitor {
    FString operator()(const int64_t& id) { return FString::FromInt(id); }
    FString operator()(const std::string& id) {
      return UTF8_TO_TCHAR(id.c_str());
    }
    FString operator()(const std::monostate& id) { return {}; }
  };

  return std::visit(GetIdVisitor{}, InFeature._pFeature->id);
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
      // If this value will fit into a double losslessly, we return it as a JSON
      // number.
      const std::optional<double> doubleOpt =
          CesiumUtility::losslessNarrow<double, std::uint64_t>(value);
      if (doubleOpt) {
        return MakeShared<FJsonValueNumber>(*doubleOpt);
      }

      // Otherwise, we return it as a number string.
      return MakeShared<FJsonValueNumberString>(FString::FromInt(value));
    }
    TSharedPtr<FJsonValue> operator()(const std::int64_t& value) {
      // If this value will fit into a double losslessly, we return it as a JSON
      // number.
      const std::optional<double> doubleOpt =
          CesiumUtility::losslessNarrow<double, std::int64_t>(value);
      if (doubleOpt) {
        return MakeShared<FJsonValueNumber>(*doubleOpt);
      }

      // Otherwise, we return it as a number string.
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
  if (!InFeature._pDocument || !InFeature._pFeature) {
    return {};
  }
  TSharedPtr<FJsonObject> object = MakeShared<FJsonObject>();
  if (InFeature._pFeature->properties) {
    for (const auto& [k, v] : *InFeature._pFeature->properties) {
      object->SetField(UTF8_TO_TCHAR(k.c_str()), jsonValueToUnrealJsonValue(v));
    }
  }

  FJsonObjectWrapper wrapper;
  wrapper.JsonObject = MoveTemp(object);
  return wrapper;
}

FCesiumGeoJsonObject UCesiumGeoJsonFeatureBlueprintLibrary::GetGeometry(
    const FCesiumGeoJsonFeature& InFeature) {
  if (!InFeature._pDocument || !InFeature._pFeature ||
      !InFeature._pFeature->geometry) {
    return {};
  }

  return FCesiumGeoJsonObject(
      InFeature._pDocument,
      InFeature._pFeature->geometry.get());
}

bool UCesiumGeoJsonFeatureBlueprintLibrary::IsValid(
    const FCesiumGeoJsonFeature& InFeature) {
  return InFeature._pDocument != nullptr && InFeature._pFeature != nullptr;
}

bool UCesiumGeoJsonObjectBlueprintLibrary::IsValid(
    const FCesiumGeoJsonObject& InObject) {
  return InObject._pDocument != nullptr && InObject._pObject != nullptr;
}

ECesiumGeoJsonObjectType UCesiumGeoJsonObjectBlueprintLibrary::GetObjectType(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return {};
  }

  // Assert that enums are equivalent.
  static_assert(
      (uint32_t)CesiumVectorData::GeoJsonObjectType::Point ==
      (uint32_t)ECesiumGeoJsonObjectType::Point);
  static_assert(
      (uint32_t)CesiumVectorData::GeoJsonObjectType::MultiPoint ==
      (uint32_t)ECesiumGeoJsonObjectType::MultiPoint);
  static_assert(
      (uint32_t)CesiumVectorData::GeoJsonObjectType::LineString ==
      (uint32_t)ECesiumGeoJsonObjectType::LineString);
  static_assert(
      (uint32_t)CesiumVectorData::GeoJsonObjectType::MultiLineString ==
      (uint32_t)ECesiumGeoJsonObjectType::MultiLineString);
  static_assert(
      (uint32_t)CesiumVectorData::GeoJsonObjectType::Polygon ==
      (uint32_t)ECesiumGeoJsonObjectType::Polygon);
  static_assert(
      (uint32_t)CesiumVectorData::GeoJsonObjectType::MultiPolygon ==
      (uint32_t)ECesiumGeoJsonObjectType::MultiPolygon);
  static_assert(
      (uint32_t)CesiumVectorData::GeoJsonObjectType::GeometryCollection ==
      (uint32_t)ECesiumGeoJsonObjectType::GeometryCollection);
  static_assert(
      (uint32_t)CesiumVectorData::GeoJsonObjectType::Feature ==
      (uint32_t)ECesiumGeoJsonObjectType::Feature);
  static_assert(
      (uint32_t)CesiumVectorData::GeoJsonObjectType::FeatureCollection ==
      (uint32_t)ECesiumGeoJsonObjectType::FeatureCollection);

  return (ECesiumGeoJsonObjectType)InObject._pObject->getType();
}

FBox UCesiumGeoJsonObjectBlueprintLibrary::GetBoundingBox(
    const FCesiumGeoJsonObject& InObject,
    EHasValue& Branches) {
  if (!InObject._pDocument || !InObject._pObject) {
    Branches = EHasValue::NoValue;
    return {};
  }

  const std::optional<CesiumGeometry::AxisAlignedBox>& boundingBox =
      InObject._pObject->getBoundingBox();
  if (boundingBox) {
    Branches = EHasValue::HasValue;
    return FBox(
        FVector(
            boundingBox->minimumX,
            boundingBox->minimumY,
            boundingBox->minimumZ),
        FVector(
            boundingBox->maximumX,
            boundingBox->maximumY,
            boundingBox->maximumZ));
  }

  Branches = EHasValue::NoValue;
  return {};
}

FJsonObjectWrapper UCesiumGeoJsonObjectBlueprintLibrary::GetForeignMembers(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return {};
  }

  TSharedPtr<FJsonObject> object = MakeShared<FJsonObject>();
  for (const auto& [k, v] : InObject._pObject->getForeignMembers()) {
    object->SetField(UTF8_TO_TCHAR(k.c_str()), jsonValueToUnrealJsonValue(v));
  }

  FJsonObjectWrapper wrapper;
  wrapper.JsonObject = MoveTemp(object);
  return wrapper;
}

FVector UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsPoint(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return {};
  }

  const CesiumVectorData::GeoJsonPoint* pPoint =
      std::get_if<CesiumVectorData::GeoJsonPoint>(&InObject._pObject->value);

  if (!pPoint) {
    return FVector::ZeroVector;
  }

  return VecMath::createVector(pPoint->coordinates);
}

TArray<FVector> UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsMultiPoint(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return {};
  }

  const CesiumVectorData::GeoJsonMultiPoint* pMultiPoint =
      std::get_if<CesiumVectorData::GeoJsonMultiPoint>(
          &InObject._pObject->value);

  if (!pMultiPoint) {
    return TArray<FVector>();
  }

  TArray<FVector> Points;
  Points.Reserve(pMultiPoint->coordinates.size());

  for (size_t i = 0; i < pMultiPoint->coordinates.size(); i++) {
    Points.Emplace(VecMath::createVector(pMultiPoint->coordinates[i]));
  }

  return Points;
}

FCesiumGeoJsonLineString
UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsLineString(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return {};
  }

  const CesiumVectorData::GeoJsonLineString* pLineString =
      std::get_if<CesiumVectorData::GeoJsonLineString>(
          &InObject._pObject->value);

  if (!pLineString) {
    return TArray<FVector>();
  }

  TArray<FVector> Points;
  Points.Reserve(pLineString->coordinates.size());

  for (size_t i = 0; i < pLineString->coordinates.size(); i++) {
    Points.Emplace(VecMath::createVector(pLineString->coordinates[i]));
  }

  return FCesiumGeoJsonLineString(MoveTemp(Points));
}

TArray<FCesiumGeoJsonLineString>
UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsMultiLineString(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return {};
  }

  const CesiumVectorData::GeoJsonMultiLineString* pMultiLineString =
      std::get_if<CesiumVectorData::GeoJsonMultiLineString>(
          &InObject._pObject->value);

  if (!pMultiLineString) {
    return TArray<FCesiumGeoJsonLineString>();
  }

  TArray<FCesiumGeoJsonLineString> Lines;
  Lines.Reserve(pMultiLineString->coordinates.size());
  for (size_t i = 0; i < pMultiLineString->coordinates[i].size(); i++) {

    TArray<FVector> Points;
    Points.Reserve(pMultiLineString->coordinates[i].size());

    for (size_t j = 0; j < pMultiLineString->coordinates[i].size(); j++) {
      Points.Emplace(
          VecMath::createVector(pMultiLineString->coordinates[i][j]));
    }

    Lines.Emplace(MoveTemp(Points));
  }

  return Lines;
}

FCesiumGeoJsonPolygon UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsPolygon(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return {};
  }

  const CesiumVectorData::GeoJsonPolygon* pPolygon =
      std::get_if<CesiumVectorData::GeoJsonPolygon>(&InObject._pObject->value);

  if (!pPolygon) {
    return FCesiumGeoJsonPolygon();
  }

  return FCesiumGeoJsonPolygon(InObject._pDocument, &pPolygon->coordinates);
}

TArray<FCesiumGeoJsonPolygon>
UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsMultiPolygon(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return {};
  }

  const CesiumVectorData::GeoJsonMultiPolygon* pMultiPolygon =
      std::get_if<CesiumVectorData::GeoJsonMultiPolygon>(
          &InObject._pObject->value);

  if (!pMultiPolygon) {
    return TArray<FCesiumGeoJsonPolygon>();
  }

  TArray<FCesiumGeoJsonPolygon> Polygons;
  Polygons.Reserve(pMultiPolygon->coordinates.size());

  for (size_t i = 0; i < pMultiPolygon->coordinates.size(); i++) {
    Polygons.Emplace(InObject._pDocument, &pMultiPolygon->coordinates[i]);
  }

  return Polygons;
}

TArray<FCesiumGeoJsonObject>
UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsGeometryCollection(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return {};
  }

  const CesiumVectorData::GeoJsonGeometryCollection* pGeometryCollection =
      std::get_if<CesiumVectorData::GeoJsonGeometryCollection>(
          &InObject._pObject->value);

  if (!pGeometryCollection) {

    return TArray<FCesiumGeoJsonObject>();
  }

  TArray<FCesiumGeoJsonObject> Geometries;
  Geometries.Reserve(pGeometryCollection->geometries.size());

  for (size_t i = 0; i < pGeometryCollection->geometries.size(); i++) {
    Geometries.Emplace(FCesiumGeoJsonObject(
        InObject._pDocument,
        &pGeometryCollection->geometries[i]));
  }

  return Geometries;
}

FCesiumGeoJsonFeature UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsFeature(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return {};
  }

  const CesiumVectorData::GeoJsonFeature* pFeature =
      std::get_if<CesiumVectorData::GeoJsonFeature>(&InObject._pObject->value);

  if (!pFeature) {
    return FCesiumGeoJsonFeature();
  }

  return FCesiumGeoJsonFeature(InObject._pDocument, pFeature);
}

TArray<FCesiumGeoJsonFeature>
UCesiumGeoJsonObjectBlueprintLibrary::GetObjectAsFeatureCollection(
    const FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return {};
  }

  const CesiumVectorData::GeoJsonFeatureCollection* pFeatureCollection =
      std::get_if<CesiumVectorData::GeoJsonFeatureCollection>(
          &InObject._pObject->value);

  TArray<FCesiumGeoJsonFeature> Features;
  Features.Reserve(pFeatureCollection->features.size());

  for (size_t i = 0; i < pFeatureCollection->features.size(); i++) {
    const CesiumVectorData::GeoJsonFeature* pFeature =
        pFeatureCollection->features[i]
            .getIf<CesiumVectorData::GeoJsonFeature>();
    if (pFeature == nullptr) {
      continue;
    }
    Features.Emplace(FCesiumGeoJsonFeature(InObject._pDocument, pFeature));
  }

  return Features;
}

FCesiumVectorStyle UCesiumGeoJsonObjectBlueprintLibrary::GetStyle(
    const FCesiumGeoJsonObject& InObject,
    EHasValue& Branches) {
  if (!InObject._pDocument || !InObject._pObject) {
    Branches = EHasValue::NoValue;
    return FCesiumVectorStyle();
  }

  const std::optional<CesiumVectorData::VectorStyle> style =
      InObject._pObject->getStyle();
  Branches = style ? EHasValue::HasValue : EHasValue::NoValue;
  return style ? FCesiumVectorStyle::fromNative(*style) : FCesiumVectorStyle();
}

void UCesiumGeoJsonObjectBlueprintLibrary::SetStyle(
    UPARAM(Ref) FCesiumGeoJsonObject& InObject,
    const FCesiumVectorStyle& InStyle) {
  if (!InObject._pDocument || !InObject._pObject) {
    return;
  }

  const_cast<CesiumVectorData::GeoJsonObject*>(InObject._pObject)->getStyle() =
      InStyle.toNative();
}

void UCesiumGeoJsonObjectBlueprintLibrary::ClearStyle(
    UPARAM(Ref) FCesiumGeoJsonObject& InObject) {
  if (!InObject._pDocument || !InObject._pObject) {
    return;
  }

  const_cast<CesiumVectorData::GeoJsonObject*>(InObject._pObject)->getStyle() =
      std::nullopt;
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
      Points.Emplace(VecMath::createVector((*InPolygon._rings)[i][j]));
    }

    Rings.Emplace(MoveTemp(Points));
  }

  return Rings;
}

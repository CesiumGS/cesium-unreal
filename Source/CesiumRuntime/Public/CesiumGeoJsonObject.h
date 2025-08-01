// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumUtility/IntrusivePointer.h"
#include "CesiumVectorData/GeoJsonDocument.h"
#include "CesiumVectorData/GeoJsonObject.h"
#include "CesiumVectorStyle.h"
#include "JsonObjectWrapper.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SharedPointer.h"

#include <glm/ext/vector_double3.hpp>
#include <memory>

#include "CesiumGeoJsonObject.generated.h"

/**
 * @brief A single object in the GeoJSON document.
 */
USTRUCT(BlueprintType)
struct FCesiumGeoJsonObject {
  GENERATED_BODY()

  /**
   * @brief Creates a new `FCesiumGeoJsonObject` containing an empty GeoJSON
   * object.
   */
  FCesiumGeoJsonObject() : _pDocument(nullptr), _pObject(nullptr) {}

  /**
   * @brief Creates a new `FCesiumGeoJsonObject` wrapping the provided
   * `CesiumVectorData::GeoJsonObject`.
   */
  FCesiumGeoJsonObject(
      const std::shared_ptr<CesiumVectorData::GeoJsonDocument>& doc,
      const CesiumVectorData::GeoJsonObject* pObject)
      : _pDocument(doc), _pObject(pObject) {}

  const std::shared_ptr<CesiumVectorData::GeoJsonDocument>&
  getDocument() const {
    return this->_pDocument;
  }

  const CesiumVectorData::GeoJsonObject* getObject() const {
    return this->_pObject;
  }

private:
  std::shared_ptr<CesiumVectorData::GeoJsonDocument> _pDocument;
  const CesiumVectorData::GeoJsonObject* _pObject;

  friend class UCesiumGeoJsonObjectBlueprintLibrary;
};

/**
 * @brief The supported GeoJSON object types.
 */
UENUM(BlueprintType)
enum class ECesiumGeoJsonObjectType : uint8 {
  Point = 0,
  MultiPoint = 1,
  LineString = 2,
  MultiLineString = 3,
  Polygon = 4,
  MultiPolygon = 5,
  GeometryCollection = 6,
  Feature = 7,
  FeatureCollection = 8
};

/**
 * @brief a GeoJson "Feature" object.
 */
USTRUCT(BlueprintType)
struct FCesiumGeoJsonFeature {
  GENERATED_BODY()

  /** @brief Creates a new `FCesiumVectorPrimitive` with an empty primitive. */
  FCesiumGeoJsonFeature();

  /**
   * @brief Creates a new `FCesiumGeoJsonFeature` wrapping the provided
   * `CesiumVectorData::GeoJsonFeature`.
   */
  FCesiumGeoJsonFeature(
      const std::shared_ptr<CesiumVectorData::GeoJsonDocument>& document,
      const CesiumVectorData::GeoJsonFeature* feature);

private:
  std::shared_ptr<CesiumVectorData::GeoJsonDocument> _pDocument;
  const CesiumVectorData::GeoJsonFeature* _pFeature;

  friend class UCesiumGeoJsonFeatureBlueprintLibrary;
};

/**
 * @brief The type of a feature's ID field.
 */
UENUM(BlueprintType)
enum class ECesiumGeoJsonFeatureIdType : uint8 {
  /**
   * @brief The feature has no ID.
   */
  None,
  /**
   * @brief The feature's ID is an integer.
   */
  Integer,
  /**
   * @brief The feature's ID is a string.
   */
  String
};

UCLASS()
class UCesiumGeoJsonFeatureBlueprintLibrary : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Returns true if this feature's
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Feature")
  static ECesiumGeoJsonFeatureIdType
  GetIdType(const FCesiumGeoJsonFeature& InFeature);

  /**
   * @brief Returns the ID of the provided feature, or -1 if no ID was
   * present or if the ID is not an integer.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Feature")
  static int64 GetIdAsInteger(const FCesiumGeoJsonFeature& InFeature);

  /**
   * @brief Returns the ID of the provided feature, or an empty string if no
   * ID was present. If the ID is an integer, it will be converted to a string.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Feature")
  static FString GetIdAsString(const FCesiumGeoJsonFeature& InFeature);

  /**
   * @brief Obtains the properties attached to this feature, if any.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Feature")
  static FJsonObjectWrapper
  GetProperties(const FCesiumGeoJsonFeature& InFeature);

  /**
   * @brief Obtains the `FCesiumGeoJsonObject` specified as the geometry of this
   * feature, if any.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Feature")
  static FCesiumGeoJsonObject
  GetGeometry(const FCesiumGeoJsonFeature& InFeature);

  /**
   * @brief Checks if this `FCesiumGeoJsonFeature` is valid or not.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Feature")
  static bool IsValid(const FCesiumGeoJsonFeature& InFeature);
};

/**
 * @brief A `FCesiumGeoJsonPolygon` is a polygon made up of one or
 * more linear rings.
 */
USTRUCT(BlueprintType)
struct FCesiumGeoJsonPolygon {
  GENERATED_BODY()

  /**
   * @brief Creates a new `FCesiumGeoJsonPolygon` with an empty
   * composite polygon.
   */
  FCesiumGeoJsonPolygon() : _document(nullptr), _rings(nullptr) {}

  /**
   * @brief Creates a new `FCesiumGeoJsonPolygon` wrapping the
   * provided `CesiumGeospatial::CompositeCartographicPolygon`.
   */
  FCesiumGeoJsonPolygon(
      const std::shared_ptr<CesiumVectorData::GeoJsonDocument>& document,
      const std::vector<std::vector<glm::dvec3>>* rings)
      : _document(document), _rings(rings) {}

private:
  std::shared_ptr<CesiumVectorData::GeoJsonDocument> _document;
  const std::vector<std::vector<glm::dvec3>>* _rings;

  friend class UCesiumGeoJsonPolygonBlueprintFunctionLibrary;
};

/**
 * @brief A `FCesiumGeoJsonLineString` is a set of points representing a line.
 */
USTRUCT(BlueprintType)
struct FCesiumGeoJsonLineString {
  GENERATED_BODY()
public:
  /**
   * @brief Creates a new `FCesiumGeoJsonLineString` with an empty line string.
   */
  FCesiumGeoJsonLineString() : Points() {}

  /**
   * @brief Creates a new `FCesiumGeoJsonLineString` from a set of
   * Longitude-Latitude-Height points.
   */
  FCesiumGeoJsonLineString(TArray<FVector>&& InPoints);

  /**
   * @brief The Longitude-Latitude-Height points of this polygon.
   */
  UPROPERTY(BlueprintReadOnly, Category = "Cesium|Vector|Polygon")
  TArray<FVector> Points;
};

UCLASS()
class UCesiumGeoJsonPolygonBlueprintFunctionLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()
public:
  /**
   * @brief Returns the linear rings that make up this composite polygon.
   *
   * The first returned ring represents the outer bounds of the polygon. Any
   * additional rings define holes within those bounds.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Polygon")
  static TArray<FCesiumGeoJsonLineString>
  GetPolygonRings(const FCesiumGeoJsonPolygon& InPolygon);
};

/**
 * @brief Enum used for branching when a UFUNCTION could return a value or could
 * return no value.
 */
UENUM()
enum class EHasValue : uint8 { HasValue, NoValue };

/**
 * @brief A Blueprint Funciton Library for interacting with `FCesiumVectorNode`
 * values.
 */
UCLASS()
class UCesiumGeoJsonObjectBlueprintLibrary : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Checks if the provided GeoJSON object is valid.
   *
   * Any operations performed with an invalid object will likely give incorrect
   * results.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static bool IsValid(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief Returns the `ECesiumGeoJsonObjectType` of the GeoJSON value this
   * object represents.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static ECesiumGeoJsonObjectType
  GetObjectType(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief Attempts to obtain this GeoJSON object's bounding box. If the `No
   * Value` branch is called, the object has no bounding box.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|Vector|Object",
      Meta = (ExpandEnumAsExecs = "Branches"))
  static FBox
  GetBoundingBox(const FCesiumGeoJsonObject& InObject, EHasValue& Branches);

  /**
   * @brief Obtains any foreign members on this GeoJSON object.
   *
   * Foreign members are members found in the loaded GeoJSON document that
   * are not part of the specification for this GeoJSON object type.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static FJsonObjectWrapper
  GetForeignMembers(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief If this object is a GeoJSON Point type, this returns the
   * `coordinates` of that Point. Otherwise, a zero vector is returned.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static FVector GetObjectAsPoint(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief If this object is a GeoJSON MultiPoint type, this returns the array
   * of `coordinates` on that MultiPoint object. Otherwise, an empty array is
   * returned.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static TArray<FVector>
  GetObjectAsMultiPoint(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief If this object is a GeoJSON LineString type, this returns a
   * `FCesiumGeoJsonLineString` representing that line. Otherwise, a
   * `FCesiumGeoJsonLineString` without any points is returned.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static FCesiumGeoJsonLineString
  GetObjectAsLineString(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief If this object is a GeoJSON MultiLineString type, this returns an
   * array of `FCesiumGeoJsonLineString` objects representing the lines.
   * Otherwise, an empty array is returned.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static TArray<FCesiumGeoJsonLineString>
  GetObjectAsMultiLineString(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief If this object is a GeoJSON Polygon type, this returns a
   * `FCesiumGeoJsonPolygon` representing that line. Otherwise, a
   * `FCesiumGeoJsonPolygon` without any points is returned.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static FCesiumGeoJsonPolygon
  GetObjectAsPolygon(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief If this object is a GeoJSON MultiPolygon type, this returns an
   * array of `FCesiumGeoJsonPolygon` objects representing the polygons.
   * Otherwise, an empty array is returned.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static TArray<FCesiumGeoJsonPolygon>
  GetObjectAsMultiPolygon(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief If this object is a GeoJSON GeometryCollection type, this returns an
   * array of `FCesiumGeoJsonObject` objects representing the objects.
   * Otherwise, an empty array is returned.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static TArray<FCesiumGeoJsonObject>
  GetObjectAsGeometryCollection(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief If this object is a GeoJSON Feature type, this returns a
   * `FCesiumGeoJsonFeature` representing that feature. Otherwise, an invalid
   * `FCesiumGeoJsonFeature` is returned.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static FCesiumGeoJsonFeature
  GetObjectAsFeature(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief If this object is a GeoJSON FeatureCollection type, this returns an
   * array of `FCesiumGeoJsonFeature` objects representing the features.
   * Otherwise, an empty array is returned.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static TArray<FCesiumGeoJsonFeature>
  GetObjectAsFeatureCollection(const FCesiumGeoJsonObject& InObject);

  /**
   * @brief Returns the `FCesiumVectorStyle` attached to this object, if any. If
   * there is no style attached to this object, the `No Value` branch is taken.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Object",
      Meta = (ExpandEnumAsExecs = "Branches"))
  static FCesiumVectorStyle
  GetStyle(const FCesiumGeoJsonObject& InObject, EHasValue& Branches);

  /**
   * @brief Sets the style of the given `FCesiumGeoJsonObject`.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Object")
  static void SetStyle(
      UPARAM(Ref) FCesiumGeoJsonObject& InObject,
      const FCesiumVectorStyle& InStyle);

  /**
   * @brief Removes any existing style on the given GeoJSON object.
   *
   * GeoJSON objects without a style specified will inherit their style from
   * their parent object, or that parent's parent, and so on up the tree. If no
   * styles are found anywhere in the ancestors of this object, the default
   * style will be used.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Object")
  static void ClearStyle(UPARAM(Ref) FCesiumGeoJsonObject& InObject);
};

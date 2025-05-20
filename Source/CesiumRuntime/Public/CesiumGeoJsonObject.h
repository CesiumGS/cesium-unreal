// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumUtility/IntrusivePointer.h"
#include "CesiumVectorData/GeoJsonDocument.h"
#include "CesiumVectorData/GeoJsonObject.h"
#include "JsonObjectWrapper.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SharedPointer.h"

#include "CesiumGeoJsonObject.generated.h"

/**
 * @brief A single node in the vector document.
 *
 * A node typically contains geometry along with metadata attached to that
 * geometry.
 */
USTRUCT(BlueprintType)
struct FCesiumGeoJsonObject {
  GENERATED_BODY()

  /**
   * @brief Creates a new `FCesiumGeoJsonObject` containing an empty vector
   * node.
   */
  FCesiumGeoJsonObject() : _document(nullptr), _object(nullptr) {}

  /**
   * @brief Creates a new `FCesiumGeoJsonObject` wrapping the provided
   * `CesiumVectorData::GeoJsonObject`.
   */
  FCesiumGeoJsonObject(
      const CesiumUtility::IntrusivePointer<CesiumVectorData::GeoJsonDocument>&
          doc,
      const CesiumVectorData::GeoJsonObject* node)
      : _document(doc), _object(node) {}

private:
  CesiumUtility::IntrusivePointer<CesiumVectorData::GeoJsonDocument> _document;
  const CesiumVectorData::GeoJsonObject* _object;

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
  FCesiumGeoJsonFeature() : _document(nullptr), _feature(nullptr) {}

  /**
   * @brief Creates a new `FCesiumGeoJsonFeature` wrapping the provided
   * `CesiumVectorData::GeoJsonFeature`.
   */
  FCesiumGeoJsonFeature(
      const CesiumUtility::IntrusivePointer<CesiumVectorData::GeoJsonDocument>&
          document,
      const CesiumVectorData::GeoJsonFeature* feature)
      : _document(document), _feature(feature) {}

private:
  CesiumUtility::IntrusivePointer<CesiumVectorData::GeoJsonDocument> _document;
  const CesiumVectorData::GeoJsonFeature* _feature;

  friend class UCesiumGeoJsonFeatureBlueprintLibrary;
};

UCLASS()
class UCesiumGeoJsonFeatureBlueprintLibrary : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
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
      const CesiumUtility::IntrusivePointer<CesiumVectorData::GeoJsonDocument>&
          document,
      const std::vector<std::vector<CesiumGeospatial::Cartographic>>* rings)
      : _document(document), _rings(rings) {}

private:
  const CesiumUtility::IntrusivePointer<CesiumVectorData::GeoJsonDocument>
      _document;
  const std::vector<std::vector<CesiumGeospatial::Cartographic>>* _rings;

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
 * @brief A Blueprint Funciton Library for interacting with `FCesiumVectorNode`
 * values.
 */
UCLASS()
class UCesiumGeoJsonObjectBlueprintLibrary : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static bool IsValid(const FCesiumGeoJsonObject& InObject);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static ECesiumGeoJsonObjectType
  GetObjectType(const FCesiumGeoJsonObject& InObject);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static FVector GetObjectAsPoint(const FCesiumGeoJsonObject& InObject);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static TArray<FVector>
  GetObjectAsMultiPoint(const FCesiumGeoJsonObject& InObject);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static FCesiumGeoJsonLineString
  GetObjectAsLineString(const FCesiumGeoJsonObject& InObject);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static TArray<FCesiumGeoJsonLineString>
  GetObjectAsMultiLineString(const FCesiumGeoJsonObject& InObject);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static FCesiumGeoJsonPolygon
  GetObjectAsPolygon(const FCesiumGeoJsonObject& InObject);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static TArray<FCesiumGeoJsonPolygon>
  GetObjectAsMultiPolygon(const FCesiumGeoJsonObject& InObject);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static TArray<FCesiumGeoJsonObject>
  GetObjectAsGeometryCollection(const FCesiumGeoJsonObject& InObject);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static FCesiumGeoJsonFeature
  GetObjectAsFeature(const FCesiumGeoJsonObject& InObject);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Object")
  static TArray<FCesiumGeoJsonFeature>
  GetObjectAsFeatureCollection(const FCesiumGeoJsonObject& InObject);
};

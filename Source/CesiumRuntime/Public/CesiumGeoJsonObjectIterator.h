#pragma once

#include "CesiumGeoJsonObject.h"

#include "CesiumGeoJsonObjectIterator.generated.h"

/**
 * Iterates over a GeoJSON object, returning the object itself and all of its
 * children (and their children, and so on).
 */
USTRUCT(BlueprintType)
struct FCesiumGeoJsonObjectIterator {
  GENERATED_BODY()

  /** @brief Creates an iterator that will return no objects. */
  FCesiumGeoJsonObjectIterator();

  /** @brief Creates a new iterator to iterate over the given object. */
  FCesiumGeoJsonObjectIterator(const FCesiumGeoJsonObject& object);

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
  /**
   * Creates an iterator over the GeoJSON object that will return this object
   * and any children (and children of those children, and so on).
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Object")
  static FCesiumGeoJsonObjectIterator
  Iterate(const FCesiumGeoJsonObject& Object);

  /**
   * Moves the iterator to the next available GeoJSON object and returns that
   * object. If no more objects are available, an invalid FCesiumGeoJsonObject
   * is returned.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Iterator")
  static FCesiumGeoJsonObject Next(UPARAM(Ref)
                                       FCesiumGeoJsonObjectIterator& Iterator);

  /**
   * Checks if this iterator has ended (no further objects available).
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Iterator")
  static bool IsEnded(const FCesiumGeoJsonObjectIterator& Iterator);

  /**
   * Gets the feature the current object belongs to, if any.
   *
   * This will be the first parent of this object that is a feature.
   * For example, with a document with a hierarchy like:
   * - FeatureCollection -> Feature -> GeometryCollection -> Point
   * Calling GetFeature on the Point, GeometryCollection, or Feature will both
   * return the same Feature object. Calling GetFeature on the FeatureCollection
   * will return an invalid feature as there is no parent feature.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Iterator")
  static FCesiumGeoJsonFeature
  GetFeature(UPARAM(Ref) FCesiumGeoJsonObjectIterator& Iterator);
};

/**
 * Iterates over every Point value in a GeoJSON object and all of its children.
 */
USTRUCT(BlueprintType)
struct FCesiumGeoJsonPointIterator {
  GENERATED_BODY()

  /** @brief Creates an iterator that will return no points. */
  FCesiumGeoJsonPointIterator();

  /** @brief Creates a new iterator to iterate over the given object. */
  FCesiumGeoJsonPointIterator(const FCesiumGeoJsonObject& object);

private:
  FCesiumGeoJsonObject _object;
  CesiumVectorData::ConstGeoJsonPointIterator _iterator;

  friend class UCesiumGeoJsonPointIteratorFunctionLibrary;
};

UCLASS()
class UCesiumGeoJsonPointIteratorFunctionLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Creates an iterator over the GeoJSON object that will return any point
   * values in the object and any of its children.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Object")
  static FCesiumGeoJsonPointIterator
  Iterate(const FCesiumGeoJsonObject& Object);

  /**
   * Moves the iterator to the next available point value and returns that
   * point. If no more points are available, a zero vector is returned.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Iterator")
  static FVector Next(UPARAM(Ref) FCesiumGeoJsonPointIterator& Iterator);

  /**
   * Checks if this iterator has ended (no further points available).
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Iterator")
  static bool IsEnded(const FCesiumGeoJsonPointIterator& Iterator);
};

/**
 * Iterates over every LineString value in a GeoJSON object and all of its
 * children.
 */
USTRUCT(BlueprintType)
struct FCesiumGeoJsonLineStringIterator {
  GENERATED_BODY()

  /** @brief Creates an iterator that will return no line strings. */
  FCesiumGeoJsonLineStringIterator();

  /** @brief Creates a new iterator to iterate over the given object. */
  FCesiumGeoJsonLineStringIterator(const FCesiumGeoJsonObject& object);

private:
  FCesiumGeoJsonObject _object;
  CesiumVectorData::ConstGeoJsonLineStringIterator _iterator;

  friend class UCesiumGeoJsonLineStringIteratorFunctionLibrary;
};

UCLASS()
class UCesiumGeoJsonLineStringIteratorFunctionLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Creates an iterator over the GeoJSON object that will return any line
   * string values in the object and any of its children.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Object")
  static FCesiumGeoJsonLineStringIterator
  Iterate(const FCesiumGeoJsonObject& Object);

  /**
   * Moves the iterator to the next available line string value and returns that
   * line string. If no more line strings are available, an empty line is
   * returned.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Iterator")
  static FCesiumGeoJsonLineString
  Next(UPARAM(Ref) FCesiumGeoJsonLineStringIterator& Iterator);

  /**
   * Checks if this iterator has ended (no further line strings available).
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Iterator")
  static bool IsEnded(const FCesiumGeoJsonLineStringIterator& Iterator);
};

/**
 * Iterates over every Polygon value in a GeoJSON object and all of its
 * children.
 */
USTRUCT(BlueprintType)
struct FCesiumGeoJsonPolygonIterator {
  GENERATED_BODY()

  /** @brief Creates an iterator that will return no polygons. */
  FCesiumGeoJsonPolygonIterator();

  /** @brief Creates a new iterator to iterate over the given object. */
  FCesiumGeoJsonPolygonIterator(const FCesiumGeoJsonObject& object);

private:
  FCesiumGeoJsonObject _object;
  CesiumVectorData::ConstGeoJsonPolygonIterator _iterator;

  friend class UCesiumGeoJsonPolygonIteratorFunctionLibrary;
};

UCLASS()
class UCesiumGeoJsonPolygonIteratorFunctionLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Creates an iterator over the GeoJSON object that will return any polygon
   * values in the object and any of its children.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Object")
  static FCesiumGeoJsonPolygonIterator
  Iterate(const FCesiumGeoJsonObject& Object);

  /**
   * Moves the iterator to the next available polygon value and returns that
   * polygon. If no more polygons are available, an empty polygon is
   * returned.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Iterator")
  static FCesiumGeoJsonPolygon
  Next(UPARAM(Ref) FCesiumGeoJsonPolygonIterator& Iterator);

  /**
   * Checks if this iterator has ended (no further polygons available).
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Iterator")
  static bool IsEnded(const FCesiumGeoJsonPolygonIterator& Iterator);
};

// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumUtility/IntrusivePointer.h"
#include "CesiumVectorData/VectorDocument.h"
#include "CesiumVectorData/VectorNode.h"
#include "JsonObjectWrapper.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SharedPointer.h"

#include "CesiumVectorNode.generated.h"

/**
 * @brief A single node in the vector document.
 *
 * A node typically contains geometry along with metadata attached to that
 * geometry.
 */
USTRUCT(BlueprintType)
struct FCesiumVectorNode {
  GENERATED_BODY()

  /**
   * @brief Creates a new `FCesiumVectorNode` containing an empty vector node.
   */
  FCesiumVectorNode() : _document(nullptr), _node(nullptr) {}

  /**
   * @brief Creates a new `FCesiumVectorNode` wrapping the provided
   * `CesiumVectorData::VectorNode`.
   */
  FCesiumVectorNode(
      const CesiumUtility::IntrusivePointer<CesiumVectorData::VectorDocument>&
          doc,
      const CesiumVectorData::VectorNode* node)
      : _document(doc), _node(node) {}

private:
  CesiumUtility::IntrusivePointer<CesiumVectorData::VectorDocument> _document;
  const CesiumVectorData::VectorNode* _node;

  friend class UCesiumVectorNodeBlueprintLibrary;
};

/**
 * @brief The supported types of vector data geometry primitive.
 */
UENUM(BlueprintType)
enum class ECesiumVectorPrimitiveType : uint8 {
  /** @brief The Point primitive represents a single point in space. */
  Point = 0,
  /**
   * @brief The Line primitive represents a series of one or more line
   * segments.
   */
  Line = 1,
  /**
   * @brief The Polygon primitive represents a polygon made up of one or more
   * linear rings.
   */
  Polygon = 2
};

/**
 * @brief A single geometry primitive. The type of the primitive is represented
 * by `ECesiumVectorPrimitiveType`.
 */
USTRUCT(BlueprintType)
struct FCesiumVectorPrimitive {
  GENERATED_BODY()

  /** @brief Creates a new `FCesiumVectorPrimitive` with an empty primitive. */
  FCesiumVectorPrimitive() : _document(nullptr), _primitive(nullptr) {}

  /**
   * @brief Creates a new `FCesiumVectorPrimitive` wrapping the provided
   * `CesiumVectorData::VectorPrimitive`.
   */
  FCesiumVectorPrimitive(
      const CesiumUtility::IntrusivePointer<CesiumVectorData::VectorDocument>&
          document,
      const CesiumVectorData::VectorPrimitive* primitive)
      : _document(document), _primitive(primitive) {}

private:
  CesiumUtility::IntrusivePointer<CesiumVectorData::VectorDocument> _document;
  const CesiumVectorData::VectorPrimitive* _primitive;

  friend class UCesiumVectorPrimitiveBlueprintLibrary;
};

/**
 * @brief A Blueprint Funciton Library for interacting with `FCesiumVectorNode`
 * values.
 */
UCLASS()
class UCesiumVectorNodeBlueprintLibrary : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Returns the ID of the provided vector node, or -1 if no ID was
   * present or if the ID is not an integer.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Node")
  static int64 GetIdAsInteger(const FCesiumVectorNode& InVectorNode);

  /**
   * @brief Returns the ID of the provided vector node, or an empty string if no
   * ID was present. If the ID is an integer, it will be converted to a string.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Node")
  static FString GetIdAsString(const FCesiumVectorNode& InVectorNode);

  /**
   * @brief Returns any child nodes of this vector node.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Node")
  static TArray<FCesiumVectorNode>
  GetChildren(const FCesiumVectorNode& InVectorNode);

  /**
   * @brief Obtains the properties attached to this node, if any.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Node")
  static FJsonObjectWrapper
  GetProperties(const FCesiumVectorNode& InVectorNode);

  /**
   * @brief Returns an array of primitives contained in this node.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector|Node")
  static TArray<FCesiumVectorPrimitive>
  GetPrimitives(const FCesiumVectorNode& InVectorNode);

  /**
   * @brief Returns all primitives of the given type from this node.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Node")
  static TArray<FCesiumVectorPrimitive> GetPrimitivesOfType(
      const FCesiumVectorNode& InVectorNode,
      ECesiumVectorPrimitiveType InType);

  /**
   * @brief Returns all primitives of the given type in this node or in any
   * child nodes, recursively.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|Vector|Node",
      meta = (DisplayName = "Get All Primitives of Type"))
  static TArray<FCesiumVectorPrimitive> GetPrimitivesOfTypeRecursively(
      const FCesiumVectorNode& InVectorNode,
      ECesiumVectorPrimitiveType InType);

  /**
   * @brief Returns the first child node found with the given ID. If no node was
   * found, "Success" will return false.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|Vector|Node",
      meta = (DisplayName = "Find Node By String ID"))
  static UPARAM(DisplayName = "Success") bool FindNodeByStringId(
      const FCesiumVectorNode& InVectorNode,
      const FString& InNodeId,
      FCesiumVectorNode& OutNode);

  /**
   * @brief Returns the first child node found with the given ID. If no node was
   * found, "Success" will return false.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium|Vector|Node",
      meta = (DisplayName = "Find Node By Integer ID"))
  static UPARAM(DisplayName = "Success") bool FindNodeByIntId(
      const FCesiumVectorNode& InVectorNode,
      int64 InNodeId,
      FCesiumVectorNode& OutNode);
};

/**
 * @brief A `FCesiumCompositeCartographicPolygon` is a polygon made up of one or
 * more linear rings.
 */
USTRUCT(BlueprintType)
struct FCesiumCompositeCartographicPolygon {
  GENERATED_BODY()

  /**
   * @brief Creates a new `FCesiumCompositeCartographicPolygon` with an empty
   * composite polygon.
   */
  FCesiumCompositeCartographicPolygon() : _polygon({}) {}

  /**
   * @brief Creates a new `FCesiumCompositeCartographicPolygon` wrapping the
   * provided `CesiumGeospatial::CompositeCartographicPolygon`.
   */
  FCesiumCompositeCartographicPolygon(
      const CesiumGeospatial::CompositeCartographicPolygon& polygon)
      : _polygon(polygon) {}

private:
  CesiumGeospatial::CompositeCartographicPolygon _polygon;

  friend class UCesiumCompositeCartographicPolygonBlueprintLibrary;
};

/**
 * @brief A `FCesiumPolygonLinearRing` is a single linear ring of a
 * `FCesiumCompositeCartographicPolygon`.
 */
USTRUCT(BlueprintType)
struct FCesiumPolygonLinearRing {
  GENERATED_BODY()
public:
  /**
   * @brief Creates a new `FCesiumPolygonLinearRing` with an empty linear ring.
   */
  FCesiumPolygonLinearRing() : Points() {}

  /**
   * @brief Creates a new `FCesiumPolygonLinearRing` from a set of
   * Longitude-Latitude-Height points.
   */
  FCesiumPolygonLinearRing(TArray<FVector>&& InPoints);

  /**
   * @brief The Longitude-Latitude-Height points of this polygon.
   */
  UPROPERTY(BlueprintReadOnly, Category = "Cesium|Vector|Polygon")
  TArray<FVector> Points;
};

UCLASS()
class UCesiumCompositeCartographicPolygonBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()
public:
  /**
   * @brief Returns whether this `FCesiumCompositeCartographicPolygon` contains
   * the provided Longitude-Latitude-Height position.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Polygon")
  static bool PolygonContainsPoint(
      const FCesiumCompositeCartographicPolygon& InPolygon,
      const FVector& InPoint);

  /**
   * @brief Returns the linear rings that make up this composite polygon.
   *
   * The first returned ring represents the outer bounds of the polygon. Any
   * additional rings define holes within those bounds.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Polygon")
  static TArray<FCesiumPolygonLinearRing>
  GetPolygonRings(const FCesiumCompositeCartographicPolygon& InPolygon);
};

/**
 * @brief A Blueprint Funciton Library for interacting with
 * `FCesiumVectorPrimitive` values.
 */
UCLASS()
class UCesiumVectorPrimitiveBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()
public:
  /**
   * @brief Returns the `ECesiumVectorPrimitiveType` of this
   * `FCesiumVectorPrimitive`.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Vector|Primitive")
  static ECesiumVectorPrimitiveType
  GetPrimitiveType(const FCesiumVectorPrimitive& InPrimitive);

  /**
   * @brief Assuming this primitive is a `ECesiumVectorPrimitiveType::Point`,
   * returns the point value. If it is not a point, returns `FVector(0, 0, 0)`.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Primitive")
  static FVector GetPrimitiveAsPoint(const FCesiumVectorPrimitive& InPrimitive);

  /**
   * @brief Assuming this primitive is a `ECesiumVectorPrimitiveType::Line`,
   * returns the points defining the line segments. If it is not a line, returns
   * an empty `TArray<FVector>`.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Primitive")
  static TArray<FVector>
  GetPrimitiveAsLine(const FCesiumVectorPrimitive& InPrimitive);

  /**
   * @brife Assuming this primitive is a `ECesiumVectorPrimitiveType::Polygon`,
   * returns the `FCesiumCompositeCartographicPolygon`. If it is not a polygon,
   * returns a default-constructed `FCesiumCompositeCartographicPolygon`.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector|Primitive")
  static FCesiumCompositeCartographicPolygon
  GetPrimitiveAsPolygon(const FCesiumVectorPrimitive& InPrimitive);
};

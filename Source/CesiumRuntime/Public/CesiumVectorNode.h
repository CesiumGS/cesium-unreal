// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumVectorData/VectorNode.h"

#include "CesiumVectorNode.generated.h"

USTRUCT(BlueprintType)
struct FCesiumVectorNode {
  GENERATED_BODY()

  FCesiumVectorNode(const CesiumVectorData::VectorNode& node) : _node(node) {}

private:
  CesiumVectorData::VectorNode _node;

  friend class UCesiumVectorNodeBlueprintLibrary;
};

UENUM(BlueprintType)
enum class ECesiumVectorPrimitiveType : uint8 {
  Point = 0,
  Line = 1,
  Polygon = 2
};

USTRUCT(BlueprintType)
struct FCesiumVectorPrimitive {
  GENERATED_BODY()

  FCesiumVectorPrimitive(const CesiumVectorData::VectorPrimitive& primitive)
      : _primitive(primitive) {}

private:
  CesiumVectorData::VectorPrimitive _primitive;

  friend class UCesiumVectorPrimitiveBlueprintLibrary;
};

UCLASS()
class UCesiumVectorNodeBlueprintLibrary : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * @brief Returns the ID of the provided vector node, or -1 if no ID was
   * present or if the ID is not an integer.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector")
  static int64 GetIdAsInteger(const FCesiumVectorNode& InVectorNode);

  /**
   * @brief Returns the ID of the provided vector node, or an empty string if no
   * ID was present. If the ID is an integer, it will be converted to a string.
   */
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector")
  static FString GetIdAsString(const FCesiumVectorNode& InVectorNode);

  /**
   * @brief Returns an array of primitives contained in this node.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector")
  static TArray<FCesiumVectorPrimitive>
  GetPrimitives(const FCesiumVectorNode& InVectorNode);

  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector")
  static TArray<FCesiumVectorPrimitive> GetPrimitivesOfType(
      const FCesiumVectorNode& InVectorNode,
      ECesiumVectorPrimitiveType InType);
};

USTRUCT(BlueprintType)
struct FCesiumCompositeCartographicPolygon {
  GENERATED_BODY()

  FCesiumCompositeCartographicPolygon(
      const CesiumGeospatial::CompositeCartographicPolygon& polygon)
      : _polygon(polygon) {}

private:
  CesiumGeospatial::CompositeCartographicPolygon _polygon;
};

UCLASS()
class UCesiumVectorPrimitiveBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()
public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Vector")
  static ECesiumVectorPrimitiveType
  GetPrimitiveType(const FCesiumVectorPrimitive& InPrimitive);

  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector")
  static FVector GetPrimitiveAsPoint(const FCesiumVectorPrimitive& InPrimitive);

  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector")
  static TArray<FVector>
  GetPrimitiveAsLine(const FCesiumVectorPrimitive& InPrimitive);

  UFUNCTION(BlueprintCallable, Category = "Cesium|Vector")
  static FCesiumCompositeCartographicPolygon
  GetPrimitiveAsPolygon(const FCesiumVectorPrimitive& InPrimitive);
};

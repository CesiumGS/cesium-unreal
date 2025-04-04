#include "CesiumVectorNode.h"

#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeospatial/CompositeCartographicPolygon.h"

#include <utility>
#include <variant>
#include <vector>

int64 UCesiumVectorNodeBlueprintLibrary::GetIdAsInteger(
    const FCesiumVectorNode& InVectorNode) {
  const int64_t* pId = std::get_if<int64_t>(&InVectorNode._node.id);
  if (!pId) {
    return -1;
  }

  return *pId;
}

FString UCesiumVectorNodeBlueprintLibrary::GetIdAsString(
    const FCesiumVectorNode& InVectorNode) {
  struct GetIdVisitor {
    FString operator()(const int64_t& id) { return FString::FromInt(id); }
    FString operator()(const std::string& id) {
      return UTF8_TO_TCHAR(id.c_str());
    }
    FString operator()(const std::monostate& id) { return FString(); }
  };

  return std::visit(GetIdVisitor{}, InVectorNode._node.id);
}

TArray<FCesiumVectorPrimitive> UCesiumVectorNodeBlueprintLibrary::GetPrimitives(
    const FCesiumVectorNode& InVectorNode) {
  TArray<FCesiumVectorPrimitive> primitives;
  primitives.Reserve(InVectorNode._node.primitives.size());
  for (const CesiumVectorData::VectorPrimitive& primitive :
       InVectorNode._node.primitives) {
    primitives.Emplace(primitive);
  }
  return primitives;
}

TArray<FCesiumVectorPrimitive>
UCesiumVectorNodeBlueprintLibrary::GetPrimitivesOfType(
    const FCesiumVectorNode& InVectorNode,
    ECesiumVectorPrimitiveType InType) {
  struct CheckTypeVisitor {
    ECesiumVectorPrimitiveType IntendedType;
    bool operator()(const CesiumGeospatial::Cartographic&) {
      return IntendedType == ECesiumVectorPrimitiveType::Point;
    }
    bool operator()(const std::vector<CesiumGeospatial::Cartographic>&) {
      return IntendedType == ECesiumVectorPrimitiveType::Line;
    }
    bool operator()(const CesiumGeospatial::CompositeCartographicPolygon&) {
      return IntendedType == ECesiumVectorPrimitiveType::Polygon;
    }
  };

  TArray<FCesiumVectorPrimitive> primitives;
  for (const CesiumVectorData::VectorPrimitive& primitive :
       InVectorNode._node.primitives) {
    if (std::visit(CheckTypeVisitor{InType}, primitive)) {
      primitives.Emplace(primitive);
    }
  }

  return primitives;
}

ECesiumVectorPrimitiveType
UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveType(
    const FCesiumVectorPrimitive& InPrimitive) {
  struct GetTypeVisitor {
    ECesiumVectorPrimitiveType
    operator()(const CesiumGeospatial::Cartographic&) {
      return ECesiumVectorPrimitiveType::Point;
    }
    ECesiumVectorPrimitiveType
    operator()(const std::vector<CesiumGeospatial::Cartographic>&) {
      return ECesiumVectorPrimitiveType::Line;
    }
    ECesiumVectorPrimitiveType
    operator()(const CesiumGeospatial::CompositeCartographicPolygon&) {
      return ECesiumVectorPrimitiveType::Polygon;
    }
  };

  return std::visit(GetTypeVisitor{}, InPrimitive._primitive);
}

FVector UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsPoint(
    const FCesiumVectorPrimitive& InPrimitive) {
  const CesiumGeospatial::Cartographic* pCartographic =
      std::get_if<CesiumGeospatial::Cartographic>(&InPrimitive._primitive);
  if (!pCartographic) {
    return FVector::ZeroVector;
  }

  return FVector(
      pCartographic->longitude,
      pCartographic->latitude,
      pCartographic->height);
}

TArray<FVector> UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsLine(
    const FCesiumVectorPrimitive& InPrimitive) {
  const std::vector<CesiumGeospatial::Cartographic>* pCartographicLine =
      std::get_if<std::vector<CesiumGeospatial::Cartographic>>(
          &InPrimitive._primitive);
  if (!pCartographicLine) {
    return TArray<FVector>();
  }

  TArray<FVector> line;
  line.Reserve(pCartographicLine->size());
  for (const CesiumGeospatial::Cartographic& point : *pCartographicLine) {
    line.Emplace(point.longitude, point.latitude, point.height);
  }

  return line;
}

FCesiumCompositeCartographicPolygon
UCesiumVectorPrimitiveBlueprintLibrary::GetPrimitiveAsPolygon(
    const FCesiumVectorPrimitive& InPrimitive) {
  const CesiumGeospatial::CompositeCartographicPolygon* pPolygon =
      std::get_if<CesiumGeospatial::CompositeCartographicPolygon>(
          &InPrimitive._primitive);
  if (!pPolygon) {
    return FCesiumCompositeCartographicPolygon(
        CesiumGeospatial::CompositeCartographicPolygon({}));
  }

  return FCesiumCompositeCartographicPolygon(*pPolygon);
}

// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumVoxelRendererComponent.h"
#include "CalcBounds.h"
#include "Cesium3DTileset.h"
#include "CesiumLifetime.h"
#include "CesiumMaterialUserData.h"
#include "CesiumRuntime.h"
#include "CesiumVoxelMetadataComponent.h"
#include "EncodedFeaturesMetadata.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/ConstructorHelpers.h"
#include "VecMath.h"

#include <Cesium3DTiles/ExtensionContent3dTilesContentVoxels.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <CesiumUtility/Math.h>
#include <variant>

// Sets default values for this component's properties
UCesiumVoxelRendererComponent::UCesiumVoxelRendererComponent()
    : USceneComponent() {
  // Structure to hold one-time initialization
  struct FConstructorStatics {
    ConstructorHelpers::FObjectFinder<UMaterialInstance> DefaultMaterial;
    ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh;
    FConstructorStatics()
        : DefaultMaterial(TEXT(
              "/CesiumForUnreal/Materials/Instances/MI_CesiumVoxel.MI_CesiumVoxel")),
          CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube")) {}
  };
  static FConstructorStatics ConstructorStatics;

  this->DefaultMaterial = ConstructorStatics.DefaultMaterial.Object;
  this->CubeMesh = ConstructorStatics.CubeMesh.Object;
  this->CubeMesh->NeverStream = true;

  PrimaryComponentTick.bCanEverTick = false;
}

UCesiumVoxelRendererComponent::~UCesiumVoxelRendererComponent() {}

void UCesiumVoxelRendererComponent::BeginDestroy() {
  if (this->MeshComponent) {
    // Only handle the destruction of the material instance. The
    // UStaticMeshComponent attached to this component will be destroyed by the
    // call to destroyComponentRecursively in Cesium3DTileset.cpp.
    UMaterialInstanceDynamic* pMaterial =
        Cast<UMaterialInstanceDynamic>(MeshComponent->GetMaterial(0));
    if (pMaterial) {
      CesiumLifetime::destroy(pMaterial);
    }
  }

  // Reset the pointers.
  this->MeshComponent = nullptr;
  this->_pResources.Reset();

  Super::BeginDestroy();
}

namespace {
EVoxelGridShape getVoxelGridShape(
    const Cesium3DTilesSelection::BoundingVolume& boundingVolume) {
  if (std::get_if<CesiumGeometry::OrientedBoundingBox>(&boundingVolume)) {
    return EVoxelGridShape::Box;
  }
  if (std::get_if<CesiumGeometry::BoundingCylinderRegion>(&boundingVolume)) {
    return EVoxelGridShape::Cylinder;
  }
  if (std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume)) {
    return EVoxelGridShape::Ellipsoid;
  }

  return EVoxelGridShape::Invalid;
}

void setVoxelBoxProperties(
    UCesiumVoxelRendererComponent* pVoxelComponent,
    UMaterialInstanceDynamic* pVoxelMaterial,
    const CesiumGeometry::OrientedBoundingBox& box) {
  glm::dmat3 halfAxes = box.getHalfAxes();
  pVoxelComponent->HighPrecisionTransform = glm::dmat4(
      glm::dvec4(halfAxes[0], 0) * 0.02,
      glm::dvec4(halfAxes[1], 0) * 0.02,
      glm::dvec4(halfAxes[2], 0) * 0.02,
      glm::dvec4(box.getCenter(), 1));

  // The transform and scale of the box are handled in the component's
  // transform, so there is no need to duplicate it here. Instead, this
  // transform is configured to scale the engine-provided Cube ([-50, 50]) to
  // unit space ([-1, 1]).
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 0"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(0.02, 0, 0, 0));
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 1"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(0, 0.02, 0, 0));
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 2"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(0, 0, 0.02, 0));
}

void setVoxelCylinderProperties(
    UCesiumVoxelRendererComponent* pVoxelComponent,
    UMaterialInstanceDynamic* pVoxelMaterial,
    const CesiumGeometry::BoundingCylinderRegion& cylinder) {
  // Approximate the cylinder region as a box.
  const CesiumGeometry::OrientedBoundingBox& box =
      cylinder.toOrientedBoundingBox();

  glm::dmat3 halfAxes = box.getHalfAxes();
  pVoxelComponent->HighPrecisionTransform = glm::dmat4(
      glm::dvec4(halfAxes[0], 0) * 0.02,
      glm::dvec4(halfAxes[1], 0) * 0.02,
      glm::dvec4(halfAxes[2], 0) * 0.02,
      glm::dvec4(box.getCenter(), 1));

  // For now, only the height bounds and maximum radius are used.
  // The angle will be relevant when clipping.
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Min Bounds"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector(0, -CesiumUtility::Math::OnePi, -1));
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Max Bounds"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector(1, CesiumUtility::Math::OnePi, 1));

  // The transform and scale of the cylinder are handled in the component's
  // transform, so there is no need to duplicate it here. Instead, this
  // transform is configured to scale the engine-provided Cube ([-50, 50]) to
  // unit space ([-1, 1]).
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 0"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(0.02, 0, 0, 0));
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 1"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(0, 0.02, 0, 0));
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 2"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(0, 0, 0.02, 0));
}

/**
 * @brief Describes the quality of a radian value relative to the axis it is
 * defined in. This determines the math for the ray-intersection tested against
 * that value in the voxel shader.
 */
enum CartographicAngleDescription : int8 {
  None = 0,
  Zero = 1,
  UnderHalf = 2,
  Half = 3,
  OverHalf = 4
};

CartographicAngleDescription interpretLongitudeRange(double value) {
  const double longitudeEpsilon = CesiumUtility::Math::Epsilon10;

  if (value >= CesiumUtility::Math::OnePi - longitudeEpsilon &&
      value < CesiumUtility::Math::TwoPi - longitudeEpsilon) {
    // longitude range > PI
    return CartographicAngleDescription::OverHalf;
  }
  if (value > longitudeEpsilon &&
      value < CesiumUtility::Math::OnePi - longitudeEpsilon) {
    // longitude range < PI
    return CartographicAngleDescription::UnderHalf;
  }
  if (value < longitudeEpsilon) {
    // longitude range ~= 0
    return CartographicAngleDescription::Zero;
  }

  return CartographicAngleDescription::None;
}

CartographicAngleDescription interpretLatitudeValue(double value) {
  const double latitudeEpsilon = CesiumUtility::Math::Epsilon10;
  const double zeroLatitudeEpsilon =
      CesiumUtility::Math::Epsilon3; // 0.001 radians = 0.05729578 degrees

  if (value > -CesiumUtility::Math::OnePi + latitudeEpsilon &&
      value < -zeroLatitudeEpsilon) {
    // latitude between (-PI, 0)
    return CartographicAngleDescription::UnderHalf;
  }
  if (value >= -zeroLatitudeEpsilon && value <= +zeroLatitudeEpsilon) {
    // latitude ~= 0
    return CartographicAngleDescription::Half;
  }
  if (value > zeroLatitudeEpsilon) {
    // latitude between (0, PI)
    return CartographicAngleDescription::OverHalf;
  }

  return CartographicAngleDescription::None;
}

FVector getEllipsoidRadii(const ACesiumGeoreference* pGeoreference) {
  FVector radii =
      VecMath::createVector(CesiumGeospatial::Ellipsoid::WGS84.getRadii());
  if (pGeoreference) {
    UCesiumEllipsoid* pEllipsoid = pGeoreference->GetEllipsoid();
    radii = pEllipsoid ? pEllipsoid->GetRadii() : radii;
  }

  return radii;
}

void setVoxelEllipsoidProperties(
    UCesiumVoxelRendererComponent* pVoxelComponent,
    UMaterialInstanceDynamic* pVoxelMaterial,
    const CesiumGeospatial::BoundingRegion& region,
    ACesium3DTileset* pTileset) {
  // Although the ellipsoid corresponds to the size & location of the Earth, the
  // cube is scaled to fit the region, which may be much smaller. This prevents
  // unnecessary pixels from running the voxel raymarching shader.
  const CesiumGeometry::OrientedBoundingBox box = region.getBoundingBox();
  glm::dmat3 halfAxes = box.getHalfAxes();
  pVoxelComponent->HighPrecisionTransform = glm::dmat4(
      glm::dvec4(halfAxes[0], 0) * 0.02,
      glm::dvec4(halfAxes[1], 0) * 0.02,
      glm::dvec4(halfAxes[2], 0) * 0.02,
      glm::dvec4(box.getCenter(), 1));

  FVector radii = getEllipsoidRadii(pTileset->ResolveGeoreference());
  // The default bounds define the minimum and maximum extents for the shape's
  // actual bounds, in the order of (longitude, latitude, height). The longitude
  // and latitude bounds describe the angular range of the full ellipsoid.
  const FVector defaultMinimumBounds(
      -CesiumUtility::Math::OnePi,
      -CesiumUtility::Math::PiOverTwo,
      -radii.GetMin());
  const FVector defaultMaximumBounds(
      CesiumUtility::Math::OnePi,
      CesiumUtility::Math::PiOverTwo,
      10 * radii.GetMin());

  const CesiumGeospatial::GlobeRectangle& rectangle = region.getRectangle();
  double minimumLongitude = rectangle.getWest();
  double maximumLongitude = rectangle.getEast();
  double minimumLatitude = rectangle.getSouth();
  double maximumLatitude = rectangle.getNorth();

  // Don't let the minimum height extend past the center of the Earth.
  double minimumHeight =
      glm::max(region.getMinimumHeight(), defaultMinimumBounds.Z);
  double maximumHeight = region.getMaximumHeight();

  // Defines the extents of the longitude in UV space. In other words, this
  // expresses the minimum, maximum, and midpoint values of the longitude range
  // in UV space.
  FVector longitudeUVExtents = FVector::Zero();
  double longitudeUVScale = 1;
  double longitudeUVOffset = 0;

  FIntVector4 longitudeFlags(0);

  // Longitude
  {
    const double defaultRange = CesiumUtility::Math::TwoPi;
    bool isLongitudeReversed = maximumLongitude < minimumLongitude;
    double longitudeRange = maximumLongitude - minimumLongitude +
                            isLongitudeReversed * defaultRange;

    // Refers to the discontinuity at longitude 0 / 2pi.
    const double discontinuityEpsilon =
        CesiumUtility::Math::Epsilon3; // 0.001 radians = 0.05729578 degrees
    bool longitudeMinimumHasDiscontinuity = CesiumUtility::Math::equalsEpsilon(
        minimumLongitude,
        0,
        discontinuityEpsilon);
    bool longitudeMaximumHasDiscontinuity = CesiumUtility::Math::equalsEpsilon(
        maximumLongitude,
        CesiumUtility::Math::TwoPi,
        discontinuityEpsilon);

    CartographicAngleDescription longitudeRangeIndicator =
        interpretLongitudeRange(longitudeRange);

    longitudeFlags.X = longitudeRangeIndicator;
    longitudeFlags.Y = longitudeMinimumHasDiscontinuity;
    longitudeFlags.Z = longitudeMaximumHasDiscontinuity;
    longitudeFlags.W = isLongitudeReversed;

    // Compute the extents of the longitude range in UV Shape Space.
    double minimumLongitudeUV =
        (minimumLongitude - defaultMinimumBounds.X) / defaultRange;
    double maximumLongitudeUV =
        (maximumLongitude - defaultMinimumBounds.X) / defaultRange;
    // Given a longitude range, represents the actual value where "0" would be
    // in UV coordinates.
    double longitudeRangeUVZero = 1.0 - longitudeRange / defaultRange;
    // TODO: document this
    double longitudeRangeUVZeroMid =
        glm::fract(maximumLongitudeUV + 0.5 * longitudeRangeUVZero);

    longitudeUVExtents = FVector(
        minimumLongitudeUV,
        maximumLongitudeUV,
        longitudeRangeUVZeroMid);

    const double longitudeEpsilon = CesiumUtility::Math::Epsilon10;
    if (longitudeRange > longitudeEpsilon) {
      longitudeUVScale = defaultRange / longitudeRange;
      longitudeUVOffset =
          -(minimumLongitude - defaultMinimumBounds.X) / longitudeRange;
    }
  }

  // Latitude
  CartographicAngleDescription latitudeMinValueFlag =
      interpretLatitudeValue(minimumLatitude);
  CartographicAngleDescription latitudeMaxValueFlag =
      interpretLatitudeValue(maximumLatitude);
  double latitudeUVScale = 1;
  double latitudeUVOffset = 0;

  {
    const double latitudeEpsilon = CesiumUtility::Math::Epsilon10;
    const double defaultRange = CesiumUtility::Math::OnePi;
    double latitudeRange = maximumLatitude - minimumLatitude;
    if (latitudeRange >= latitudeEpsilon) {
      latitudeUVScale = defaultRange / latitudeRange;
      latitudeUVOffset =
          (defaultMinimumBounds.Y - minimumLatitude) / latitudeRange;
    }
  }

  // Compute the farthest a point can be from the center of the ellipsoid.
  const FVector outerExtent = radii + maximumHeight;
  const double maximumExtent = outerExtent.GetMax();

  const FVector radiiUV = outerExtent / maximumExtent;
  const double axisRatio = radiiUV.Z / radiiUV.X;
  const double eccentricitySquared = 1.0 - axisRatio * axisRatio;
  const FVector2D evoluteScale(
      (radiiUV.X * radiiUV.X - radiiUV.Z * radiiUV.Z) / radiiUV.X,
      (radiiUV.Z * radiiUV.Z - radiiUV.X * radiiUV.X) / radiiUV.Z);

  // Used to compute geodetic surface normal.
  const FVector inverseRadiiSquaredUV = FVector::One() / (radiiUV * radiiUV);
  // The percent of space that is between the inner and outer ellipsoid.
  const double thickness = (maximumHeight - minimumHeight) / maximumExtent;
  const double inverseHeightDifferenceUV =
      maximumHeight != minimumHeight ? 1.0 / thickness : 0;

  // Ray-intersection math for latitude requires sin(latitude).
  // The actual latitude values aren't used by other parts of the shader, so
  // passing sin(latitude) here saves space.
  // Shape Min Bounds = Region Min (xyz)
  // X = longitude, Y = sin(latitude), Z = height relative to the maximum extent
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Min Bounds"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector(
          minimumLongitude,
          FMath::Sin(minimumLatitude),
          (minimumHeight - maximumHeight) / maximumExtent));

  // Shape Max Bounds = Region Max (xyz)
  // X = longitude, Y = sin(latitude), Z = height relative to the maximum extent
  // Since clipping isn't supported, Z resolves to 0.
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Max Bounds"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector(maximumLongitude, FMath::Sin(maximumLatitude), 0));

  // Data is packed across multiple vec4s to conserve space.
  // 0 = Longitude Range Flags (xyzw)
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Packed Data 0"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(longitudeFlags));
  // 1 = Min Latitude Flag (x), Max Latitude Flag (y), Evolute Scale (zw)
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Packed Data 1"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(
          latitudeMinValueFlag,
          latitudeMaxValueFlag,
          evoluteScale.X,
          evoluteScale.Y));
  // 2 = Radii UV (xyz)
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Packed Data 2"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(radiiUV, 0));
  // 3 = Inverse Radii UV Squared (xyz), Inverse Height Difference UV (w)
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Packed Data 3"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(inverseRadiiSquaredUV, inverseHeightDifferenceUV));
  // 4 = Longitude UV extents (xyz), Eccentricity Squared (w)
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Packed Data 4"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(longitudeUVExtents, eccentricitySquared));
  // 5 = UV -> Shape UV Transforms (scale / offset)
  // Longitude (xy), Latitude (zw)
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Packed Data 5"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(
          longitudeUVScale,
          longitudeUVOffset,
          latitudeUVScale,
          latitudeUVOffset));
}

FCesiumMetadataValue
getMetadataValue(const std::optional<CesiumUtility::JsonValue>& jsonValue) {
  if (!jsonValue)
    return FCesiumMetadataValue();

  if (jsonValue->isArray()) {
    CesiumUtility::JsonValue::Array array = jsonValue->getArray();
    if (array.size() == 0 || array.size() > 4) {
      return FCesiumMetadataValue();
    }

    // Attempt to convert the array to a vec4 (or a value with less dimensions).
    size_t endIndex = FMath::Min(array.size(), (size_t)4);
    TArray<float> values;
    for (size_t i = 0; i < endIndex; i++) {
      values.Add(UCesiumMetadataValueBlueprintLibrary::GetFloat(
          getMetadataValue(array[i]),
          0.0f));
    }

    switch (values.Num()) {
    case 1:
      return FCesiumMetadataValue(values[0]);
    case 2:
      return FCesiumMetadataValue(glm::vec2(values[0], values[1]));
    case 3:
      return FCesiumMetadataValue(glm::vec3(values[0], values[1], values[2]));
    case 4:
      return FCesiumMetadataValue(
          glm::vec4(values[0], values[1], values[2], values[3]));
    default:
      return FCesiumMetadataValue();
    }
  }

  if (jsonValue->isInt64()) {
    return FCesiumMetadataValue(jsonValue->getInt64OrDefault(0));
  }

  if (jsonValue->isUint64()) {
    return FCesiumMetadataValue(jsonValue->getUint64OrDefault(0));
  }

  if (jsonValue->isDouble()) {
    return FCesiumMetadataValue(jsonValue->getDoubleOrDefault(0.0));
  }

  return FCesiumMetadataValue();
}

// uint32 getMaximumTextureMemory(
//     const FCesiumVoxelClassDescription* pDescription,
//     const glm::uvec3& gridDimensions,
//     uint64_t tileCount) {
//   int32_t pixelSize = 0;
//
//   if (pDescription) {
//     for (const FCesiumPropertyAttributePropertyDescription& Property :
//          pDescription->Properties) {
//       EncodedFeaturesMetadata::EncodedPixelFormat pixelFormat =
//           EncodedFeaturesMetadata::getPixelFormat(
//               Property.EncodingDetails.Type,
//               Property.EncodingDetails.ComponentType);
//       pixelSize = FMath::Max(
//           pixelSize,
//           pixelFormat.bytesPerChannel * pixelFormat.channels);
//     }
//   }
//
//   return (uint32)pixelSize * gridDimensions.x * gridDimensions.y *
//          gridDimensions.z * tileCount;
// }

} // namespace

/*static*/ UMaterialInstanceDynamic*
UCesiumVoxelRendererComponent::CreateVoxelMaterial(
    UCesiumVoxelRendererComponent* pVoxelComponent,
    const FVector& dimensions,
    const FVector& paddingBefore,
    const FVector& paddingAfter,
    ACesium3DTileset* pTilesetActor,
    const Cesium3DTiles::Class* pVoxelClass,
    const FCesiumVoxelClassDescription* pDescription,
    const Cesium3DTilesSelection::BoundingVolume& boundingVolume) {
  UMaterialInterface* pMaterial = pTilesetActor->GetMaterial();

  UMaterialInstanceDynamic* pVoxelMaterial = UMaterialInstanceDynamic::Create(
      pMaterial ? pMaterial : pVoxelComponent->DefaultMaterial,
      nullptr,
      FName("VoxelMaterial"));
  pVoxelMaterial->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

  EVoxelGridShape shape = pVoxelComponent->Options.gridShape;

  pVoxelMaterial->SetTextureParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Octree"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      pVoxelComponent->_pResources->GetOctreeTexture());
  pVoxelMaterial->SetScalarParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape Constant"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      uint8(shape));

  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Grid Dimensions"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      dimensions);
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Padding Before"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      paddingBefore);
  pVoxelMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Padding After"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      paddingAfter);

  if (shape == EVoxelGridShape::Box) {
    const CesiumGeometry::OrientedBoundingBox* pBox =
        std::get_if<CesiumGeometry::OrientedBoundingBox>(&boundingVolume);
    assert(pBox != nullptr);
    setVoxelBoxProperties(pVoxelComponent, pVoxelMaterial, *pBox);
  } else if (shape == EVoxelGridShape::Cylinder) {
    const CesiumGeometry::BoundingCylinderRegion* pCylinder =
        std::get_if<CesiumGeometry::BoundingCylinderRegion>(&boundingVolume);
    assert(pCylinder != nullptr);
    setVoxelCylinderProperties(pVoxelComponent, pVoxelMaterial, *pCylinder);
  } else if (shape == EVoxelGridShape::Ellipsoid) {
    const CesiumGeospatial::BoundingRegion* pRegion =
        std::get_if<CesiumGeospatial::BoundingRegion>(&boundingVolume);
    assert(pRegion != nullptr);
    setVoxelEllipsoidProperties(
        pVoxelComponent,
        pVoxelMaterial,
        *pRegion,
        pTilesetActor);
  }

  if (pDescription && pVoxelClass) {
    for (const auto propertyIt : pVoxelClass->properties) {
      FString UnrealName(propertyIt.first.c_str());

      for (const FCesiumPropertyAttributePropertyDescription& Property :
           pDescription->Properties) {
        if (Property.Name != UnrealName) {
          continue;
        }

        FString PropertyName =
            EncodedFeaturesMetadata::createHlslSafeName(Property.Name);

        pVoxelMaterial->SetTextureParameterValueByInfo(
            FMaterialParameterInfo(
                FName(PropertyName),
                EMaterialParameterAssociation::LayerParameter,
                0),
            pVoxelComponent->_pResources->GetDataTexture(Property.Name));

        if (Property.PropertyDetails.bHasScale) {
          EncodedFeaturesMetadata::SetPropertyParameterValue(
              pVoxelMaterial,
              EMaterialParameterAssociation::LayerParameter,
              0,
              PropertyName +
                  EncodedFeaturesMetadata::MaterialPropertyScaleSuffix,
              Property.EncodingDetails.Type,
              getMetadataValue(propertyIt.second.scale),
              1);
        }

        if (Property.PropertyDetails.bHasOffset) {
          EncodedFeaturesMetadata::SetPropertyParameterValue(
              pVoxelMaterial,
              EMaterialParameterAssociation::LayerParameter,
              0,
              PropertyName +
                  EncodedFeaturesMetadata::MaterialPropertyOffsetSuffix,
              Property.EncodingDetails.Type,
              getMetadataValue(propertyIt.second.offset),
              0);
        }

        if (Property.PropertyDetails.bHasNoDataValue) {
          EncodedFeaturesMetadata::SetPropertyParameterValue(
              pVoxelMaterial,
              EMaterialParameterAssociation::LayerParameter,
              0,
              PropertyName +
                  EncodedFeaturesMetadata::MaterialPropertyNoDataSuffix,
              Property.EncodingDetails.Type,
              getMetadataValue(propertyIt.second.noData),
              0);
        }

        if (Property.PropertyDetails.bHasDefaultValue) {
          EncodedFeaturesMetadata::SetPropertyParameterValue(
              pVoxelMaterial,
              EMaterialParameterAssociation::LayerParameter,
              0,
              PropertyName +
                  EncodedFeaturesMetadata::MaterialPropertyDefaultValueSuffix,
              Property.EncodingDetails.Type,
              getMetadataValue(propertyIt.second.defaultProperty),
              0);
        }
      }
    }

    pVoxelMaterial->SetVectorParameterValueByInfo(
        FMaterialParameterInfo(
            UTF8_TO_TCHAR("Tile Count"),
            EMaterialParameterAssociation::LayerParameter,
            0),
        pVoxelComponent->_pResources->GetTileCount());
  }

  return pVoxelMaterial;
}

/*static*/ UCesiumVoxelRendererComponent* UCesiumVoxelRendererComponent::Create(
    ACesium3DTileset* pTilesetActor,
    const Cesium3DTilesSelection::TilesetMetadata& tilesetMetadata,
    const Cesium3DTilesSelection::Tile& rootTile,
    const Cesium3DTiles::ExtensionContent3dTilesContentVoxels& voxelExtension,
    const FCesiumVoxelClassDescription* pDescription) {
  if (!pTilesetActor) {
    return nullptr;
  }

  const std::string& voxelClassId = voxelExtension.classProperty;
  if (tilesetMetadata.schema->classes.find(voxelClassId) ==
      tilesetMetadata.schema->classes.end()) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Tileset %s contains voxels, but cannot find the metadata class that describes its contents."),
        *pTilesetActor->GetName())
    return nullptr;
  }

  // Validate voxel grid dimensions.
  const std::vector<int64_t> dimensions = voxelExtension.dimensions;
  if (dimensions.size() < 3 || dimensions[0] <= 0 || dimensions[1] <= 0 ||
      dimensions[2] <= 0) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Tileset %s contains voxels but has invalid dimensions."),
        *pTilesetActor->GetName())
    return nullptr;
  }

  // Validate voxel grid padding, if present.
  glm::uvec3 paddingBefore(0);
  glm::uvec3 paddingAfter(0);

  if (voxelExtension.padding) {
    const std::vector<int64_t> before = voxelExtension.padding->before;
    if (before.size() != 3 || before[0] < 0 || before[1] < 0 || before[2] < 0) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT(
              "Tileset %s has invalid value for padding.before in its voxel extension."),
          *pTilesetActor->GetName())
      return nullptr;
    }

    const std::vector<int64_t> after = voxelExtension.padding->after;
    if (after.size() != 3 || after[0] < 0 || after[1] < 0 || after[2] < 0) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Tileset %s has invalid value for padding.after in its voxel extension."),
          *pTilesetActor->GetName())
      return nullptr;
    }

    paddingBefore = {before[0], before[1], before[2]};
    paddingAfter = {after[0], after[1], after[2]};
  }

  // Check that bounding volume is supported.
  const Cesium3DTilesSelection::BoundingVolume& boundingVolume =
      rootTile.getBoundingVolume();
  EVoxelGridShape shape = getVoxelGridShape(boundingVolume);
  if (shape == EVoxelGridShape::Invalid) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Tileset %s has invalid value for padding.after in its voxel extension."),
        *pTilesetActor->GetName())
    return nullptr;
  }

  const Cesium3DTiles::Class* pVoxelClass =
      &tilesetMetadata.schema->classes.at(voxelClassId);
  assert(pVoxelClass != nullptr);

  UCesiumVoxelRendererComponent* pVoxelComponent =
      NewObject<UCesiumVoxelRendererComponent>(pTilesetActor);
  pVoxelComponent->SetMobility(pTilesetActor->GetRootComponent()->Mobility);
  pVoxelComponent->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pVoxelComponent->_pTileset = pTilesetActor;

  UStaticMeshComponent* pVoxelMesh =
      NewObject<UStaticMeshComponent>(pVoxelComponent);
  pVoxelMesh->SetStaticMesh(pVoxelComponent->CubeMesh);
  pVoxelMesh->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pVoxelMesh->SetMobility(pVoxelComponent->Mobility);
  pVoxelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

  FCustomDepthParameters customDepthParameters =
      pTilesetActor->GetCustomDepthParameters();

  pVoxelMesh->SetRenderCustomDepth(customDepthParameters.RenderCustomDepth);
  pVoxelMesh->SetCustomDepthStencilWriteMask(
      customDepthParameters.CustomDepthStencilWriteMask);
  pVoxelMesh->SetCustomDepthStencilValue(
      customDepthParameters.CustomDepthStencilValue);
  pVoxelMesh->bCastDynamicShadow = false;

  pVoxelMesh->SetupAttachment(pVoxelComponent);
  pVoxelMesh->RegisterComponent();

  pVoxelComponent->MeshComponent = pVoxelMesh;

  // The expected size of the incoming glTF attributes depends on padding and
  // voxel grid shape.
  glm::uvec3 dataDimensions =
      glm::uvec3(dimensions[0], dimensions[1], dimensions[2]) + paddingBefore +
      paddingAfter;
  if (shape == EVoxelGridShape::Box || shape == EVoxelGridShape::Cylinder) {
    // Account for y-up in glTF -> z-up in 3D Tiles.
    dataDimensions =
        glm::uvec3(dataDimensions.x, dataDimensions.z, dataDimensions.y);
  }

  uint32 requestedTextureMemory =
      FVoxelResources::DefaultDataTextureMemoryBytes;

  // uint64_t knownTileCount = 0;
  // if (tilesetMetadata.metadata) {
  //   const Cesium3DTiles::MetadataEntity& metadata =
  //   *tilesetMetadata.metadata;
  //   // TODO: This should find the property by "TILESET_TILE_COUNT"
  //   if (metadata.properties.find("tileCount") != metadata.properties.end()) {
  //     const CesiumUtility::JsonValue& value =
  //         metadata.properties.at("tileCount");
  //     if (value.isInt64()) {
  //       knownTileCount = value.getInt64OrDefault(0);
  //     } else if (value.isUint64()) {
  //       knownTileCount = value.getUint64OrDefault(0);
  //     }
  //   }
  // }

  // if (knownTileCount > 0) {
  //  uint32 maximumTextureMemory =
  //      getMaximumTextureMemory(pDescription, dataDimensions, knownTileCount);
  //  requestedTextureMemory = FMath::Min(
  //      maximumTextureMemory,
  //      FVoxelResources::MaximumDataTextureMemoryBytes);
  //}

  pVoxelComponent->_pResources = MakeUnique<FVoxelResources>(
      pDescription,
      shape,
      dataDimensions,
      pVoxelMesh->GetScene()->GetFeatureLevel(),
      requestedTextureMemory);

  CreateGltfOptions::CreateVoxelOptions& options = pVoxelComponent->Options;
  options.pTilesetExtension = &voxelExtension;
  options.pVoxelClass = pVoxelClass;
  options.gridShape = shape;
  options.voxelCount = dataDimensions.x * dataDimensions.y * dataDimensions.z;

  UMaterialInstanceDynamic* pMaterial =
      UCesiumVoxelRendererComponent::CreateVoxelMaterial(
          pVoxelComponent,
          FVector(dimensions[0], dimensions[1], dimensions[2]),
          FVector(paddingBefore.x, paddingBefore.y, paddingBefore.z),
          FVector(paddingAfter.x, paddingAfter.y, paddingAfter.z),
          pTilesetActor,
          pVoxelClass,
          pDescription,
          boundingVolume);
  pVoxelMesh->SetMaterial(0, pMaterial);

  const glm::dmat4& cesiumToUnrealTransform =
      pTilesetActor->GetCesiumTilesetToUnrealRelativeWorldTransform();
  pVoxelComponent->UpdateTransformFromCesium(cesiumToUnrealTransform);

  return pVoxelComponent;
}

void UCesiumVoxelRendererComponent::UpdateTiles(
    const std::vector<Cesium3DTilesSelection::Tile*>& VisibleTiles,
    const std::vector<double>& VisibleTileScreenSpaceErrors) {
  this->_pResources->Update(VisibleTiles, VisibleTileScreenSpaceErrors);
}

namespace {
/**
 * Updates the input voxel material to account for origin shifting or ellipsoid
 * changes from the tileset's georeference.
 */
void updateEllipsoidVoxelParameters(
    UMaterialInstanceDynamic* pMaterial,
    const ACesiumGeoreference* pGeoreference) {
  if (!pMaterial || !pGeoreference) {
    return;
  }
  FVector radii = getEllipsoidRadii(pGeoreference);
  FMatrix unrealToEcef =
      pGeoreference->ComputeUnrealToEarthCenteredEarthFixedTransformation();
  glm::dmat4 transformToUnit = glm::dmat4(
                                   glm::dvec4(1.0 / (radii.X), 0, 0, 0),
                                   glm::dvec4(0, 1.0 / (radii.Y), 0, 0),
                                   glm::dvec4(0, 0, 1.0 / (radii.Z), 0),
                                   glm::dvec4(0, 0, 0, 1)) *
                               VecMath::createMatrix4D(unrealToEcef);

  pMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 0"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(
          transformToUnit[0][0],
          transformToUnit[1][0],
          transformToUnit[2][0],
          transformToUnit[3][0]));
  pMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 1"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(
          transformToUnit[0][1],
          transformToUnit[1][1],
          transformToUnit[2][1],
          transformToUnit[3][1]));
  pMaterial->SetVectorParameterValueByInfo(
      FMaterialParameterInfo(
          UTF8_TO_TCHAR("Shape TransformToUnit Row 2"),
          EMaterialParameterAssociation::LayerParameter,
          0),
      FVector4(
          transformToUnit[0][2],
          transformToUnit[1][2],
          transformToUnit[2][2],
          transformToUnit[3][2]));
}
} // namespace

void UCesiumVoxelRendererComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  FTransform transform = FTransform(VecMath::createMatrix(
      CesiumToUnrealTransform * this->HighPrecisionTransform));

  if (this->MeshComponent->Mobility == EComponentMobility::Movable) {
    // For movable objects, move the component in the normal way, but don't
    // generate collisions along the way. Teleporting physics is imperfect,
    // but it's the best available option.
    this->MeshComponent->SetRelativeTransform(
        transform,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
  } else {
    // Unreal will yell at us for calling SetRelativeTransform on a static
    // object, but we still need to adjust (accurately!) for origin rebasing
    // and georeference changes. It's "ok" to move a static object in this way
    // because, we assume, the globe and globe-oriented lights, etc. are
    // moving too, so in a relative sense the object isn't actually moving.
    // This isn't a perfect assumption, of course.
    this->MeshComponent->SetRelativeTransform_Direct(transform);
    this->MeshComponent->UpdateComponentToWorld();
    this->MeshComponent->MarkRenderTransformDirty();
  }

  if (this->Options.gridShape == EVoxelGridShape::Ellipsoid) {
    // Ellipsoid voxels are rendered specially due to the ellipsoid radii and
    // georeference, so the material must be updated here.
    UMaterialInstanceDynamic* pMaterial =
        Cast<UMaterialInstanceDynamic>(this->MeshComponent->GetMaterial(0));
    ACesiumGeoreference* pGeoreference = this->_pTileset->ResolveGeoreference();

    updateEllipsoidVoxelParameters(pMaterial, pGeoreference);
  }
}


#pragma once

#include "Components/ActorComponent.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "CesiumEncodedMetadataComponent.generated.h"

UENUM()
enum class ECesiumPropertyComponentType : uint8 { Uint8, Float };

UENUM()
enum class ECesiumPropertyType : uint8 { Scalar, Vec2, Vec3, Vec4 };

ENUM()
enum class ECesiumFeatureTableAccessType : uint8 {
  Unknown,
  Texture,
  Attribute,
  Mixed
};

// Note that these don't exhaustively cover the possibilities of glTF metadata
// classes, they only cover the subset that can be encoded into textures. For
// example, arbitrary size arrays and strings are excluded.

// TODO: descriptions
USTRUCT()
struct CESIUMRUNTIME_API FPropertyDescription {
  GENERATED_USTRUCT_BODY()

  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumPropertyComponentType ComponentType =
      ECesiumPropertyComponentType::Float;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumPropertyType Type = ECesiumPropertyType::Scalar;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool Normalized = false;
};

USTRUCT()
struct CESIUMRUNTIME_API FFeatureTableDescription {
  GENERATED_USTRUCT_BODY()

  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumFeatureTableAccessType AccessType =
      ECesiumFeatureTableAccessType::Unknown;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  TArray<FPropertyDescription> Properties;
};

USTRUCT()
struct CESIUMRUNTIME_API FFeatureTextureDescription {
  GENERATED_USTRUCT_BODY()

  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;
};

UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumEncodedMetadataComponent
    : public UActorComponent {
  GENERATED_BODY()

public:
  UPROPERTY(EditAnywhere, Category = "EncodeMetadata")
  TArray<FFeatureTableDescription> FeatureTables;

  UPROPERTY(EditAnywhere, Category = "EncodeMetadata")
  TArray<FFeatureTextureDescription> FeatureTextures;

  UFUNCTION(CallInEditor, Category = "EncodeMetadata")
  void AutoFill();

#if WITH_EDITOR
  UPROPERTY(EditAnywhere, Category = "GenerateMaterial")
  int32 FeatureIdAttributes;

  UFUNCTION(CallInEditor, Category = "GenerateMaterial")
  void GenerateMaterial();
#endif
};

// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltf/ExtensionMeshPrimitiveExtStructuralMetadata.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumGltfSpecUtility.h"
#include "CesiumMetadataPickingBlueprintLibrary.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumMetadataPickingSpec,
    "Cesium.Unit.MetadataPicking",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
Model model;
MeshPrimitive* pPrimitive;
ExtensionExtMeshFeatures* pMeshFeatures;
ExtensionModelExtStructuralMetadata* pModelMetadata;
ExtensionMeshPrimitiveExtStructuralMetadata* pPrimitiveMetadata;
PropertyTable* pPropertyTable;
PropertyTexture* pPropertyTexture;
TObjectPtr<UCesiumGltfComponent> pModelComponent;
TObjectPtr<UCesiumGltfPrimitiveComponent> pPrimitiveComponent;
CesiumGltfPrimitiveBase* pBase;
END_DEFINE_SPEC(FCesiumMetadataPickingSpec)

void FCesiumMetadataPickingSpec::Define() {
  Describe("FindUVFromHit", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      pPrimitive->mode = CesiumGltf::MeshPrimitive::Mode::TRIANGLES;
      pPrimitiveComponent = NewObject<UCesiumGltfPrimitiveComponent>();
      pBase = getPrimitiveBase(pPrimitiveComponent);
      pBase->pMeshPrimitive = pPrimitive;

      std::vector<glm::vec3> positions{
          glm::vec3(-1, 0, 0),
          glm::vec3(0, 1, 0),
          glm::vec3(1, 0, 0),
          glm::vec3(-1, 3, 0),
          glm::vec3(0, 4, 0),
          glm::vec3(1, 3, 0),
      };
      CreateAttributeForPrimitive(
          model,
          *pPrimitive,
          "POSITION",
          AccessorSpec::Type::VEC3,
          AccessorSpec::ComponentType::FLOAT,
          positions);
      int32_t positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      // For convenience when testing, the UVs are the same as the positions
      // they correspond to. This means that the interpolated UV value should be
      // directly equal to the barycentric coordinates of the triangle.
      std::vector<glm::vec2> texCoords{
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0),
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0)};
      CreateAttributeForPrimitive(
          model,
          *pPrimitive,
          "TEXCOORD_0",
          AccessorSpec::Type::VEC2,
          AccessorSpec::ComponentType::FLOAT,
          texCoords);

      pBase->PositionAccessor =
          AccessorView<FVector3f>(model, positionAccessorIndex);
      pBase->TexCoordAccessorMap.emplace(
          0,
          AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
              model,
              static_cast<int32_t>(model.accessors.size() - 1)));
    });

    It("returns false if hit has no valid component", [this]() {
      FHitResult Hit;
      Hit.Location = FVector_NetQuantize(0, -1, 0);
      Hit.FaceIndex = 0;
      Hit.Component = nullptr;

      FVector2D UV;
      TestFalse(
          "found hit",
          UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 0, UV));
    });

    It("returns false if specified texcoord set does not exist", [this]() {
      FHitResult Hit;
      Hit.Location = FVector_NetQuantize(0, -1, 0);
      Hit.FaceIndex = 0;
      Hit.Component = pPrimitiveComponent;

      FVector2D UV;
      TestFalse(
          "found hit",
          UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 1, UV));
    });

    It("gets hit for primitive without indices", [this]() {
      FHitResult Hit;
      Hit.Location = FVector_NetQuantize(0, -1, 0);
      Hit.FaceIndex = 0;
      Hit.Component = pPrimitiveComponent;

      FVector2D UV = FVector2D::Zero();
      TestTrue(
          "found hit",
          UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 0, UV));
      TestTrue("UV at point (X)", FMath::IsNearlyEqual(UV[0], 0.0));
      TestTrue("UV at point (Y)", FMath::IsNearlyEqual(UV[1], 1.0));

      Hit.Location = FVector_NetQuantize(0, -0.5, 0);
      TestTrue(
          "found hit",
          UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 0, UV));
      TestTrue(
          "UV at point inside triangle (X)",
          FMath::IsNearlyEqual(UV[0], 0.0));
      TestTrue(
          "UV at point inside triangle (Y)",
          FMath::IsNearlyEqual(UV[1], 0.5));

      Hit.FaceIndex = 1;
      Hit.Location = FVector_NetQuantize(0, -4, 0);
      TestTrue(
          "found hit",
          UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 0, UV));
      TestTrue("UV at point (X)", FMath::IsNearlyEqual(UV[0], 0.0));
      TestTrue("UV at point (Y)", FMath::IsNearlyEqual(UV[1], 1.0));
    });

    It("gets hit for primitive with indices", [this]() {
      // Switch the order of the triangles.
      const std::vector<uint8_t> indices{3, 4, 5, 0, 1, 2};
      CreateIndicesForPrimitive(
          model,
          *pPrimitive,
          AccessorSpec::ComponentType::UNSIGNED_BYTE,
          indices);

      pBase->IndexAccessor = AccessorView<uint8_t>(
          model,
          static_cast<int32_t>(model.accessors.size() - 1));

      FHitResult Hit;
      Hit.Location = FVector_NetQuantize(0, -4, 0);
      Hit.FaceIndex = 0;
      Hit.Component = pPrimitiveComponent;

      FVector2D UV = FVector2D::Zero();

      TestTrue(
          "found hit",
          UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 0, UV));
      TestTrue("UV at point (X)", FMath::IsNearlyEqual(UV[0], 0.0));
      TestTrue("UV at point (Y)", FMath::IsNearlyEqual(UV[1], 1.0));

      Hit.Location = FVector_NetQuantize(0, -3.5, 0);
      TestTrue(
          "found hit",
          UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 0, UV));
      TestTrue(
          "UV at point inside triangle (X)",
          FMath::IsNearlyEqual(UV[0], 0.0));
      TestTrue(
          "UV at point inside triangle (Y)",
          FMath::IsNearlyEqual(UV[1], 0.5));

      Hit.FaceIndex = 1;
      Hit.Location = FVector_NetQuantize(0, -1, 0);
      TestTrue(
          "found hit",
          UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(Hit, 0, UV));
      TestTrue("UV at point (X)", FMath::IsNearlyEqual(UV[0], 0.0));
      TestTrue("UV at point (Y)", FMath::IsNearlyEqual(UV[1], 1.0));
    });
  });

  Describe("GetPropertyTableValuesFromHit", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      pPrimitive->mode = MeshPrimitive::Mode::TRIANGLES;

      // Two disconnected triangles.
      std::vector<glm::vec3> positions{
          glm::vec3(-1, 1, 0),
          glm::vec3(1, 1, 0),
          glm::vec3(1, -1, 0),
          glm::vec3(2, 2, 0),
          glm::vec3(-2, 2, 0),
          glm::vec3(-2, -2, 0),
      };
      std::vector<std::byte> positionData(positions.size() * sizeof(glm::vec3));
      std::memcpy(positionData.data(), positions.data(), positionData.size());
      CreateAttributeForPrimitive(
          model,
          *pPrimitive,
          "POSITION",
          AccessorSpec::Type::VEC3,
          AccessorSpec::ComponentType::FLOAT,
          std::move(positionData));

      pMeshFeatures = &pPrimitive->addExtension<ExtensionExtMeshFeatures>();
      pModelMetadata =
          &model.addExtension<ExtensionModelExtStructuralMetadata>();

      std::string className = "testClass";
      pModelMetadata->schema.emplace();
      pModelMetadata->schema->classes[className];

      pPropertyTable = &pModelMetadata->propertyTables.emplace_back();
      pPropertyTable->classProperty = className;

      pModelComponent = NewObject<UCesiumGltfComponent>();
      pPrimitiveComponent =
          NewObject<UCesiumGltfPrimitiveComponent>(pModelComponent);
      pPrimitiveComponent->AttachToComponent(
          pModelComponent,
          FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
      pBase = getPrimitiveBase(pPrimitiveComponent);
      pBase->pMeshPrimitive = pPrimitive;
    });

    It("returns empty map for invalid component", [this]() {
      int32_t positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      FeatureId& featureId =
          AddFeatureIDsAsAttributeToModel(model, *pPrimitive, featureIDs, 2, 0);
      featureId.propertyTable =
          static_cast<int64_t>(pModelMetadata->propertyTables.size() - 1);

      pBase->PositionAccessor =
          AccessorView<FVector3f>(model, positionAccessorIndex);

      std::vector<int32_t> scalarValues{1, 2};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      const std::string scalarPropertyName("scalarProperty");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          scalarValues);

      pModelComponent->Metadata = FCesiumModelMetadata(model, *pModelMetadata);
      pBase->Features =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

      FHitResult Hit;
      Hit.FaceIndex = -1;
      Hit.Component = nullptr;

      auto values =
          UCesiumMetadataPickingBlueprintLibrary::GetPropertyTableValuesFromHit(
              Hit);
      TestTrue("empty values for invalid hit", values.IsEmpty());
    });

    It("returns empty map for invalid feature ID set index", [this]() {
      int32_t positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      FeatureId& featureId =
          AddFeatureIDsAsAttributeToModel(model, *pPrimitive, featureIDs, 2, 0);
      featureId.propertyTable =
          static_cast<int64_t>(pModelMetadata->propertyTables.size() - 1);

      pBase->PositionAccessor =
          AccessorView<FVector3f>(model, positionAccessorIndex);

      std::vector<int32_t> scalarValues{1, 2};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      const std::string scalarPropertyName("scalarProperty");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          scalarValues);

      pModelComponent->Metadata = FCesiumModelMetadata(model, *pModelMetadata);
      pBase->Features =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

      FHitResult Hit;
      Hit.FaceIndex = 0;
      Hit.Component = pPrimitiveComponent;

      auto values =
          UCesiumMetadataPickingBlueprintLibrary::GetPropertyTableValuesFromHit(
              Hit,
              -1);
      TestTrue("empty values for negative index", values.IsEmpty());

      values =
          UCesiumMetadataPickingBlueprintLibrary::GetPropertyTableValuesFromHit(
              Hit,
              1);
      TestTrue(
          "empty values for positive out-of-range index",
          values.IsEmpty());
    });

    It("returns empty values if feature ID set is not associated with a property table",
       [this]() {
         int32_t positionAccessorIndex =
             static_cast<int32_t>(model.accessors.size() - 1);

         std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
         AddFeatureIDsAsAttributeToModel(model, *pPrimitive, featureIDs, 2, 0);

         pBase->PositionAccessor =
             AccessorView<FVector3f>(model, positionAccessorIndex);

         std::vector<int32_t> scalarValues{1, 2};
         pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
         const std::string scalarPropertyName("scalarProperty");
         AddPropertyTablePropertyToModel(
             model,
             *pPropertyTable,
             scalarPropertyName,
             ClassProperty::Type::SCALAR,
             ClassProperty::ComponentType::INT32,
             scalarValues);

         std::vector<glm::vec2> vec2Values{
             glm::vec2(1.0f, 2.5f),
             glm::vec2(3.1f, -4.0f)};
         const std::string vec2PropertyName("vec2Property");
         AddPropertyTablePropertyToModel(
             model,
             *pPropertyTable,
             vec2PropertyName,
             ClassProperty::Type::VEC2,
             ClassProperty::ComponentType::FLOAT32,
             vec2Values);

         pModelComponent->Metadata =
             FCesiumModelMetadata(model, *pModelMetadata);
         pBase->Features =
             FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

         FHitResult Hit;
         Hit.FaceIndex = 0;
         Hit.Component = pPrimitiveComponent;

         const auto values = UCesiumMetadataPickingBlueprintLibrary::
             GetPropertyTableValuesFromHit(Hit);
         TestTrue("values are empty", values.IsEmpty());
       });

    It("returns values for first feature ID set by default", [this]() {
      int32_t positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
      FeatureId& featureId =
          AddFeatureIDsAsAttributeToModel(model, *pPrimitive, featureIDs, 2, 0);
      featureId.propertyTable =
          static_cast<int64_t>(pModelMetadata->propertyTables.size() - 1);

      pBase->PositionAccessor =
          AccessorView<FVector3f>(model, positionAccessorIndex);

      std::vector<int32_t> scalarValues{1, 2};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      const std::string scalarPropertyName("scalarProperty");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          scalarValues);

      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(3.1f, -4.0f)};
      const std::string vec2PropertyName("vec2Property");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      pModelComponent->Metadata = FCesiumModelMetadata(model, *pModelMetadata);
      pBase->Features =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

      FHitResult Hit;
      Hit.Component = pPrimitiveComponent;

      for (size_t i = 0; i < scalarValues.size(); i++) {
        Hit.FaceIndex = i;

        const auto values = UCesiumMetadataPickingBlueprintLibrary::
            GetPropertyTableValuesFromHit(Hit);

        TestEqual("number of values", values.Num(), 2);
        TestTrue(
            "contains scalar value",
            values.Contains(FString(scalarPropertyName.c_str())));
        TestTrue(
            "contains vec2 value",
            values.Contains(FString(vec2PropertyName.c_str())));

        const FCesiumMetadataValue* pScalarValue =
            values.Find(FString(scalarPropertyName.c_str()));
        if (pScalarValue) {
          TestEqual(
              "scalar value",
              UCesiumMetadataValueBlueprintLibrary::GetInteger(
                  *pScalarValue,
                  0),
              scalarValues[i]);
        }

        const FCesiumMetadataValue* pVec2Value =
            values.Find(FString(vec2PropertyName.c_str()));
        if (pVec2Value) {
          FVector2D expected(
              static_cast<double>(vec2Values[i][0]),
              static_cast<double>(vec2Values[i][1]));
          TestEqual(
              "vec2 value",
              UCesiumMetadataValueBlueprintLibrary::GetVector2D(
                  *pVec2Value,
                  FVector2D::Zero()),
              expected);
        }
      }
    });

    It("returns values for specified feature ID set", [this]() {
      int32_t positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      std::vector<uint8_t> featureIDs0{1, 1, 1, 0, 0, 0};
      FeatureId& featureId0 = AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs0,
          2,
          0);
      std::vector<uint8_t> featureIDs1{0, 0, 0, 1, 1, 1};
      FeatureId& featureId1 = AddFeatureIDsAsAttributeToModel(
          model,
          *pPrimitive,
          featureIDs1,
          2,
          1);
      featureId0.propertyTable = featureId1.propertyTable =
          static_cast<int64_t>(pModelMetadata->propertyTables.size() - 1);

      pBase->PositionAccessor =
          AccessorView<FVector3f>(model, positionAccessorIndex);

      std::vector<int32_t> scalarValues{1, 2};
      pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
      const std::string scalarPropertyName("scalarProperty");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT32,
          scalarValues);

      std::vector<glm::vec2> vec2Values{
          glm::vec2(1.0f, 2.5f),
          glm::vec2(3.1f, -4.0f)};
      const std::string vec2PropertyName("vec2Property");
      AddPropertyTablePropertyToModel(
          model,
          *pPropertyTable,
          vec2PropertyName,
          ClassProperty::Type::VEC2,
          ClassProperty::ComponentType::FLOAT32,
          vec2Values);

      pModelComponent->Metadata = FCesiumModelMetadata(model, *pModelMetadata);
      pBase->Features =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

      FHitResult Hit;
      Hit.Component = pPrimitiveComponent;

      for (size_t i = 0; i < scalarValues.size(); i++) {
        Hit.FaceIndex = i;

        const auto values = UCesiumMetadataPickingBlueprintLibrary::
            GetPropertyTableValuesFromHit(Hit, 1);
        TestEqual("number of values", values.Num(), 2);
        TestTrue(
            "contains scalar value",
            values.Contains(FString(scalarPropertyName.c_str())));
        TestTrue(
            "contains vec2 value",
            values.Contains(FString(vec2PropertyName.c_str())));

        const FCesiumMetadataValue* pScalarValue =
            values.Find(FString(scalarPropertyName.c_str()));
        if (pScalarValue) {
          TestEqual(
              "scalar value",
              UCesiumMetadataValueBlueprintLibrary::GetInteger(
                  *pScalarValue,
                  0),
              scalarValues[i]);
        }

        const FCesiumMetadataValue* pVec2Value =
            values.Find(FString(vec2PropertyName.c_str()));
        FVector2D expected(
            static_cast<double>(vec2Values[i][0]),
            static_cast<double>(vec2Values[i][1]));
        if (pVec2Value) {
          TestEqual(
              "vec2 value",
              UCesiumMetadataValueBlueprintLibrary::GetVector2D(
                  *pVec2Value,
                  FVector2D::Zero()),
              expected);
        }
      }
    });
  });

  Describe("GetPropertyTextureValuesFromHit", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      pPrimitive->mode = MeshPrimitive::Mode::TRIANGLES;

      std::vector<glm::vec3> positions{
          glm::vec3(-1, 0, 0),
          glm::vec3(0, 1, 0),
          glm::vec3(1, 0, 0),
          glm::vec3(-1, 3, 0),
          glm::vec3(0, 4, 0),
          glm::vec3(1, 3, 0),
      };

      CreateAttributeForPrimitive(
          model,
          *pPrimitive,
          "POSITION",
          AccessorSpec::Type::VEC3,
          AccessorSpec::ComponentType::FLOAT,
          GetValuesAsBytes(positions));

      int32_t positionAccessorIndex =
          static_cast<int32_t>(model.accessors.size() - 1);

      // For convenience when testing, the UVs are the same as the positions
      // they correspond to. This means that the interpolated UV value should be
      // directly equal to the barycentric coordinates of the triangle.
      std::vector<glm::vec2> texCoords0{
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0),
          glm::vec2(-1, 0),
          glm::vec2(0, 1),
          glm::vec2(1, 0)};

      CreateAttributeForPrimitive(
          model,
          *pPrimitive,
          "TEXCOORD_0",
          AccessorSpec::Type::VEC2,
          AccessorSpec::ComponentType::FLOAT,
          texCoords0);

      pModelMetadata =
          &model.addExtension<ExtensionModelExtStructuralMetadata>();

      std::string className = "testClass";
      pModelMetadata->schema.emplace();
      pModelMetadata->schema->classes[className];

      pPropertyTexture = &pModelMetadata->propertyTextures.emplace_back();
      pPropertyTexture->classProperty = className;

      pPrimitiveMetadata =
          &pPrimitive
               ->addExtension<ExtensionMeshPrimitiveExtStructuralMetadata>();
      pPrimitiveMetadata->propertyTextures.push_back(0);

      pModelComponent = NewObject<UCesiumGltfComponent>();
      pPrimitiveComponent =
          NewObject<UCesiumGltfPrimitiveComponent>(pModelComponent);

      pPrimitiveComponent->AttachToComponent(
          pModelComponent,
          FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));

      pBase = getPrimitiveBase(pPrimitiveComponent);
      pBase->pMeshPrimitive = pPrimitive;
      pBase->PositionAccessor =
          AccessorView<FVector3f>(model, positionAccessorIndex);
      pBase->TexCoordAccessorMap.emplace(
          0,
          AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
              model,
              static_cast<int32_t>(model.accessors.size() - 1)));
    });

    It("returns empty map for invalid component", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::array<int8_t, 4> scalarValues{-1, 2, -3, 4};
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          scalarValues,
          {0});

      pModelComponent->Metadata = FCesiumModelMetadata(model, *pModelMetadata);
      pBase->Metadata =
          FCesiumPrimitiveMetadata(*pPrimitive, *pPrimitiveMetadata);

      FHitResult Hit;
      Hit.FaceIndex = -1;
      Hit.Component = nullptr;

      auto values = UCesiumMetadataPickingBlueprintLibrary::
          GetPropertyTextureValuesFromHit(Hit);
      TestTrue("empty values for invalid hit", values.IsEmpty());
    });

    It("returns empty map for invalid primitive property texture index",
       [this]() {
         std::string scalarPropertyName("scalarProperty");
         std::array<int8_t, 4> scalarValues{-1, 2, -3, 4};
         AddPropertyTexturePropertyToModel(
             model,
             *pPropertyTexture,
             scalarPropertyName,
             ClassProperty::Type::SCALAR,
             ClassProperty::ComponentType::INT8,
             scalarValues,
             {0});

         pModelComponent->Metadata =
             FCesiumModelMetadata(model, *pModelMetadata);
         pBase->Metadata =
             FCesiumPrimitiveMetadata(*pPrimitive, *pPrimitiveMetadata);

         FHitResult Hit;
         Hit.FaceIndex = 0;
         Hit.Component = pPrimitiveComponent;

         auto values = UCesiumMetadataPickingBlueprintLibrary::
             GetPropertyTextureValuesFromHit(Hit, -1);
         TestTrue("empty values for negative index", values.IsEmpty());

         values = UCesiumMetadataPickingBlueprintLibrary::
             GetPropertyTextureValuesFromHit(Hit, 1);
         TestTrue(
             "empty values for positive out-of-range index",
             values.IsEmpty());
       });

    It("returns empty values if property texture does not exist in model metadata",
       [this]() {
         std::string scalarPropertyName("scalarProperty");
         std::array<int8_t, 4> scalarValues{-1, 2, -3, 4};
         AddPropertyTexturePropertyToModel(
             model,
             *pPropertyTexture,
             scalarPropertyName,
             ClassProperty::Type::SCALAR,
             ClassProperty::ComponentType::INT8,
             scalarValues,
             {0});

         pModelComponent->Metadata = FCesiumModelMetadata();

         pPrimitiveMetadata->propertyTextures.clear();
         pPrimitiveMetadata->propertyTextures.push_back(1);
         pBase->Metadata =
             FCesiumPrimitiveMetadata(*pPrimitive, *pPrimitiveMetadata);

         FHitResult Hit;
         Hit.FaceIndex = 0;
         Hit.Component = pPrimitiveComponent;

         const auto values = UCesiumMetadataPickingBlueprintLibrary::
             GetPropertyTextureValuesFromHit(Hit);
         TestTrue("values are empty", values.IsEmpty());
       });

    It("returns values for first primitive property texture by default",
       [this]() {
         std::string scalarPropertyName("scalarProperty");
         std::array<int8_t, 4> scalarValues{-1, 2, -3, 4};
         AddPropertyTexturePropertyToModel(
             model,
             *pPropertyTexture,
             scalarPropertyName,
             ClassProperty::Type::SCALAR,
             ClassProperty::ComponentType::INT8,
             scalarValues,
             {0});

         std::array<glm::u8vec2, 4> vec2Values{
             glm::u8vec2(1, 2),
             glm::u8vec2(0, 4),
             glm::u8vec2(8, 8),
             glm::u8vec2(10, 23)};
         const std::string vec2PropertyName("vec2Property");
         AddPropertyTexturePropertyToModel(
             model,
             *pPropertyTexture,
             vec2PropertyName,
             ClassProperty::Type::VEC2,
             ClassProperty::ComponentType::UINT8,
             vec2Values,
             {0, 1});

         pModelComponent->Metadata =
             FCesiumModelMetadata(model, *pModelMetadata);
         pBase->Metadata =
             FCesiumPrimitiveMetadata(*pPrimitive, *pPrimitiveMetadata);

         FHitResult Hit;
         Hit.FaceIndex = 0;
         Hit.Component = pPrimitiveComponent;

         std::array<FVector_NetQuantize, 3> locations{
             FVector_NetQuantize(1, 0, 0),
             FVector_NetQuantize(0, -1, 0),
             FVector_NetQuantize(0, -0.25, 0)};
         std::array<int8_t, 3> expectedScalar{
             scalarValues[1],
             scalarValues[2],
             scalarValues[0]};
         std::array<FVector2D, 3> expectedVec2{
             FVector2D(vec2Values[1][0], vec2Values[1][1]),
             FVector2D(vec2Values[2][0], vec2Values[2][1]),
             FVector2D(vec2Values[0][0], vec2Values[0][1])};

         for (size_t i = 0; i < locations.size(); i++) {
           Hit.Location = locations[i];

           const auto values = UCesiumMetadataPickingBlueprintLibrary::
               GetPropertyTextureValuesFromHit(Hit);

           TestEqual("number of values", values.Num(), 2);
           TestTrue(
               "contains scalar value",
               values.Contains(FString(scalarPropertyName.c_str())));
           TestTrue(
               "contains vec2 value",
               values.Contains(FString(vec2PropertyName.c_str())));

           const FCesiumMetadataValue* pScalarValue =
               values.Find(FString(scalarPropertyName.c_str()));
           if (pScalarValue) {
             TestEqual(
                 "scalar value",
                 UCesiumMetadataValueBlueprintLibrary::GetInteger(
                     *pScalarValue,
                     0),
                 expectedScalar[i]);
           }

           const FCesiumMetadataValue* pVec2Value =
               values.Find(FString(vec2PropertyName.c_str()));
           if (pVec2Value) {
             TestEqual(
                 "vec2 value",
                 UCesiumMetadataValueBlueprintLibrary::GetVector2D(
                     *pVec2Value,
                     FVector2D::Zero()),
                 expectedVec2[i]);
           }
         }
       });

    It("returns values for specified property texture", [this]() {
      std::string scalarPropertyName("scalarProperty");
      std::array<int8_t, 4> scalarValues{-1, 2, -3, 4};
      AddPropertyTexturePropertyToModel(
          model,
          *pPropertyTexture,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          scalarValues,
          {0});

      // Make another property texture
      PropertyTexture& propertyTexture =
          pModelMetadata->propertyTextures.emplace_back();
      propertyTexture.classProperty = "testClass";
      std::array<int8_t, 4> newScalarValues = {100, -20, 33, -4};
      AddPropertyTexturePropertyToModel(
          model,
          propertyTexture,
          scalarPropertyName,
          ClassProperty::Type::SCALAR,
          ClassProperty::ComponentType::INT8,
          newScalarValues,
          {0});

      pModelComponent->Metadata = FCesiumModelMetadata(model, *pModelMetadata);

      pPrimitiveMetadata->propertyTextures.push_back(1);
      pBase->Metadata =
          FCesiumPrimitiveMetadata(*pPrimitive, *pPrimitiveMetadata);

      FHitResult Hit;
      Hit.Component = pPrimitiveComponent;
      Hit.FaceIndex = 0;

      std::array<FVector_NetQuantize, 3> locations{
          FVector_NetQuantize(1, 0, 0),
          FVector_NetQuantize(0, -1, 0),
          FVector_NetQuantize(0, -0.25, 0)};
      std::array<int8_t, 3> expectedScalar{
          newScalarValues[1],
          newScalarValues[2],
          newScalarValues[0]};
      for (size_t i = 0; i < locations.size(); i++) {
        Hit.Location = locations[i];

        const auto values = UCesiumMetadataPickingBlueprintLibrary::
            GetPropertyTextureValuesFromHit(Hit, 1);
        TestEqual("number of values", values.Num(), 1);
        TestTrue(
            "contains scalar value",
            values.Contains(FString(scalarPropertyName.c_str())));

        const FCesiumMetadataValue* pScalarValue =
            values.Find(FString(scalarPropertyName.c_str()));
        if (pScalarValue) {
          TestEqual(
              "scalar value",
              UCesiumMetadataValueBlueprintLibrary::GetInteger(
                  *pScalarValue,
                  0),
              expectedScalar[i]);
        }
      }
    });
  });

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  Describe("Deprecated", [this]() {
    Describe("GetMetadataValuesForFace", [this]() {
      BeforeEach([this]() {
        model = Model();
        Mesh& mesh = model.meshes.emplace_back();
        pPrimitive = &mesh.primitives.emplace_back();

        // Two disconnected triangles.
        std::vector<glm::vec3> positions{
            glm::vec3(-1, 1, 0),
            glm::vec3(1, 1, 0),
            glm::vec3(1, -1, 0),
            glm::vec3(2, 2, 0),
            glm::vec3(-2, 2, 0),
            glm::vec3(-2, -2, 0),
        };
        std::vector<std::byte> positionData(
            positions.size() * sizeof(glm::vec3));
        std::memcpy(positionData.data(), positions.data(), positionData.size());
        CreateAttributeForPrimitive(
            model,
            *pPrimitive,
            "POSITION",
            AccessorSpec::Type::VEC3,
            AccessorSpec::ComponentType::FLOAT,
            std::move(positionData));

        pMeshFeatures = &pPrimitive->addExtension<ExtensionExtMeshFeatures>();
        pModelMetadata =
            &model.addExtension<ExtensionModelExtStructuralMetadata>();

        std::string className = "testClass";
        pModelMetadata->schema.emplace();
        pModelMetadata->schema->classes[className];

        pPropertyTable = &pModelMetadata->propertyTables.emplace_back();
        pPropertyTable->classProperty = className;

        pModelComponent = NewObject<UCesiumGltfComponent>();
        pPrimitiveComponent =
            NewObject<UCesiumGltfPrimitiveComponent>(pModelComponent);
        pPrimitiveComponent->AttachToComponent(
            pModelComponent,
            FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
        pBase = getPrimitiveBase(pPrimitiveComponent);
      });

      It("returns empty map for invalid face index", [this]() {
        std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
        FeatureId& featureId = AddFeatureIDsAsAttributeToModel(
            model,
            *pPrimitive,
            featureIDs,
            2,
            0);
        featureId.propertyTable =
            static_cast<int64_t>(pModelMetadata->propertyTables.size() - 1);

        std::vector<int32_t> scalarValues{1, 2};
        pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
        const std::string scalarPropertyName("scalarProperty");
        AddPropertyTablePropertyToModel(
            model,
            *pPropertyTable,
            scalarPropertyName,
            ClassProperty::Type::SCALAR,
            ClassProperty::ComponentType::INT32,
            scalarValues);

        pModelComponent->Metadata =
            FCesiumModelMetadata(model, *pModelMetadata);
        pBase->Features =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

        auto values =
            UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
                pPrimitiveComponent,
                -1);
        TestTrue("empty values for negative index", values.IsEmpty());

        values =
            UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
                pPrimitiveComponent,
                2);
        TestTrue(
            "empty values for positive out-of-range index",
            values.IsEmpty());
      });

      It("returns empty map for invalid feature ID set index", [this]() {
        std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
        FeatureId& featureId = AddFeatureIDsAsAttributeToModel(
            model,
            *pPrimitive,
            featureIDs,
            2,
            0);
        featureId.propertyTable =
            static_cast<int64_t>(pModelMetadata->propertyTables.size() - 1);

        std::vector<int32_t> scalarValues{1, 2};
        pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
        const std::string scalarPropertyName("scalarProperty");
        AddPropertyTablePropertyToModel(
            model,
            *pPropertyTable,
            scalarPropertyName,
            ClassProperty::Type::SCALAR,
            ClassProperty::ComponentType::INT32,
            scalarValues);

        pModelComponent->Metadata =
            FCesiumModelMetadata(model, *pModelMetadata);
        pBase->Features =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

        auto values =
            UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
                pPrimitiveComponent,
                0,
                -1);
        TestTrue("empty values for negative index", values.IsEmpty());

        values =
            UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
                pPrimitiveComponent,
                0,
                1);
        TestTrue(
            "empty values for positive out-of-range index",
            values.IsEmpty());
      });

      It("returns empty values if feature ID set is not associated with a property table",
         [this]() {
           std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
           AddFeatureIDsAsAttributeToModel(
               model,
               *pPrimitive,
               featureIDs,
               2,
               0);

           std::vector<int32_t> scalarValues{1, 2};
           pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
           const std::string scalarPropertyName("scalarProperty");
           AddPropertyTablePropertyToModel(
               model,
               *pPropertyTable,
               scalarPropertyName,
               ClassProperty::Type::SCALAR,
               ClassProperty::ComponentType::INT32,
               scalarValues);

           std::vector<glm::vec2> vec2Values{
               glm::vec2(1.0f, 2.5f),
               glm::vec2(3.1f, -4.0f)};
           const std::string vec2PropertyName("vec2Property");
           AddPropertyTablePropertyToModel(
               model,
               *pPropertyTable,
               vec2PropertyName,
               ClassProperty::Type::VEC2,
               ClassProperty::ComponentType::FLOAT32,
               vec2Values);

           pModelComponent->Metadata =
               FCesiumModelMetadata(model, *pModelMetadata);
           pBase->Features =
               FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

           const auto values =
               UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
                   pPrimitiveComponent,
                   0);
           TestTrue("values are empty", values.IsEmpty());
         });

      It("returns values for first feature ID set by default", [this]() {
        std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
        FeatureId& featureId = AddFeatureIDsAsAttributeToModel(
            model,
            *pPrimitive,
            featureIDs,
            2,
            0);
        featureId.propertyTable =
            static_cast<int64_t>(pModelMetadata->propertyTables.size() - 1);

        std::vector<int32_t> scalarValues{1, 2};
        pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
        const std::string scalarPropertyName("scalarProperty");
        AddPropertyTablePropertyToModel(
            model,
            *pPropertyTable,
            scalarPropertyName,
            ClassProperty::Type::SCALAR,
            ClassProperty::ComponentType::INT32,
            scalarValues);

        std::vector<glm::vec2> vec2Values{
            glm::vec2(1.0f, 2.5f),
            glm::vec2(3.1f, -4.0f)};
        const std::string vec2PropertyName("vec2Property");
        AddPropertyTablePropertyToModel(
            model,
            *pPropertyTable,
            vec2PropertyName,
            ClassProperty::Type::VEC2,
            ClassProperty::ComponentType::FLOAT32,
            vec2Values);

        pModelComponent->Metadata =
            FCesiumModelMetadata(model, *pModelMetadata);
        pBase->Features =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

        for (size_t i = 0; i < scalarValues.size(); i++) {
          const auto values =
              UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
                  pPrimitiveComponent,
                  static_cast<int64_t>(i));
          TestEqual("number of values", values.Num(), 2);
          TestTrue(
              "contains scalar value",
              values.Contains(FString(scalarPropertyName.c_str())));
          TestTrue(
              "contains vec2 value",
              values.Contains(FString(vec2PropertyName.c_str())));

          const FCesiumMetadataValue* pScalarValue =
              values.Find(FString(scalarPropertyName.c_str()));
          if (pScalarValue) {
            TestEqual(
                "scalar value",
                UCesiumMetadataValueBlueprintLibrary::GetInteger(
                    *pScalarValue,
                    0),
                scalarValues[i]);
          }

          const FCesiumMetadataValue* pVec2Value =
              values.Find(FString(vec2PropertyName.c_str()));
          if (pVec2Value) {
            FVector2D expected(
                static_cast<double>(vec2Values[i][0]),
                static_cast<double>(vec2Values[i][1]));
            TestEqual(
                "vec2 value",
                UCesiumMetadataValueBlueprintLibrary::GetVector2D(
                    *pVec2Value,
                    FVector2D::Zero()),
                expected);
          }
        }
      });

      It("returns values for specified feature ID set", [this]() {
        std::vector<uint8_t> featureIDs0{1, 1, 1, 0, 0, 0};
        FeatureId& featureId0 = AddFeatureIDsAsAttributeToModel(
            model,
            *pPrimitive,
            featureIDs0,
            2,
            0);
        std::vector<uint8_t> featureIDs1{0, 0, 0, 1, 1, 1};
        FeatureId& featureId1 = AddFeatureIDsAsAttributeToModel(
            model,
            *pPrimitive,
            featureIDs1,
            2,
            1);
        featureId0.propertyTable = featureId1.propertyTable =
            static_cast<int64_t>(pModelMetadata->propertyTables.size() - 1);

        std::vector<int32_t> scalarValues{1, 2};
        pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
        const std::string scalarPropertyName("scalarProperty");
        AddPropertyTablePropertyToModel(
            model,
            *pPropertyTable,
            scalarPropertyName,
            ClassProperty::Type::SCALAR,
            ClassProperty::ComponentType::INT32,
            scalarValues);

        std::vector<glm::vec2> vec2Values{
            glm::vec2(1.0f, 2.5f),
            glm::vec2(3.1f, -4.0f)};
        const std::string vec2PropertyName("vec2Property");
        AddPropertyTablePropertyToModel(
            model,
            *pPropertyTable,
            vec2PropertyName,
            ClassProperty::Type::VEC2,
            ClassProperty::ComponentType::FLOAT32,
            vec2Values);

        pModelComponent->Metadata =
            FCesiumModelMetadata(model, *pModelMetadata);
        pBase->Features =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

        for (size_t i = 0; i < scalarValues.size(); i++) {
          const auto values =
              UCesiumMetadataPickingBlueprintLibrary::GetMetadataValuesForFace(
                  pPrimitiveComponent,
                  i,
                  1);
          TestEqual("number of values", values.Num(), 2);
          TestTrue(
              "contains scalar value",
              values.Contains(FString(scalarPropertyName.c_str())));
          TestTrue(
              "contains vec2 value",
              values.Contains(FString(vec2PropertyName.c_str())));

          const FCesiumMetadataValue& scalarValue =
              *values.Find(FString(scalarPropertyName.c_str()));
          TestEqual(
              "scalar value",
              UCesiumMetadataValueBlueprintLibrary::GetInteger(scalarValue, 0),
              scalarValues[i]);

          const FCesiumMetadataValue& vec2Value =
              *values.Find(FString(vec2PropertyName.c_str()));
          FVector2D expected(
              static_cast<double>(vec2Values[i][0]),
              static_cast<double>(vec2Values[i][1]));
          TestEqual(
              "vec2 value",
              UCesiumMetadataValueBlueprintLibrary::GetVector2D(
                  vec2Value,
                  FVector2D::Zero()),
              expected);
        }
      });
    });

    Describe("GetMetadataValuesForFaceAsStrings", [this]() {
      BeforeEach([this]() {
        model = Model();
        Mesh& mesh = model.meshes.emplace_back();
        pPrimitive = &mesh.primitives.emplace_back();

        // Two disconnected triangles.
        std::vector<glm::vec3> positions{
            glm::vec3(-1, 1, 0),
            glm::vec3(1, 1, 0),
            glm::vec3(1, -1, 0),
            glm::vec3(2, 2, 0),
            glm::vec3(-2, 2, 0),
            glm::vec3(-2, -2, 0),
        };
        std::vector<std::byte> positionData(
            positions.size() * sizeof(glm::vec3));
        std::memcpy(positionData.data(), positions.data(), positionData.size());
        CreateAttributeForPrimitive(
            model,
            *pPrimitive,
            "POSITION",
            AccessorSpec::Type::VEC3,
            AccessorSpec::ComponentType::FLOAT,
            std::move(positionData));

        pMeshFeatures = &pPrimitive->addExtension<ExtensionExtMeshFeatures>();
        pModelMetadata =
            &model.addExtension<ExtensionModelExtStructuralMetadata>();

        std::string className = "testClass";
        pModelMetadata->schema.emplace();
        pModelMetadata->schema->classes[className];

        pPropertyTable = &pModelMetadata->propertyTables.emplace_back();
        pPropertyTable->classProperty = className;

        pModelComponent = NewObject<UCesiumGltfComponent>();
        pPrimitiveComponent =
            NewObject<UCesiumGltfPrimitiveComponent>(pModelComponent);
        pPrimitiveComponent->AttachToComponent(
            pModelComponent,
            FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
        pBase = getPrimitiveBase(pPrimitiveComponent);
      });

      It("returns values for first feature ID set by default", [this]() {
        std::vector<uint8_t> featureIDs{0, 0, 0, 1, 1, 1};
        FeatureId& featureId = AddFeatureIDsAsAttributeToModel(
            model,
            *pPrimitive,
            featureIDs,
            2,
            0);
        featureId.propertyTable =
            static_cast<int64_t>(pModelMetadata->propertyTables.size() - 1);

        std::vector<int32_t> scalarValues{1, 2};
        pPropertyTable->count = static_cast<int64_t>(scalarValues.size());
        const std::string scalarPropertyName("scalarProperty");
        AddPropertyTablePropertyToModel(
            model,
            *pPropertyTable,
            scalarPropertyName,
            ClassProperty::Type::SCALAR,
            ClassProperty::ComponentType::INT32,
            scalarValues);

        std::vector<glm::vec2> vec2Values{
            glm::vec2(1.0f, 2.5f),
            glm::vec2(3.1f, -4.0f)};
        const std::string vec2PropertyName("vec2Property");
        AddPropertyTablePropertyToModel(
            model,
            *pPropertyTable,
            vec2PropertyName,
            ClassProperty::Type::VEC2,
            ClassProperty::ComponentType::FLOAT32,
            vec2Values);

        pModelComponent->Metadata =
            FCesiumModelMetadata(model, *pModelMetadata);
        pBase->Features =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pMeshFeatures);

        for (size_t i = 0; i < scalarValues.size(); i++) {
          const auto strings = UCesiumMetadataPickingBlueprintLibrary::
              GetMetadataValuesForFaceAsStrings(
                  pPrimitiveComponent,
                  static_cast<int64_t>(i));
          TestEqual("number of strings", strings.Num(), 2);
          TestTrue(
              "contains scalar value",
              strings.Contains(FString(scalarPropertyName.c_str())));
          TestTrue(
              "contains vec2 value",
              strings.Contains(FString(vec2PropertyName.c_str())));

          const FString* pScalarString =
              strings.Find(FString(scalarPropertyName.c_str()));
          if (pScalarString) {
            TestEqual(
                "scalar value",
                *pScalarString,
                FString(std::to_string(scalarValues[i]).c_str()));
          }

          const FString* pVec2String =
              strings.Find(FString(vec2PropertyName.c_str()));
          if (pVec2String) {
            std::string expected(
                "X=" + std::to_string(vec2Values[i][0]) +
                " Y=" + std::to_string(vec2Values[i][1]));
            TestEqual("vec2 value", *pVec2String, FString(expected.c_str()));
          }
        }
      });
    });
  });
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

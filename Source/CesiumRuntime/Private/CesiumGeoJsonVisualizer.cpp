#include "CesiumGeoJsonVisualizer.h"
#include "CesiumGeoreference.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfLinesComponent.h"
#include "CesiumRuntime.h"
#include "StaticMeshResources.h"

ACesiumGeoJsonVisualizer::ACesiumGeoJsonVisualizer() : AActor() {
  this->Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
  this->RootComponent = this->Root;
}

ACesiumGeoJsonVisualizer::~ACesiumGeoJsonVisualizer() {}

ACesiumGeoreference* ACesiumGeoJsonVisualizer::ResolveGeoreference() {
  if (IsValid(this->ResolvedGeoreference)) {
    return this->ResolvedGeoreference;
  }

  if (IsValid(this->Georeference.Get())) {
    this->ResolvedGeoreference = this->Georeference.Get();
  } else {
    this->ResolvedGeoreference =
        ACesiumGeoreference::GetDefaultGeoreferenceForActor(this);
  }

  return this->ResolvedGeoreference;
}

void ACesiumGeoJsonVisualizer::AddLineString(
    const FCesiumGeoJsonLineString& LineString,
    bool bDebugMode) {
  ACesiumGeoreference* pGeoreference = this->ResolveGeoreference();
   int32 pointCount = LineString.Points.Num();

  if (!pGeoreference || pointCount == 0) {
    return;
  }

  // hack to ignore duplicate points
   TArray<FVector> uniquePoints;
   uniquePoints.Reserve(pointCount);
   uniquePoints.Add(LineString.Points[0]);
   uniquePoints.Add(LineString.Points[1]);
   for (int i = 2, u = 1; i < pointCount; i++) {
    FVector pos = LineString.Points[i];
    if (uniquePoints[u] != pos) {
      uniquePoints.Add(pos);
      u++;
    }
  }

   pointCount = uniquePoints.Num();

  TUniquePtr<FStaticMeshRenderData> RenderData =
      MakeUnique<FStaticMeshRenderData>();
  RenderData->AllocateLODResources(1);

  FStaticMeshLODResources& LODResources = RenderData->LODResources[0];

  FPositionVertexBuffer& positionBuffer =
      LODResources.VertexBuffers.PositionVertexBuffer;
  FColorVertexBuffer& colorBuffer =
      LODResources.VertexBuffers.ColorVertexBuffer;
  FStaticMeshVertexBuffer& normalBuffer =
      LODResources.VertexBuffers.StaticMeshVertexBuffer;

  positionBuffer.Init(pointCount, false);
  colorBuffer.Init(pointCount, false);
  normalBuffer.Init(pointCount, 1, false);

  FVector min(std::numeric_limits<double>::max()),
      max(std::numeric_limits<double>::lowest());

  for (int32 i = 0; i < pointCount; i++) {
    FVector3f& resultPosition = positionBuffer.VertexPosition(i);
    FVector unrealPosition =
        pGeoreference->TransformLongitudeLatitudeHeightPositionToUnreal(
            uniquePoints[i]);
    resultPosition.X = unrealPosition.X;
    resultPosition.Y = unrealPosition.Y;
    resultPosition.Z = unrealPosition.Z;

    min = FVector::Min(min, unrealPosition);
    max = FVector::Max(max, unrealPosition);

    FColor& resultColor = colorBuffer.VertexColor(i);
    resultColor.R = 0.0;
    resultColor.G = 1.0;
    resultColor.B = 1.0;
    resultColor.A = 1.0;

    FVector normal =
        pGeoreference->GetEllipsoid()->GeodeticSurfaceNormal(unrealPosition);

    normalBuffer.SetVertexTangents(
        i,
        FVector3f(0.0f),
        FVector3f(0.0),
        FVector3f(normal));
  }

  LODResources.bHasColorVertexData = true;

  TArray<uint32> indices;

  if (bDebugMode) {
    indices.Reserve(pointCount * 2 - 2);
    for (int32 i = 0; i < pointCount - 1; i++) {
      indices.Add(i);
      indices.Add(i + 1);
    }
  } else {
    indices.Reserve(pointCount);
    for (int32 i = 0; i < pointCount; i++) {
      indices.Add(i);
    }
  }

  LODResources.IndexBuffer.SetIndices(
      indices,
      pointCount >= std::numeric_limits<uint16>::max()
          ? EIndexBufferStride::Type::Force32Bit
          : EIndexBufferStride::Type::Force16Bit);

  FStaticMeshSectionArray& Sections = LODResources.Sections;
  FStaticMeshSection& section = Sections.AddDefaulted_GetRef();
  section.NumTriangles = 1; // This will be ignored.
  section.FirstIndex = 0;
  section.MinVertexIndex = 0;
  section.MaxVertexIndex = pointCount - 1;
  section.bEnableCollision = false;
  section.bCastShadow = false;
  section.MaterialIndex = 0;

  min *= 100;
  max *= 100;

  FBox aaBox(min, max);
  aaBox.GetCenterAndExtents(
      RenderData->Bounds.Origin,
      RenderData->Bounds.BoxExtent);
  RenderData->Bounds.SphereRadius = 100.0f;

  UCesiumGltfComponent* pGltf = NewObject<UCesiumGltfComponent>(this);
  pGltf->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

  UCesiumGltfLinesComponent* pMesh =
      NewObject<UCesiumGltfLinesComponent>(pGltf, "");

  // Temporary variable hacks just to get something showing
  pMesh->IsPolyline = !bDebugMode;
  pMesh->LineWidth = 20;

  pMesh->bUseDefaultCollision = false;
  pMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  pMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
  pMesh->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pMesh->bCastDynamicShadow = false;

  UStaticMesh* pStaticMesh = NewObject<UStaticMesh>(pMesh, "");
  pStaticMesh->bSupportRayTracing = false;

  pMesh->SetStaticMesh(pStaticMesh);

  pStaticMesh->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pStaticMesh->NeverStream = true;
  pStaticMesh->SetRenderData(std::move(RenderData));

  UMaterialInstanceDynamic* pMaterial = UMaterialInstanceDynamic::Create(
      Material ? Material : pGltf->BaseMaterial,
      nullptr,
      "");
  pMaterial->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pMaterial->TwoSided = true;

  pStaticMesh->AddMaterial(pMaterial);

  pStaticMesh->SetLightingGuid();

  pStaticMesh->InitResources();

  pStaticMesh->CalculateExtendedBounds();
  pStaticMesh->GetRenderData()->ScreenSize[0].Default = 1.0f;

  pStaticMesh->CreateBodySetup();

  pMesh->SetupAttachment(pGltf);

  pMesh->RegisterComponent();

  pGltf->AttachToComponent(
      RootComponent,
      FAttachmentTransformRules::KeepRelativeTransform);
  pGltf->SetVisibility(true, true);
}

void ACesiumGeoJsonVisualizer::Clear() {
  TArray<UCesiumGltfLinesComponent*> gltfComponents;
  this->GetComponents<UCesiumGltfLinesComponent>(gltfComponents);

  for (UCesiumGltfLinesComponent* pGltf : gltfComponents) {
    pGltf->DestroyComponent();
  }
}

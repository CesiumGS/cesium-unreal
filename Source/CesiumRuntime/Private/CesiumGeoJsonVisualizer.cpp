#include "CesiumGeoJsonVisualizer.h"
#include "CesiumGeoreference.h"
#include "CesiumGltfLinesComponent.h"
#include "CesiumRuntime.h"
#include "StaticMeshResources.h"

ACesiumGeoJsonVisualizer::ACesiumGeoJsonVisualizer() : AActor() {}

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
    const FCesiumGeoJsonLineString& LineString) {
  ACesiumGeoreference* pGeoreference = this->ResolveGeoreference();
  if (!pGeoreference || LineString.Points.Num() == 0) {
    return;
  }

  TUniquePtr<FStaticMeshRenderData> RenderData =
      MakeUnique<FStaticMeshRenderData>();
  RenderData->AllocateLODResources(1);

  FStaticMeshLODResources& LODResources = RenderData->LODResources[0];

  const int32 pointCount = LineString.Points.Num();
  FPositionVertexBuffer& positionBuffer =
      LODResources.VertexBuffers.PositionVertexBuffer;
  FColorVertexBuffer& colorBuffer =
      LODResources.VertexBuffers.ColorVertexBuffer;
  FStaticMeshVertexBuffer& normalBuffer =
      LODResources.VertexBuffers.StaticMeshVertexBuffer;

  TArray<uint32> indices;
  indices.Reserve(pointCount);

  positionBuffer.Init(pointCount, false);
  colorBuffer.Init(pointCount, false);
  normalBuffer.Init(pointCount, 0, false);

  for (int32 i = 0; i < pointCount; i++) {
    FVector3f& resultPosition = positionBuffer.VertexPosition(i);
    FVector unrealPosition =
        pGeoreference->TransformLongitudeLatitudeHeightPositionToUnreal(
            LineString.Points[i]);
    resultPosition.X = unrealPosition.X;
    resultPosition.Y = unrealPosition.Y;
    resultPosition.Z = unrealPosition.Z;

    FColor& resultColor = colorBuffer.VertexColor(i);
    resultColor.R = 1.0;
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

    indices.Add(i);
  }

  LODResources.IndexBuffer.SetIndices(
      indices,
      pointCount >= std::numeric_limits<uint16>::max()
          ? EIndexBufferStride::Type::Force32Bit
          : EIndexBufferStride::Type::Force16Bit);

  UCesiumGltfLinesComponent* pMesh =
      NewObject<UCesiumGltfLinesComponent>(this, "");

  // Temporary variable hacks just to get something showing
  // pMesh->IsPolyline = true;
  // pMesh->LineWidth = 5;

  pMesh->bUseDefaultCollision = false;
  pMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
  pMesh->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pMesh->bCastDynamicShadow = false;

  UStaticMesh* pStaticMesh = NewObject<UStaticMesh>(pMesh, "");
  pStaticMesh->bSupportRayTracing = false;

  pStaticMesh->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pStaticMesh->NeverStream = true;

  pStaticMesh->SetRenderData(std::move(RenderData));
  pMesh->SetStaticMesh(pStaticMesh);
}

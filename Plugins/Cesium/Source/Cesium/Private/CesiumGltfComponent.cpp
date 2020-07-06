// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumGltfComponent.h"
#include "tiny_gltf.h"
#include "GltfAccessor.h"
#include "UnrealConversions.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "StaticMeshResources.h"
#include "Async/Async.h"
#include "MeshTypes.h"
#include "Engine/StaticMesh.h"
#include <iostream>
#include "Cesium3DTiles/Gltf.h"
#include <glm/ext/matrix_transform.hpp>

// Invert left and right vectors to turn glTF's right-handed coordinate
// system into a left-handed one like Unreal's. This does _not_ bring the
// coordinate fully into Unreal Engine's coordinate system (which is Z-up),
// but after this X axis inversion, the normal glTF->Cesium transformation
// will actually end up bringing the coordinates into the Unreal Engine
// axes.

static glm::dvec3 gltfToLeftHanded(const glm::dvec3& rightHandedVector)
{
	return glm::dvec3(-rightHandedVector.x, rightHandedVector.y, rightHandedVector.z);
}

static FVector gltfToLeftHanded(const FVector& rightHandedVector)
{
	return FVector(-rightHandedVector.X, rightHandedVector.Y, rightHandedVector.Z);
}

static uint32_t nextMaterialId = 0;

struct LoadModelResult
{
	FStaticMeshRenderData* RenderData;
	tinygltf::Image image;
	tinygltf::PbrMetallicRoughness pbr;
	glm::dmat4x4 transform;
};

// https://github.com/CesiumGS/3d-tiles/tree/master/specification#gltf-transforms
// If we invert the X axis in a glTF to make the coordinate system left-handed,
// then this same matrix also transforms glTF coordinates to Unreal Engine coordinates.
glm::dmat4x4 gltfAxesToCesiumAxes(
	glm::dvec4(1.0,  0.0,  0.0, 0.0),
	glm::dvec4(0.0,  0.0, 1.0, 0.0),
	glm::dvec4(0.0, -1.0,  0.0, 0.0),
	glm::dvec4(0.0,  0.0,  0.0, 1.0)
);

double centimetersPerMeter = 100.0;

glm::dmat4x4 scaleToUnrealWorld = glm::scale(glm::dmat4x4(1.0), glm::dvec3(centimetersPerMeter, centimetersPerMeter, centimetersPerMeter));

glm::dmat4x4 unrealToOrFromCesium(
	glm::dvec4(1.0, 0.0, 0.0, 0.0),
	glm::dvec4(0.0, -1.0, 0.0, 0.0),
	glm::dvec4(0.0, 0.0, 1.0, 0.0),
	glm::dvec4(0.0, 0.0, 0.0, 1.0)
);

static LoadModelResult loadModelAnyThreadPart(const tinygltf::Model& model, const glm::dmat4x4& transform) {
	LoadModelResult result;

	FStaticMeshRenderData* RenderData = new FStaticMeshRenderData();
	RenderData->AllocateLODResources(1);

	FStaticMeshLODResources& LODResources = RenderData->LODResources[0];

	//double centimetersPerMeter = 100.0;

	for (auto meshIt = model.meshes.begin(); meshIt != model.meshes.end(); ++meshIt)
	{
		const tinygltf::Mesh& mesh = *meshIt;

		for (auto primitiveIt = mesh.primitives.begin(); primitiveIt != mesh.primitives.end(); ++primitiveIt)
		{
			TMap<int32, FVertexID> indexToVertexIdMap;

			const tinygltf::Primitive& primitive = *primitiveIt;
			auto positionAccessorIt = primitive.attributes.find("POSITION");
			if (positionAccessorIt == primitive.attributes.end()) {
				// This primitive doesn't have a POSITION semantic, ignore it.
				continue;
			}

			int positionAccessorID = positionAccessorIt->second;
			GltfAccessor<FVector> positionAccessor(model, positionAccessorID);

			const std::vector<double>& min = positionAccessor.gltfAccessor().minValues;
			const std::vector<double>& max = positionAccessor.gltfAccessor().maxValues;

			//glm::dvec3 minPosition = gltfToLeftHanded(glm::dvec3(min[0], min[1], min[2]));
			//glm::dvec3 maxPosition = gltfToLeftHanded(glm::dvec3(max[0], max[1], max[2]));
			glm::dvec3 minPosition = glm::dvec3(min[0], min[1], min[2]);
			glm::dvec3 maxPosition = glm::dvec3(max[0], max[1], max[2]);

			FBox aaBox(
				FVector(minPosition.x, minPosition.y, minPosition.z),
				FVector(maxPosition.x, maxPosition.y, maxPosition.z)
			);

			FBoxSphereBounds BoundingBoxAndSphere;
			aaBox.GetCenterAndExtents(BoundingBoxAndSphere.Origin, BoundingBoxAndSphere.BoxExtent);
			BoundingBoxAndSphere.SphereRadius = 0.0f;

			TArray<FStaticMeshBuildVertex> StaticMeshBuildVertices;
			StaticMeshBuildVertices.SetNum(positionAccessor.size());

			for (size_t i = 0; i < positionAccessor.size(); ++i)
			{
				FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
				//vertex.Position = gltfToLeftHanded(positionAccessor[i]);
				vertex.Position = positionAccessor[i];
				vertex.TangentZ = FVector(0.0f, 0.0f, 1.0f);
				vertex.TangentX = FVector(0.0f, 0.0f, 1.0f);

				float binormalSign =
					GetBasisDeterminantSign(vertex.TangentX.GetSafeNormal(),
						(vertex.TangentZ ^ vertex.TangentX).GetSafeNormal(),
						vertex.TangentZ.GetSafeNormal());

				vertex.TangentY = FVector::CrossProduct(vertex.TangentZ, vertex.TangentX).GetSafeNormal() * binormalSign;

				vertex.UVs[0] = FVector2D(0.0f, 0.0f);
				BoundingBoxAndSphere.SphereRadius = FMath::Max((vertex.Position - BoundingBoxAndSphere.Origin).Size(), BoundingBoxAndSphere.SphereRadius);
			}

			auto normalAccessorIt = primitive.attributes.find("NORMAL");
			if (normalAccessorIt != primitive.attributes.end())
			{
				int normalAccessorID = normalAccessorIt->second;
				GltfAccessor<FVector> normalAccessor(model, normalAccessorID);

				for (size_t i = 0; i < normalAccessor.size(); ++i)
				{
					FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
					//vertex.TangentZ = gltfToLeftHanded(normalAccessor[i]);
					vertex.TangentZ = normalAccessor[i];
					vertex.TangentX = FVector(0.0f, 0.0f, 1.0f);

					float binormalSign =
						GetBasisDeterminantSign(vertex.TangentX.GetSafeNormal(),
							(vertex.TangentZ ^ vertex.TangentX).GetSafeNormal(),
							vertex.TangentZ.GetSafeNormal());

					vertex.TangentY = FVector::CrossProduct(vertex.TangentZ, vertex.TangentX).GetSafeNormal() * binormalSign;
				}
			}
			else
			{

			}

			auto uvAccessorIt = primitive.attributes.find("TEXCOORD_0");
			if (uvAccessorIt != primitive.attributes.end())
			{
				int uvAccessorID = uvAccessorIt->second;
				GltfAccessor<FVector2D> uvAccessor(model, uvAccessorID);

				for (size_t i = 0; i < uvAccessor.size(); ++i)
				{
					FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
					vertex.UVs[0] = uvAccessor[i];
				}
			}

			RenderData->Bounds = BoundingBoxAndSphere;

			LODResources.VertexBuffers.PositionVertexBuffer.Init(StaticMeshBuildVertices);
			LODResources.VertexBuffers.StaticMeshVertexBuffer.Init(StaticMeshBuildVertices, 1);

			FColorVertexBuffer& ColorVertexBuffer = LODResources.VertexBuffers.ColorVertexBuffer;
			if (false) //bHasVertexColors)
			{
				ColorVertexBuffer.Init(StaticMeshBuildVertices);
			}
			else
			{
				ColorVertexBuffer.InitFromSingleColor(FColor::White, positionAccessor.size());
			}

			FStaticMeshLODResources::FStaticMeshSectionArray& Sections = LODResources.Sections;
			FStaticMeshSection& section = Sections.AddDefaulted_GetRef();

			GltfAccessor<uint16_t> indexAccessor(model, primitive.indices);

			// TODO: support primitive types other than TRIANGLES.
			uint32 MinVertexIndex = TNumericLimits<uint32>::Max();
			uint32 MaxVertexIndex = TNumericLimits<uint32>::Min();

			TArray<uint32> IndexBuffer;
			IndexBuffer.SetNumZeroed(indexAccessor.size());

			for (size_t i = 0, outIndex = indexAccessor.size() - 1; i < indexAccessor.size(); ++i, --outIndex)
			{
				uint16_t index = indexAccessor[i];
				MinVertexIndex = FMath::Min(MinVertexIndex, static_cast<const uint32_t>(index));
				MaxVertexIndex = FMath::Max(MaxVertexIndex, static_cast<const uint32_t>(index));
				IndexBuffer[outIndex] = index;
			}

			section.NumTriangles = indexAccessor.size() / 3;
			section.FirstIndex = 0;
			section.MinVertexIndex = MinVertexIndex;
			section.MaxVertexIndex = MaxVertexIndex;
			section.bEnableCollision = true;
			section.bCastShadow = true;

			LODResources.IndexBuffer.SetIndices(IndexBuffer, EIndexBufferStride::Force16Bit);
			LODResources.bHasDepthOnlyIndices = false;
			LODResources.bHasReversedIndices = false;
			LODResources.bHasReversedDepthOnlyIndices = false;
			LODResources.bHasAdjacencyInfo = false;

			int materialID = primitive.material;
			const tinygltf::Material& material = model.materials[materialID];

			const tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;
			const tinygltf::TextureInfo& texture = pbr.baseColorTexture;
			const tinygltf::Image& image = model.images[texture.index];

			result.image = image;
			result.pbr = pbr;

			if (model.nodes.size() > 0 && model.nodes[0].matrix.size() > 0)
			{
				const std::vector<double>& matrix = model.nodes[0].matrix;

				glm::dmat4x4 nodeTransformGltf(
					glm::dvec4(matrix[0], matrix[1], matrix[2], matrix[3]),
					glm::dvec4(matrix[4], matrix[5], matrix[6], matrix[7]),
					glm::dvec4(matrix[8], matrix[9], matrix[10], matrix[11]),
					glm::dvec4(matrix[12], matrix[13], matrix[14], matrix[15])
				);

				result.transform = transform * gltfAxesToCesiumAxes * nodeTransformGltf;
			}
			else
			{
				result.transform = transform * gltfAxesToCesiumAxes;
			}

			section.MaterialIndex = 0;

			// TODO: handle more than one primitive
			break;
		}

		// TODO: handle more than one mesh
		break;
	}

	result.RenderData = std::move(RenderData);

	return result;
}

static void loadModelGameThreadPart(UCesiumGltfComponent* pGltf, LoadModelResult&& loadResult) {
	UStaticMeshComponent* pMesh = NewObject<UStaticMeshComponent>(pGltf);
	pMesh->SetupAttachment(pGltf);
	pMesh->RegisterComponent();

	const glm::dmat4x4& transform = unrealToOrFromCesium * scaleToUnrealWorld * loadResult.transform;
	pMesh->SetWorldTransform(FTransform(FMatrix(
		FVector(transform[0].x, transform[0].y, transform[0].z),
		FVector(transform[1].x, transform[1].y, transform[1].z),
		FVector(transform[2].x, transform[2].y, transform[2].z),
		FVector(transform[3].x, transform[3].y, transform[3].z)
	)));

	UStaticMesh* pStaticMesh = NewObject<UStaticMesh>();
	pMesh->SetStaticMesh(pStaticMesh);

	pStaticMesh->bIsBuiltAtRuntime = true;
	pStaticMesh->NeverStream = true;
	pStaticMesh->RenderData = TUniquePtr<FStaticMeshRenderData>(loadResult.RenderData);

	UTexture2D* pTexture = UTexture2D::CreateTransient(loadResult.image.width, loadResult.image.height, PF_R8G8B8A8);

	unsigned char* pTextureData = static_cast<unsigned char*>(pTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	FMemory::Memcpy(pTextureData, &loadResult.image.image[0], loadResult.image.image.size());
	pTexture->PlatformData->Mips[0].BulkData.Unlock();

	//Update!
	pTexture->UpdateResource();

	const FName ImportedSlotName(*(TEXT("CesiumMaterial") + FString::FromInt(nextMaterialId++)));
	UMaterialInstanceDynamic* pMaterial = UMaterialInstanceDynamic::Create(pGltf->BaseMaterial, nullptr, ImportedSlotName);
	pMaterial->SetVectorParameterValue("baseColorFactor", FVector(loadResult.pbr.baseColorFactor[0], loadResult.pbr.baseColorFactor[1], loadResult.pbr.baseColorFactor[2]));
	pMaterial->SetScalarParameterValue("metallicFactor", loadResult.pbr.metallicFactor);
	pMaterial->SetScalarParameterValue("roughnessFactor", loadResult.pbr.roughnessFactor);
	pMaterial->SetTextureParameterValue("baseColorTexture", pTexture);
	pMaterial->TwoSided = true;

	pStaticMesh->AddMaterial(pMaterial);

	pStaticMesh->InitResources();

	// Set up RenderData bounds and LOD data
	pStaticMesh->CalculateExtendedBounds();

	pStaticMesh->RenderData->ScreenSize[0].Default = 1.0f;
	pMesh->SetMobility(EComponentMobility::Movable);

}

/*static*/ void UCesiumGltfComponent::CreateOffGameThread(AActor* pActor, const tinygltf::Model& model, const glm::dmat4x4& transform, TFunction<void(UCesiumGltfComponent*)> callback) {
	LoadModelResult result = loadModelAnyThreadPart(model, transform);

	AsyncTask(ENamedThreads::GameThread, [pActor, callback, result{ std::move(result) }]() mutable {
		UCesiumGltfComponent* Gltf = NewObject<UCesiumGltfComponent>(pActor);
		loadModelGameThreadPart(Gltf, std::move(result));
		Gltf->SetVisibility(false, true);
		callback(Gltf);
	});
}

UCesiumGltfComponent::UCesiumGltfComponent() 
	: USceneComponent()
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinder<UMaterial> BaseMaterial;
		FConstructorStatics() :
			BaseMaterial(TEXT("/Cesium/GltfMaterial.GltfMaterial"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	this->BaseMaterial = ConstructorStatics.BaseMaterial.Object;

	PrimaryComponentTick.bCanEverTick = false;
}

void UCesiumGltfComponent::LoadModel(const FString& Url)
{
	if (this->LoadedUrl == Url)
	{
		UE_LOG(LogActor, Warning, TEXT("Model URL unchanged"))
			return;
	}

	if (this->Mesh)
	{
		UE_LOG(LogActor, Warning, TEXT("Deleting old model"));
		this->Mesh->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		this->Mesh->UnregisterComponent();
		this->Mesh->DestroyComponent(false);
		this->Mesh = nullptr;
	}

	UE_LOG(LogActor, Warning, TEXT("Loading model"))

	this->LoadedUrl = Url;

	FHttpModule& httpModule = FHttpModule::Get();
	FHttpRequestRef request = httpModule.CreateRequest();
	request->SetURL(Url);

	// TODO: This delegate will be invoked in the game thread, which is totally unnecessary and a waste
	// of the game thread's time. Ideally we'd avoid the main thread entirely, but for now we just
	// dispatch the real work to another thread.
	request->OnProcessRequestComplete().BindUObject(this, &UCesiumGltfComponent::ModelRequestComplete);
	request->ProcessRequest();
}

void UCesiumGltfComponent::ModelRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x)
{
	const TArray<uint8>& content = response->GetContent();
	if (content.Num() < 4)
	{
		return;
	}

	// TODO: is it reasonable to use the global thread pool for this?
	TFuture<void> future = Async(EAsyncExecution::ThreadPool, [this, content]
	{
		gsl::span<const uint8_t> data(static_cast<const uint8_t*>(content.GetData()), content.Num());
		Cesium3DTiles::Gltf::LoadResult loadResult = Cesium3DTiles::Gltf::load(data);

		if (loadResult.warnings.length() > 0) {
			UE_LOG(LogActor, Warning, TEXT("Warnings while loading glTF: %s"), *utf8_to_wstr(loadResult.warnings));
		}

		if (loadResult.errors.length() > 0) {
			UE_LOG(LogActor, Error, TEXT("Errors while loading glTF: %s"), *utf8_to_wstr(loadResult.errors));
		}

		if (loadResult.model) {
			UE_LOG(LogActor, Error, TEXT("glTF model could not be loaded."));
			return;
		}

		tinygltf::Model& model = loadResult.model.value();

		LoadModelResult result = loadModelAnyThreadPart(model, glm::dmat4x4(1.0));

		AsyncTask(ENamedThreads::GameThread, [this, result{ std::move(result) }]() mutable {
			loadModelGameThreadPart(this, std::move(result));
		});
	});
}

void UCesiumGltfComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	this->Mesh->DestroyComponent();
	this->Mesh = nullptr;
}

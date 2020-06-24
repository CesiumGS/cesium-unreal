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
#include "Gltf.h"

static FVector gltfVectorToUnrealVector(const FVector& gltfVector)
{
	return FVector(gltfVector.X, gltfVector.Z, gltfVector.Y);
}

static uint32_t nextMaterialId = 0;

struct LoadModelResult
{
	FStaticMeshRenderData* RenderData;
	tinygltf::Image image;
	tinygltf::PbrMetallicRoughness pbr;
	FTransform transform;
};

static LoadModelResult loadModelAnyThreadPart(const tinygltf::Model& model) {
	LoadModelResult result;

	FStaticMeshRenderData* RenderData = new FStaticMeshRenderData();
	RenderData->AllocateLODResources(1);

	FStaticMeshLODResources& LODResources = RenderData->LODResources[0];

	float centimetersPerMeter = 100.0f;

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

			FVector minPosition = gltfVectorToUnrealVector(FVector(min[0], min[1], min[2])) * centimetersPerMeter;
			FVector maxPosition = gltfVectorToUnrealVector(FVector(max[0], max[1], max[2])) * centimetersPerMeter;

			FBox aaBox(minPosition, maxPosition);

			FBoxSphereBounds BoundingBoxAndSphere;
			aaBox.GetCenterAndExtents(BoundingBoxAndSphere.Origin, BoundingBoxAndSphere.BoxExtent);
			BoundingBoxAndSphere.SphereRadius = 0.0f;

			TArray<FStaticMeshBuildVertex> StaticMeshBuildVertices;
			StaticMeshBuildVertices.SetNum(positionAccessor.size());

			for (size_t i = 0; i < positionAccessor.size(); ++i)
			{
				FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
				vertex.Position = gltfVectorToUnrealVector(positionAccessor[i] * centimetersPerMeter);
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
					vertex.TangentZ = gltfVectorToUnrealVector(normalAccessor[i]);
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

			for (size_t i = 0; i < indexAccessor.size(); ++i)
			{
				uint16_t index = indexAccessor[i];
				MinVertexIndex = FMath::Min(MinVertexIndex, static_cast<const uint32_t>(index));
				MaxVertexIndex = FMath::Max(MaxVertexIndex, static_cast<const uint32_t>(index));
				IndexBuffer[i] = index;
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
				result.transform = FTransform(FMatrix(
					gltfVectorToUnrealVector(FVector(model.nodes[0].matrix[0], model.nodes[0].matrix[1], model.nodes[0].matrix[2])),
					gltfVectorToUnrealVector(FVector(model.nodes[0].matrix[8], model.nodes[0].matrix[9], model.nodes[0].matrix[10])),
					gltfVectorToUnrealVector(FVector(model.nodes[0].matrix[4], model.nodes[0].matrix[5], model.nodes[0].matrix[6])),
					gltfVectorToUnrealVector(FVector(
						(model.nodes[0].matrix[12] + 736570.6875) * centimetersPerMeter,
						(model.nodes[0].matrix[13] - 3292171.25) * centimetersPerMeter,
						(model.nodes[0].matrix[14] - 5406424.5) * centimetersPerMeter
					))
				));
			}
			else
			{
				result.transform = FTransform();
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

	pMesh->SetWorldTransform(loadResult.transform);

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

	pStaticMesh->AddMaterial(pMaterial);

	pStaticMesh->InitResources();

	// Set up RenderData bounds and LOD data
	pStaticMesh->CalculateExtendedBounds();

	pStaticMesh->RenderData->ScreenSize[0].Default = 1.0f;
	pMesh->SetMobility(EComponentMobility::Movable);

}

/*static*/ void UCesiumGltfComponent::CreateOffGameThread(AActor* pActor, const tinygltf::Model& model, TFunction<void(UCesiumGltfComponent*)> callback) {
	LoadModelResult result = loadModelAnyThreadPart(model);

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

		LoadModelResult result = loadModelAnyThreadPart(model);

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

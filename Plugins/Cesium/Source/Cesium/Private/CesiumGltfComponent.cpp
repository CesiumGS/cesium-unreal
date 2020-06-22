// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumGltfComponent.h"
#include "tiny_gltf.h"
#include "GltfAccessor.h"
#include "UnrealStringConversions.h"
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

/*static*/ void UCesiumGltfComponent::CreateOffGameThread(AActor* pActor, const tinygltf::Model& model, TFunction<void(UCesiumGltfComponent*)> callback) {
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
				MinVertexIndex = FMath::Min(MinVertexIndex, static_cast<uint32_t>(index));
				MaxVertexIndex = FMath::Max(MaxVertexIndex, static_cast<uint32_t>(index));
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

	AsyncTask(ENamedThreads::GameThread, [pActor, callback, result]() {
		UCesiumGltfComponent* Gltf = NewObject<UCesiumGltfComponent>(pActor);
		UStaticMeshComponent* pMesh = NewObject<UStaticMeshComponent>(Gltf);
		pMesh->SetupAttachment(Gltf);
		pMesh->RegisterComponent();

		pMesh->SetWorldTransform(result.transform);

		UStaticMesh* pStaticMesh = NewObject<UStaticMesh>();
		pMesh->SetStaticMesh(pStaticMesh);

		pStaticMesh->bIsBuiltAtRuntime = true;
		pStaticMesh->NeverStream = true;
		pStaticMesh->RenderData = TUniquePtr<FStaticMeshRenderData>(result.RenderData);

		UTexture2D* pTexture = UTexture2D::CreateTransient(result.image.width, result.image.height, PF_R8G8B8A8);

		unsigned char* pTextureData = static_cast<unsigned char*>(pTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

		FMemory::Memcpy(pTextureData, &result.image.image[0], result.image.image.size());
		pTexture->PlatformData->Mips[0].BulkData.Unlock();

		//Update!
		pTexture->UpdateResource();

		const FName ImportedSlotName(*(TEXT("CesiumMaterial") + FString::FromInt(nextMaterialId++)));
		UMaterialInstanceDynamic* pMaterial = UMaterialInstanceDynamic::Create(Gltf->BaseMaterial, nullptr, ImportedSlotName);
		pMaterial->SetVectorParameterValue("baseColorFactor", FVector(result.pbr.baseColorFactor[0], result.pbr.baseColorFactor[1], result.pbr.baseColorFactor[2]));
		pMaterial->SetScalarParameterValue("metallicFactor", result.pbr.metallicFactor);
		pMaterial->SetScalarParameterValue("roughnessFactor", result.pbr.roughnessFactor);
		pMaterial->SetTextureParameterValue("baseColorTexture", pTexture);

		pStaticMesh->AddMaterial(pMaterial);

		pStaticMesh->InitResources();

		// Set up RenderData bounds and LOD data
		pStaticMesh->CalculateExtendedBounds();

		pStaticMesh->RenderData->ScreenSize[0].Default = 1.0f;
		pMesh->SetMobility(EComponentMobility::Movable);

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

struct B3dmHeader
{
	unsigned char magic[4];
	uint32_t version;
	uint32_t byteLength;
	uint32_t featureTableJsonByteLength;
	uint32_t featureTableBinaryByteLength;
	uint32_t batchTableJsonByteLength;
	uint32_t batchTableBinaryByteLength;
};

struct B3dmHeaderLegacy1
{
	unsigned char magic[4];
	uint32_t version;
	uint32_t byteLength;
	uint32_t batchLength;
	uint32_t batchTableByteLength;
};

struct B3dmHeaderLegacy2
{
	unsigned char magic[4];
	uint32_t version;
	uint32_t byteLength;
	uint32_t batchTableJsonByteLength;
	uint32_t batchTableBinaryByteLength;
	uint32_t batchLength;
};

void UCesiumGltfComponent::ModelRequestComplete(FHttpRequestPtr request, FHttpResponsePtr response, bool x)
{
	const TArray<uint8>& content = response->GetContent();
	if (content.Num() < 4)
	{
		return;
	}

	// TODO: is it reasonable to use the global thread pool for this?
	TFuture<LoadModelResult> future = Async(EAsyncExecution::ThreadPool, [content]
	{
		tinygltf::TinyGLTF loader;
		tinygltf::Model model;
		std::string errors;
		std::string warnings;

		bool loadSucceeded;

		std::string magic = std::string(&content[0], &content[4]);

		if (magic == "glTF")
		{
			loadSucceeded = loader.LoadBinaryFromMemory(&model, &errors, &warnings, content.GetData(), content.Num());
		}
		else if (magic == "b3dm")
		{
			// TODO: actually use the b3dm payload
			if (content.Num() < sizeof(B3dmHeader))
			{
				// TODO: report error
				return LoadModelResult();
			}

			const B3dmHeader* pHeader = reinterpret_cast<const B3dmHeader*>(&content[0]);

			B3dmHeader header = *pHeader;
			uint32_t headerLength = sizeof(B3dmHeader);
			uint32_t batchLength;

			// Legacy header #1: [batchLength] [batchTableByteLength]
			// Legacy header #2: [batchTableJsonByteLength] [batchTableBinaryByteLength] [batchLength]
			// Current header: [featureTableJsonByteLength] [featureTableBinaryByteLength] [batchTableJsonByteLength] [batchTableBinaryByteLength]
			// If the header is in the first legacy format 'batchTableJsonByteLength' will be the start of the JSON string (a quotation mark) or the glTF magic.
			// Accordingly its first byte will be either 0x22 or 0x67, and so the minimum uint32 expected is 0x22000000 = 570425344 = 570MB. It is unlikely that the feature table JSON will exceed this length.
			// The check for the second legacy format is similar, except it checks 'batchTableBinaryByteLength' instead
			if (pHeader->batchTableJsonByteLength >= 570425344) {
				// First legacy check
				headerLength = sizeof(B3dmHeaderLegacy1);
				const B3dmHeaderLegacy1* pLegacy1 = reinterpret_cast<const B3dmHeaderLegacy1*>(&content[0]);
				batchLength = pLegacy1->batchLength;
				header.batchTableJsonByteLength = pLegacy1->batchTableByteLength;
				header.batchTableBinaryByteLength = 0;
				header.featureTableJsonByteLength = 0;
				header.featureTableBinaryByteLength = 0;

				// TODO
				//Batched3DModel3DTileContent._deprecationWarning(
				//	"b3dm-legacy-header",
				//	"This b3dm header is using the legacy format [batchLength] [batchTableByteLength]. The new format is [featureTableJsonByteLength] [featureTableBinaryByteLength] [batchTableJsonByteLength] [batchTableBinaryByteLength] from https://github.com/CesiumGS/3d-tiles/tree/master/specification/TileFormats/Batched3DModel."
				//);
			}
			else if (pHeader->batchTableBinaryByteLength >= 570425344) {
				// Second legacy check
				headerLength = sizeof(B3dmHeaderLegacy2);
				const B3dmHeaderLegacy2* pLegacy2 = reinterpret_cast<const B3dmHeaderLegacy2*>(&content[0]);
				batchLength = pLegacy2->batchLength;
				header.batchTableJsonByteLength = pLegacy2->batchTableJsonByteLength;
				header.batchTableBinaryByteLength = pLegacy2->batchTableBinaryByteLength;
				header.featureTableJsonByteLength = 0;
				header.featureTableBinaryByteLength = 0;

				// TODO
				//Batched3DModel3DTileContent._deprecationWarning(
				//	"b3dm-legacy-header",
				//	"This b3dm header is using the legacy format [batchTableJsonByteLength] [batchTableBinaryByteLength] [batchLength]. The new format is [featureTableJsonByteLength] [featureTableBinaryByteLength] [batchTableJsonByteLength] [batchTableBinaryByteLength] from https://github.com/CesiumGS/3d-tiles/tree/master/specification/TileFormats/Batched3DModel."
				//);
			}

			if (static_cast<uint32_t>(content.Num()) < pHeader->byteLength)
			{
				// TODO: report error
				return LoadModelResult();
			}

			uint32_t glbStart =
				headerLength +
				header.featureTableJsonByteLength +
				header.featureTableBinaryByteLength +
				header.batchTableJsonByteLength +
				header.batchTableBinaryByteLength;
			uint32_t glbEnd = header.byteLength;

			if (glbEnd <= glbStart)
			{
				// TODO: report error
				return LoadModelResult();
			}

			loadSucceeded = loader.LoadBinaryFromMemory(&model, &errors, &warnings, &content[glbStart], glbEnd - glbStart);
		}
		else
		{
			// TODO: how are external resources handled?
			loadSucceeded = loader.LoadASCIIFromString(&model, &errors, &warnings, reinterpret_cast<const char*>(content.GetData()), content.Num(), "");
		}

		if (!loadSucceeded)
		{
			std::cout << errors << std::endl;
			return LoadModelResult();
		}


		if (warnings.length() > 0)
		{
			std::cout << warnings << std::endl;
		}

		LoadModelResult result;

		FStaticMeshRenderData* RenderData = new FStaticMeshRenderData();
		RenderData->AllocateLODResources(1);

		FStaticMeshLODResources& LODResources = RenderData->LODResources[0];

		float centimetersPerMeter = 100.0f;

		for (auto meshIt = model.meshes.begin(); meshIt != model.meshes.end(); ++meshIt)
		{
			tinygltf::Mesh& mesh = *meshIt;

			for (auto primitiveIt = mesh.primitives.begin(); primitiveIt != mesh.primitives.end(); ++primitiveIt)
			{
				TMap<int32, FVertexID> indexToVertexIdMap;

				tinygltf::Primitive& primitive = *primitiveIt;
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
					MinVertexIndex = FMath::Min(MinVertexIndex, static_cast<uint32_t>(index));
					MaxVertexIndex = FMath::Max(MaxVertexIndex, static_cast<uint32_t>(index));
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
				tinygltf::Material& material = model.materials[materialID];

				tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;
				tinygltf::TextureInfo& texture = pbr.baseColorTexture;
				tinygltf::Image& image = model.images[texture.index];

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
	});

	// TODO: what if this component is destroyed before the future resolves?

	future.Then([this](const TFuture<LoadModelResult>& resultFuture)
	{
		LoadModelResult result = std::move(resultFuture.Get());

		// Finish up in the game thread so we can create UObjects.
		AsyncTask(ENamedThreads::GameThread, [this, result]()
		{
			UE_LOG(LogActor, Warning, TEXT("Back in game thread"))
			this->Mesh = NewObject<UStaticMeshComponent>(this);
			this->Mesh->SetupAttachment(this);
			this->Mesh->RegisterComponent();

			this->Mesh->SetWorldTransform(result.transform);

			UStaticMesh* pStaticMesh = NewObject<UStaticMesh>();
			this->Mesh->SetStaticMesh(pStaticMesh);

			pStaticMesh->bIsBuiltAtRuntime = true;
			pStaticMesh->NeverStream = true;
			pStaticMesh->RenderData = TUniquePtr<FStaticMeshRenderData>(result.RenderData);

			UTexture2D* pTexture = UTexture2D::CreateTransient(result.image.width, result.image.height, PF_R8G8B8A8);

			unsigned char* pTextureData = static_cast<unsigned char*>(pTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

			FMemory::Memcpy(pTextureData, &result.image.image[0], result.image.image.size());
			pTexture->PlatformData->Mips[0].BulkData.Unlock();

			//Update!
			pTexture->UpdateResource();

			const FName ImportedSlotName(*(TEXT("CesiumMaterial") + FString::FromInt(nextMaterialId++)));
			UMaterialInstanceDynamic* pMaterial = UMaterialInstanceDynamic::Create(this->BaseMaterial, nullptr, ImportedSlotName);
			pMaterial->SetVectorParameterValue("baseColorFactor", FVector(result.pbr.baseColorFactor[0], result.pbr.baseColorFactor[1], result.pbr.baseColorFactor[2]));
			pMaterial->SetScalarParameterValue("metallicFactor", result.pbr.metallicFactor);
			pMaterial->SetScalarParameterValue("roughnessFactor", result.pbr.roughnessFactor);
			pMaterial->SetTextureParameterValue("baseColorTexture", pTexture);

			pStaticMesh->AddMaterial(pMaterial);

			pStaticMesh->InitResources();

			// Set up RenderData bounds and LOD data
			pStaticMesh->CalculateExtendedBounds();

			pStaticMesh->RenderData->ScreenSize[0].Default = 1.0f;
			this->Mesh->SetMobility(EComponentMobility::Movable);
		});

		UE_LOG(LogActor, Warning, TEXT("Then"))
	});
}

void UCesiumGltfComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	this->Mesh->DestroyComponent();
	this->Mesh = nullptr;
}

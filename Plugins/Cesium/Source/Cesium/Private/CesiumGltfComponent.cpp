// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumGltfComponent.h"
#include "tiny_gltf.h"
#include <locale>
#include <codecvt>
#include "GltfAccessor.h"

static FString utf8_to_wstr(const std::string& utf8) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wcu8;
	return FString(wcu8.from_bytes(utf8).c_str());
}

static std::string wstr_to_utf8(const FString& utf16) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wcu8;
	return wcu8.to_bytes(*utf16);
}

static FVector gltfVectorToUnrealVector(const FVector& gltfVector)
{
	return FVector(gltfVector.X, gltfVector.Z, gltfVector.Y);
}

// Sets default values for this component's properties
UCesiumGltfComponent::UCesiumGltfComponent()
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

struct LoadModelResult
{
	FStaticMeshRenderData* RenderData;
	tinygltf::Image image;
	tinygltf::PbrMetallicRoughness pbr;
};

void UCesiumGltfComponent::LoadModel(const FString& Url)
{
	if (this->LoadedUrl == Url)
	{
		UE_LOG(LogActor, Warning, TEXT("Model URL unchanged"))
			return;
	}

	if (this->Mesh)
	{
		UE_LOG(LogActor, Warning, TEXT("Deleting old model"))
			this->Mesh->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		this->Mesh->UnregisterComponent();
		this->Mesh->DestroyComponent(false);
		this->Mesh = nullptr;
	}

	UE_LOG(LogActor, Warning, TEXT("Loading model"))

	// TODO: is it reasonable to use the global thread pool for this?
	TFuture<LoadModelResult> x = Async(EAsyncExecution::ThreadPool, [&]
	{
		tinygltf::TinyGLTF loader;
		tinygltf::Model model;
		std::string errors;
		std::string warnings;

		std::string url8 = wstr_to_utf8(Url);
		bool loadSucceeded;
		if (Url.EndsWith("glb"))
		{
			loadSucceeded = loader.LoadBinaryFromFile(&model, &errors, &warnings, url8);
		}
		else
		{
			loadSucceeded = loader.LoadASCIIFromFile(&model, &errors, &warnings, url8);
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

				std::vector<double>& min = positionAccessor.gltfAccessor().minValues;
				std::vector<double>& max = positionAccessor.gltfAccessor().maxValues;

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

	static uint32_t nextMaterialId = 0;

	// TODO: what if this component is destroyed before the future resolves?

	x.Then([this](const TFuture<LoadModelResult>& resultFuture)
	{
		LoadModelResult result = std::move(resultFuture.Get());

		// Finish up in the game thread so we can create UObjects.
		AsyncTask(ENamedThreads::GameThread, [this, result]()
		{
			this->Mesh = NewObject<UStaticMeshComponent>(this);
			this->Mesh->SetupAttachment(this);
			this->Mesh->RegisterComponent();

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

	this->LoadedUrl = Url;
}

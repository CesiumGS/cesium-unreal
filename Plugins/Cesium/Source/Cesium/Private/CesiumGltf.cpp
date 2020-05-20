// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumGltf.h"
#include "tiny_gltf.h"
#include <locale>
#include <codecvt>
#include "StaticMeshDescription.h"
#include "GltfAccessor.h"

FString utf8_to_wstr(const std::string& utf8) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wcu8;
	return FString(wcu8.from_bytes(utf8).c_str());
}

std::string wstr_to_utf8(const FString& utf16) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wcu8;
	return wcu8.to_bytes(*utf16);
}

FVector gltfVectorToUnrealVector(const FVector& gltfVector)
{
	return FVector(gltfVector.X, gltfVector.Z, gltfVector.Y);
}


// Sets default values
ACesiumGltf::ACesiumGltf() :
	Url(TEXT("C:\\Users\\kring\\Documents\\001011.glb"))
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

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	this->RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Model"));
}

void ACesiumGltf::OnConstruction(const FTransform & Transform)
{
	std::cout << *this->Url << std::endl;

	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	std::string errors;
	std::string warnings;

	std::string url = wstr_to_utf8(this->Url);
	bool loadSucceeded;
	if (this->Url.EndsWith("glb"))
	{
		loadSucceeded = loader.LoadBinaryFromFile(&model, &errors, &warnings, url);
	}
	else
	{
		loadSucceeded = loader.LoadASCIIFromFile(&model, &errors, &warnings, url);
	}

	if (!loadSucceeded)
	{
		std::cout << errors << std::endl;
		return;
	}


	if (warnings.length() > 0)
	{
		std::cout << warnings << std::endl;
	}


	//UStaticMeshComponent* pMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	UStaticMeshComponent* pMeshComponent = NewObject<UStaticMeshComponent>(this);
	pMeshComponent->SetupAttachment(this->RootComponent);
	pMeshComponent->RegisterComponent();

	UStaticMesh* pStaticMesh = NewObject<UStaticMesh>();
	pMeshComponent->SetStaticMesh(pStaticMesh);

	pStaticMesh->bIsBuiltAtRuntime = true;
	pStaticMesh->NeverStream = true;
	pStaticMesh->RenderData = MakeUnique<FStaticMeshRenderData>();
	pStaticMesh->RenderData->AllocateLODResources(1);

	FStaticMeshLODResources& LODResources = pStaticMesh->RenderData->LODResources[0];

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
					//vertex.TangentY = FVector(0.0f, 0.0, 0.0);
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

			pStaticMesh->RenderData->Bounds = BoundingBoxAndSphere;

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

			static uint32_t nextMaterialId = 0;

			int materialID = primitive.material;
			tinygltf::Material& material = model.materials[materialID];

			tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;
			tinygltf::TextureInfo& texture = pbr.baseColorTexture;
			tinygltf::Image& image = model.images[texture.index];

			UTexture2D* pTexture = UTexture2D::CreateTransient(image.width, image.height, PF_R8G8B8A8);
			
			unsigned char* pTextureData = static_cast<unsigned char*>(pTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
			
			//size_t sourceIndex = 0;
			//size_t destIndex = 0;
			//for (size_t i = 0; i < image.image.size() / 3; ++i)
			//{
			//	pTextureData[destIndex++] = image.image[sourceIndex++];
			//	pTextureData[destIndex++] = image.image[sourceIndex++];
			//	pTextureData[destIndex++] = image.image[sourceIndex++];
			//	pTextureData[destIndex++] = 255;
			//}

			FMemory::Memcpy(pTextureData, &image.image[0], image.image.size());
			pTexture->PlatformData->Mips[0].BulkData.Unlock();

			//Update!
			pTexture->UpdateResource();

			const FName ImportedSlotName(*(TEXT("CesiumMaterial") + FString::FromInt(nextMaterialId++)));
			UMaterialInstanceDynamic* pMaterial = UMaterialInstanceDynamic::Create(this->BaseMaterial, nullptr, ImportedSlotName);
			pMaterial->SetVectorParameterValue("baseColorFactor", FVector(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2]));
			pMaterial->SetScalarParameterValue("metallicFactor", pbr.metallicFactor);
			pMaterial->SetScalarParameterValue("roughnessFactor", pbr.roughnessFactor);
			pMaterial->SetTextureParameterValue("baseColorTexture", pTexture);

			pStaticMesh->AddMaterial(pMaterial);

			section.MaterialIndex = 0;

			// TODO: handle more than one primitive
			break;
		}

		// TODO: handle more than one mesh
		break;
	}

	pStaticMesh->InitResources();

	// Set up RenderData bounds and LOD data
	pStaticMesh->CalculateExtendedBounds();

	pStaticMesh->RenderData->ScreenSize[0].Default = 1.0f;
	pMeshComponent->SetMobility(EComponentMobility::Movable);

	//TArray<const FMeshDescription*> meshDescriptions({&meshDescription});
	//pStaticMesh->BuildFromMeshDescriptions(meshDescriptions);
}

// Called when the game starts or when spawned
void ACesiumGltf::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ACesiumGltf::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


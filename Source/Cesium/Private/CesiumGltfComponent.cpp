
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
#include "Engine/CollisionProfile.h"
#include <iostream>
#include "Cesium3DTiles/Gltf.h"
#include "PhysicsEngine/BodySetup.h"
#if PHYSICS_INTERFACE_PHYSX
#include "IPhysXCooking.h"
#endif
#include "UCesiumGltfPrimitiveComponent.h"
#include "CesiumTransforms.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat3x3.hpp>
#include "Cesium3DTiles/RasterOverlayTile.h"
#include "CesiumGeometry/Rectangle.h"
#include <glm/gtc/quaternion.hpp>
#include "StaticMeshOperations.h"
#include "mikktspace.h"

static uint32_t nextMaterialId = 0;

struct LoadModelResult
{
	FStaticMeshRenderData* RenderData;
	const tinygltf::Model* pModel;
	const tinygltf::Material* pMaterial;
	glm::dmat4x4 transform;
#if PHYSICS_INTERFACE_PHYSX
	PxTriangleMesh* pCollisionMesh;
#endif
	std::string name;
};

// Initialize with a static function instead of inline to avoid an
// internal compiler error in MSVC v14.27.29110.
static glm::dmat4 createGltfAxesToCesiumAxes() {
	// https://github.com/CesiumGS/3d-tiles/tree/master/specification#gltf-transforms
	return glm::dmat4(
		glm::dvec4(1.0,  0.0, 0.0, 0.0),
		glm::dvec4(0.0,  0.0, 1.0, 0.0),
		glm::dvec4(0.0, -1.0, 0.0, 0.0),
		glm::dvec4(0.0,  0.0, 0.0, 1.0)
	);
}

glm::dmat4 gltfAxesToCesiumAxes = createGltfAxesToCesiumAxes();

static const std::string rasterOverlay0 = "_CESIUMOVERLAY_0";

template <class T, class TIndexAccessor>
static void updateTextureCoordinates(
	const tinygltf::Model& model,
	const tinygltf::Primitive& primitive,
	TArray<FStaticMeshBuildVertex>& vertices,
	const TIndexAccessor& indicesAccessor,
	const T& texture,
	int textureCoordinateIndex
) {
	updateTextureCoordinates(
		model,
		primitive,
		vertices,
		indicesAccessor,
		"TEXCOORD_" + std::to_string(texture.texCoord),
		textureCoordinateIndex
	);
}

template <class TIndexAccessor>
void updateTextureCoordinates(
	const tinygltf::Model& model,
	const tinygltf::Primitive& primitive,
	TArray<FStaticMeshBuildVertex>& vertices,
	const TIndexAccessor& indicesAccessor,
	const std::string& attributeName,
	int textureCoordinateIndex
) {
	auto uvAccessorIt = primitive.attributes.find(attributeName);
	if (uvAccessorIt != primitive.attributes.end()) {
		int uvAccessorID = uvAccessorIt->second;
		GltfAccessor<FVector2D> uvAccessor(model, uvAccessorID);

		for (size_t i = 0; i < indicesAccessor.size(); ++i) {
			FStaticMeshBuildVertex& vertex = vertices[i];
			TIndexAccessor::value_type vertexIndex = indicesAccessor[i];
			vertex.UVs[textureCoordinateIndex] = uvAccessor[vertexIndex];
		}
	}
}

static int mikkGetNumFaces(const SMikkTSpaceContext* Context) {
	TArray<FStaticMeshBuildVertex>& vertices = *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
	return vertices.Num() / 3;
}

static int mikkGetNumVertsOfFace(const SMikkTSpaceContext* Context, const int FaceIdx) {
	TArray<FStaticMeshBuildVertex>& vertices = *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
	return FaceIdx < (vertices.Num() / 3) ? 3 : 0;
}

static void mikkGetPosition(const SMikkTSpaceContext* Context, float Position[3], const int FaceIdx, const int VertIdx) {
	TArray<FStaticMeshBuildVertex>& vertices = *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
	FVector& position = vertices[FaceIdx * 3 + VertIdx].Position;
	Position[0] = position.X;
	Position[1] = position.Y;
	Position[2] = position.Z;
}

static void mikkGetNormal(const SMikkTSpaceContext* Context, float Normal[3], const int FaceIdx, const int VertIdx) {
	TArray<FStaticMeshBuildVertex>& vertices = *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
	FVector& normal = vertices[FaceIdx * 3 + VertIdx].TangentZ;
	Normal[0] = normal.X;
	Normal[1] = normal.Y;
	Normal[2] = normal.Z;
}

static void mikkGetTexCoord(const SMikkTSpaceContext* Context, float UV[2], const int FaceIdx, const int VertIdx) {
	TArray<FStaticMeshBuildVertex>& vertices = *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
	FVector2D& uv = vertices[FaceIdx * 3 + VertIdx].UVs[0];
	UV[0] = uv.X;
	UV[1] = uv.Y;
}

static void mikkSetTSpaceBasic(const SMikkTSpaceContext* Context, const float Tangent[3], const float BitangentSign, const int FaceIdx, const int VertIdx) {
	TArray<FStaticMeshBuildVertex>& vertices = *reinterpret_cast<TArray<FStaticMeshBuildVertex>*>(Context->m_pUserData);
	FStaticMeshBuildVertex& vertex = vertices[FaceIdx * 3 + VertIdx];
	vertex.TangentX = FVector(Tangent[0], Tangent[1], Tangent[2]);
	vertex.TangentY = BitangentSign * FVector::CrossProduct(vertex.TangentZ, vertex.TangentX);
}

static void computeTangentSpace(TArray<FStaticMeshBuildVertex>& vertices) {
	SMikkTSpaceInterface MikkTInterface;
	MikkTInterface.m_getNormal = mikkGetNormal;
	MikkTInterface.m_getNumFaces = mikkGetNumFaces;
	MikkTInterface.m_getNumVerticesOfFace = mikkGetNumVertsOfFace;
	MikkTInterface.m_getPosition = mikkGetPosition;
	MikkTInterface.m_getTexCoord = mikkGetTexCoord;
	MikkTInterface.m_setTSpaceBasic = mikkSetTSpaceBasic;
	MikkTInterface.m_setTSpace = nullptr;

	SMikkTSpaceContext MikkTContext;
	MikkTContext.m_pInterface = &MikkTInterface;
	MikkTContext.m_pUserData = (void*)(&vertices);
	MikkTContext.m_bIgnoreDegenerates = false;
	genTangSpaceDefault(&MikkTContext);
}

static tinygltf::Material defaultMaterial;

template <class TIndexAccessor>
static void loadPrimitive(
	std::vector<LoadModelResult>& result,
	const tinygltf::Model& model,
	const tinygltf::Primitive& primitive,
	const glm::dmat4x4& transform,
#if PHYSICS_INTERFACE_PHYSX
	IPhysXCooking* pPhysXCooking,
#endif
	const GltfAccessor<FVector>& positionAccessor,
	const TIndexAccessor& indicesAccessor
) {
	if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
		// TODO: add support for primitive types other than triangles.
		return;
	}

	FStaticMeshRenderData* RenderData = new FStaticMeshRenderData();
	RenderData->AllocateLODResources(1);

	FStaticMeshLODResources& LODResources = RenderData->LODResources[0];

	const std::vector<double>& min = positionAccessor.gltfAccessor().minValues;
	const std::vector<double>& max = positionAccessor.gltfAccessor().maxValues;

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
	StaticMeshBuildVertices.SetNum(indicesAccessor.size());

	// The static mesh we construct will _not_ be indexed, even if the incoming glTF is.
	// This allows us to compute flat normals if the glTF doesn't include them already, and it
	// allows us to compute a correct tangent space basis according to the MikkTSpace algorithm
	// when tangents are not included in the glTF.

	for (size_t i = 0; i < indicesAccessor.size(); ++i)
	{
		FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
		TIndexAccessor::value_type vertexIndex = indicesAccessor[i];
		vertex.Position = positionAccessor[vertexIndex];
		vertex.UVs[0] = FVector2D(0.0f, 0.0f);
		vertex.UVs[2] = FVector2D(0.0f, 0.0f);
		BoundingBoxAndSphere.SphereRadius = FMath::Max((vertex.Position - BoundingBoxAndSphere.Origin).Size(), BoundingBoxAndSphere.SphereRadius);
	}

	// TangentX: Tangent
	// TangentY: Bi-tangent
	// TangentZ: Normal

	auto normalAccessorIt = primitive.attributes.find("NORMAL");
	if (normalAccessorIt != primitive.attributes.end()) {
		int normalAccessorID = normalAccessorIt->second;
		GltfAccessor<FVector> normalAccessor(model, normalAccessorID);

		for (size_t i = 0; i < indicesAccessor.size(); ++i) {
			FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
			TIndexAccessor::value_type vertexIndex = indicesAccessor[i];
			vertex.TangentZ = normalAccessor[vertexIndex];
		}
	} else {
		// Compute flat normals
		for (size_t i = 0; i < indicesAccessor.size(); i += 3) {
			FStaticMeshBuildVertex& v0 = StaticMeshBuildVertices[i];
			FStaticMeshBuildVertex& v1 = StaticMeshBuildVertices[i + 1];
			FStaticMeshBuildVertex& v2 = StaticMeshBuildVertices[i + 2];

			FVector v01 = v1.Position - v0.Position;
			FVector v02 = v2.Position - v0.Position;
			FVector normal = FVector::CrossProduct(v01, v02);
			
			v0.TangentZ = normal.GetSafeNormal();
			v1.TangentZ = v0.TangentZ;
			v2.TangentZ = v0.TangentZ;
		}
	}

	auto tangentAccessorIt = primitive.attributes.find("TANGENT");
	if (tangentAccessorIt != primitive.attributes.end()) {
		int tangentAccessorID = normalAccessorIt->second;
		GltfAccessor<FVector4> tangentAccessor(model, tangentAccessorID);

		for (size_t i = 0; i < indicesAccessor.size(); ++i) {
			FStaticMeshBuildVertex& vertex = StaticMeshBuildVertices[i];
			TIndexAccessor::value_type vertexIndex = indicesAccessor[i];
			const FVector4& tangent = tangentAccessor[vertexIndex];
			vertex.TangentX = tangent;
			vertex.TangentY = FVector::CrossProduct(vertex.TangentZ, vertex.TangentX) * tangent.W;
		}
	} else {
		// Use mikktspace to calculate the tangents
		computeTangentSpace(StaticMeshBuildVertices);
	}

	// In the GltfMaterial defined in the Unreal Editor, each texture has its own set of
	// texture coordinates, and these cannot be changed at runtime:
	// 0 - baseColorTexture
	// 1 - metallicRoughnessTexture
	// 2 - normalTexture
	// 3 - occlusionTexture
	// 4 - emissiveTexture

	// We need to copy the texture coordinates associated with each texture (if any) into the
	// the appropriate UVs slot in FStaticMeshBuildVertex.

	int materialID = primitive.material;
	const tinygltf::Material* pMaterial = materialID >= 0 && materialID < model.materials.size() ? &model.materials[materialID] : &defaultMaterial;

	if (pMaterial) {
		updateTextureCoordinates(model, primitive, StaticMeshBuildVertices, indicesAccessor, pMaterial->pbrMetallicRoughness.baseColorTexture, 0);
		updateTextureCoordinates(model, primitive, StaticMeshBuildVertices, indicesAccessor, pMaterial->pbrMetallicRoughness.metallicRoughnessTexture, 1);
		updateTextureCoordinates(model, primitive, StaticMeshBuildVertices, indicesAccessor, pMaterial->normalTexture, 2);
		updateTextureCoordinates(model, primitive, StaticMeshBuildVertices, indicesAccessor, pMaterial->occlusionTexture, 3);
		updateTextureCoordinates(model, primitive, StaticMeshBuildVertices, indicesAccessor, pMaterial->emissiveTexture, 4);
	}

	// Currently only one set of raster overlay texture coordinates is supported, and it is at UVs[5].
	// TODO: Support more texture coordinate sets (e.g. web mercator and geographic)
	updateTextureCoordinates(model, primitive, StaticMeshBuildVertices, indicesAccessor, rasterOverlay0, 5);

	RenderData->Bounds = BoundingBoxAndSphere;

	LODResources.VertexBuffers.PositionVertexBuffer.Init(StaticMeshBuildVertices);
	LODResources.VertexBuffers.StaticMeshVertexBuffer.Init(StaticMeshBuildVertices, 6);

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
	section.bEnableCollision = true;

	section.NumTriangles = StaticMeshBuildVertices.Num() / 3;
	section.FirstIndex = 0;
	section.MinVertexIndex = 0;
	section.MaxVertexIndex = StaticMeshBuildVertices.Num() - 1;
	section.bEnableCollision = true;
	section.bCastShadow = true;

	TArray<uint32> indices;
	indices.SetNum(StaticMeshBuildVertices.Num());

	// Note that we're reversing the order of the indices, because the change from the glTF right-handed to
	// the Unreal left-handed coordinate system reverses the winding order.
	for (int32 i = 0; i < indices.Num(); ++i) {
		indices[i] = indices.Num() - i - 1;
	}

	LODResources.IndexBuffer.SetIndices(indices, indices.Num() > std::numeric_limits<uint16>::max() ? EIndexBufferStride::Type::Force32Bit : EIndexBufferStride::Type::Force16Bit);

	LODResources.bHasDepthOnlyIndices = false;
	LODResources.bHasReversedIndices = false;
	LODResources.bHasReversedDepthOnlyIndices = false;
	LODResources.bHasAdjacencyInfo = false;

	LoadModelResult primitiveResult;
	primitiveResult.pModel = &model;
	primitiveResult.RenderData = RenderData;
	primitiveResult.transform = transform;

	primitiveResult.pMaterial = pMaterial;

	section.MaterialIndex = 0;

#if PHYSICS_INTERFACE_PHYSX
	primitiveResult.pCollisionMesh = nullptr;

	if (pPhysXCooking) {
		// TODO: use PhysX interface directly so we don't need to copy the vertices (it takes a stride parameter).
		TArray<FVector> vertices;
		vertices.SetNum(StaticMeshBuildVertices.Num());

		for (size_t i = 0; i < StaticMeshBuildVertices.Num(); ++i) {
			vertices[i] = StaticMeshBuildVertices[i].Position;
		}

		TArray<FTriIndices> physicsIndices;
		physicsIndices.SetNum(StaticMeshBuildVertices.Num() / 3);

		// Reversing triangle winding order here, too.
		for (size_t i = 0; i < StaticMeshBuildVertices.Num() / 3; ++i) {
			physicsIndices[i].v0 = i * 3 + 2;
			physicsIndices[i].v1 = i * 3 + 1;
			physicsIndices[i].v2 = i * 3;
		}

		pPhysXCooking->CreateTriMesh("PhysXGeneric", EPhysXMeshCookFlags::Default, vertices, physicsIndices, TArray<uint16>(), true, primitiveResult.pCollisionMesh);
	}
#endif

	result.push_back(std::move(primitiveResult));
}

static void loadPrimitive(
	std::vector<LoadModelResult>& result,
	const tinygltf::Model& model,
	const tinygltf::Primitive& primitive,
	const glm::dmat4x4& transform
#if PHYSICS_INTERFACE_PHYSX
	,IPhysXCooking* pPhysXCooking
#endif
) {
	auto positionAccessorIt = primitive.attributes.find("POSITION");
	if (positionAccessorIt == primitive.attributes.end()) {
		// This primitive doesn't have a POSITION semantic, ignore it.
		return;
	}

	int positionAccessorID = positionAccessorIt->second;
	GltfAccessor<FVector> positionAccessor(model, positionAccessorID);

	if (primitive.indices < 0 || primitive.indices >= model.accessors.size()) {
		std::vector<uint32_t> syntheticIndexBuffer(positionAccessor.size());
		syntheticIndexBuffer.resize(positionAccessor.size());
		for (uint32_t i = 0; i < positionAccessor.size(); ++i) {
			syntheticIndexBuffer[i] = i;
		}
		loadPrimitive(
			result,
			model,
			primitive,
			transform,
			pPhysXCooking,
#if PHYSICS_INTERFACE_PHYSX
			positionAccessor,
#endif
			syntheticIndexBuffer
		);
	} else {
		const tinygltf::Accessor& indexAccessorGltf = model.accessors[primitive.indices];
		if (indexAccessorGltf.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
			GltfAccessor<uint16_t> indexAccessor(model, primitive.indices);
			loadPrimitive(
				result,
				model,
				primitive,
				transform,
				pPhysXCooking,
#if PHYSICS_INTERFACE_PHYSX
				positionAccessor,
#endif
				indexAccessor
			);
		} else if (indexAccessorGltf.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
			GltfAccessor<uint32_t> indexAccessor(model, primitive.indices);
			loadPrimitive(
				result,
				model,
				primitive,
				transform,
				pPhysXCooking,
#if PHYSICS_INTERFACE_PHYSX
				positionAccessor,
#endif
				indexAccessor
			);
		} else {
			// TODO: report unsupported index type.
			return;
		}
	}
}

static void loadMesh(
	std::vector<LoadModelResult>& result,
	const tinygltf::Model& model,
	const tinygltf::Mesh& mesh,
	const glm::dmat4x4& transform
#if PHYSICS_INTERFACE_PHYSX
	,IPhysXCooking* pPhysXCooking
#endif
) {
	for (const tinygltf::Primitive& primitive : mesh.primitives) {
		loadPrimitive(
			result, model,
			primitive,
			transform
#if PHYSICS_INTERFACE_PHYSX
			,pPhysXCooking
#endif
		);
	}
}

static void loadNode(
	std::vector<LoadModelResult>& result,
	const tinygltf::Model& model,
	const tinygltf::Node& node,
	const glm::dmat4x4& transform
#if PHYSICS_INTERFACE_PHYSX
	,IPhysXCooking* pPhysXCooking
#endif
) {
	glm::dmat4x4 nodeTransform = transform;

	if (node.matrix.size() > 0) {
		const std::vector<double>& matrix = node.matrix;

		glm::dmat4x4 nodeTransformGltf(
			glm::dvec4(matrix[0], matrix[1], matrix[2], matrix[3]),
			glm::dvec4(matrix[4], matrix[5], matrix[6], matrix[7]),
			glm::dvec4(matrix[8], matrix[9], matrix[10], matrix[11]),
			glm::dvec4(matrix[12], matrix[13], matrix[14], matrix[15])
		);

		nodeTransform = nodeTransform * nodeTransformGltf;
	}
	else if (node.translation.size() > 0 || node.rotation.size() > 0 || node.scale.size() > 0) {
		glm::dmat4 translation(1.0);
		if (node.translation.size() == 3) {
			translation[3] = glm::dvec4(node.translation[0], node.translation[1], node.translation[2], 1.0);
		}

		glm::dquat rotationQuat(1.0, 0.0, 0.0, 0.0);
		if (node.rotation.size() == 4) {
			rotationQuat[0] = node.rotation[0];
			rotationQuat[1] = node.rotation[1];
			rotationQuat[2] = node.rotation[2];
			rotationQuat[3] = node.rotation[3];
		}

		glm::dmat4 scale(1.0);
		
		if (node.scale.size() == 3) {
			scale[0].x = node.scale[0];
			scale[1].y = node.scale[1];
			scale[2].z = node.scale[2];
		}

		nodeTransform = nodeTransform * translation * glm::dmat4(rotationQuat) * scale;
	}

	int meshId = node.mesh;
	if (meshId >= 0 && meshId < model.meshes.size()) {
		const tinygltf::Mesh& mesh = model.meshes[meshId];
		loadMesh(
			result,
			model,
			mesh,
			nodeTransform
#if PHYSICS_INTERFACE_PHYSX
			,pPhysXCooking
#endif
		);
	}

	for (int childNodeId : node.children) {
		if (childNodeId >= 0 && childNodeId < model.nodes.size()) {
			loadNode(
				result,
				model,
				model.nodes[childNodeId],
				nodeTransform
#if PHYSICS_INTERFACE_PHYSX
				,pPhysXCooking
#endif
			);
		}
	}
}

static std::vector<LoadModelResult> loadModelAnyThreadPart(
	const tinygltf::Model& model,
	const glm::dmat4x4& transform
#if PHYSICS_INTERFACE_PHYSX
	,IPhysXCooking* pPhysXCooking
#endif
) {
	std::vector<LoadModelResult> result;

	glm::dmat4x4 rootTransform;
	if (model.extras.IsObject() && model.extras.Has("RTC_CENTER")) {
		const tinygltf::Value& rtcCenterValue = model.extras.Get("RTC_CENTER");
		const tinygltf::Value::Array& rtcCenterArray = rtcCenterValue.Get<tinygltf::Value::Array>();
		if (rtcCenterArray.size() == 3) {
			glm::dmat4x4 rtcTransform(
				glm::dvec4(1.0, 0.0, 0.0, 0.0),
				glm::dvec4(0.0, 1.0, 0.0, 0.0),
				glm::dvec4(0.0, 0.0, 1.0, 0.0),
				glm::dvec4(
					rtcCenterArray[0].GetNumberAsDouble(),
					rtcCenterArray[1].GetNumberAsDouble(),
					rtcCenterArray[2].GetNumberAsDouble(),
					1.0
				)
			);
			rootTransform = transform * rtcTransform * gltfAxesToCesiumAxes;
		} else {
			rootTransform = transform * gltfAxesToCesiumAxes;
		}
	} else {
		rootTransform = transform * gltfAxesToCesiumAxes;
	}

	if (model.defaultScene >= 0 && model.defaultScene < model.scenes.size()) {
		// Show the default scene
		const tinygltf::Scene& defaultScene = model.scenes[model.defaultScene];
		for (int nodeId : defaultScene.nodes) {
			loadNode(
				result,
				model,
				model.nodes[nodeId],
				rootTransform
#if PHYSICS_INTERFACE_PHYSX
				,pPhysXCooking
#endif
			);
		}
	} else if (model.scenes.size() > 0) {
		// There's no default, so show the first scene
		const tinygltf::Scene& defaultScene = model.scenes[0];
		for (int nodeId : defaultScene.nodes) {
			loadNode(
				result,
				model,
				model.nodes[nodeId],
				rootTransform
#if PHYSICS_INTERFACE_PHYSX
				,pPhysXCooking
#endif
			);
		}
	} else if (model.nodes.size() > 0) {
		// No scenes at all, use the first node as the root node.
		loadNode(
			result,
			model,
			model.nodes[0],
			rootTransform
#if PHYSICS_INTERFACE_PHYSX
			,pPhysXCooking
#endif
		);
	} else if (model.meshes.size() > 0) {
		// No nodes either, show all the meshes.
		for (const tinygltf::Mesh& mesh : model.meshes) {
			loadMesh(
				result,
				model,
				mesh,
				rootTransform
#if PHYSICS_INTERFACE_PHYSX
				,pPhysXCooking
#endif
			);
		}
	}

	return result;
}

template <class T>
bool applyTexture(UMaterialInstanceDynamic* pMaterial, FName parameterName, const tinygltf::Model& model, const T& gltfTexture) {
	if (gltfTexture.index < 0 || gltfTexture.index >= model.textures.size()) {
		// TODO: report invalid texture if the index isn't -1
		return false;
	}

	const tinygltf::Texture& texture = model.textures[gltfTexture.index];
	if (texture.source < 0 || texture.source >= model.images.size()) {
		// TODO: report invalid texture
		return false;
	}

	const tinygltf::Image& image = model.images[texture.source];

	UTexture2D* pTexture = UTexture2D::CreateTransient(image.width, image.height, PF_R8G8B8A8);

	unsigned char* pTextureData = static_cast<unsigned char*>(pTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
	FMemory::Memcpy(pTextureData, image.image.data(), image.image.size());
	pTexture->PlatformData->Mips[0].BulkData.Unlock();

	pTexture->UpdateResource();

	pMaterial->SetTextureParameterValue(parameterName, pTexture);

	return true;
}

static void loadModelGameThreadPart(UCesiumGltfComponent* pGltf, LoadModelResult& loadResult, const glm::dmat4x4& cesiumToUnrealTransform) {
	UCesiumGltfPrimitiveComponent* pMesh = NewObject<UCesiumGltfPrimitiveComponent>(pGltf, FName(loadResult.name.c_str()));
	pMesh->HighPrecisionNodeTransform = loadResult.transform;
	pMesh->UpdateTransformFromCesium(cesiumToUnrealTransform);

	pMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	pMesh->bUseDefaultCollision = true;
	//pMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	pMesh->SetFlags(RF_Transient);

	UStaticMesh* pStaticMesh = NewObject<UStaticMesh>();
	pMesh->SetStaticMesh(pStaticMesh);

	pStaticMesh->bIsBuiltAtRuntime = true;
	pStaticMesh->NeverStream = true;
	pStaticMesh->RenderData = TUniquePtr<FStaticMeshRenderData>(loadResult.RenderData);

	const tinygltf::Model& model = *loadResult.pModel;
	const tinygltf::Material& material = loadResult.pMaterial ? *loadResult.pMaterial : defaultMaterial;
	
	const tinygltf::PbrMetallicRoughness& pbr = material.pbrMetallicRoughness;

	const FName ImportedSlotName(*(TEXT("CesiumMaterial") + FString::FromInt(nextMaterialId++)));
	UMaterialInstanceDynamic* pMaterial = UMaterialInstanceDynamic::Create(pGltf->BaseMaterial, nullptr, ImportedSlotName);

	if (pbr.baseColorFactor.size() >= 3) {
		pMaterial->SetVectorParameterValue("baseColorFactor", FVector(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2]));
	}
	pMaterial->SetScalarParameterValue("metallicFactor", pbr.metallicFactor);
	pMaterial->SetScalarParameterValue("roughnessFactor", pbr.roughnessFactor);

	applyTexture(pMaterial, "baseColorTexture", model, pbr.baseColorTexture);
	applyTexture(pMaterial, "metallicRoughnessTexture", model, pbr.metallicRoughnessTexture);
	applyTexture(pMaterial, "normalTexture", model, material.normalTexture);
	bool hasEmissiveTexture = applyTexture(pMaterial, "emissiveTexture", model, material.emissiveTexture);
	applyTexture(pMaterial, "occlusionTexture", model, material.occlusionTexture);

	if (material.emissiveFactor.size() >= 3) {
		pMaterial->SetVectorParameterValue("emissiveFactor", FVector(material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2]));
	} else if (hasEmissiveTexture) {
		// When we have an emissive texture but not a factor, we need to use a factor of vec3(1.0). The default,
		// vec3(0.0), would disable the emission from the texture.
		pMaterial->SetVectorParameterValue("emissiveFactor", FVector(1.0f, 1.0f, 1.0f));
	}

	pMaterial->TwoSided = true;

	pStaticMesh->AddMaterial(pMaterial);

	pStaticMesh->InitResources();

	// Set up RenderData bounds and LOD data
	pStaticMesh->CalculateExtendedBounds();

	pStaticMesh->RenderData->ScreenSize[0].Default = 1.0f;
	pStaticMesh->CreateBodySetup();

	//pMesh->UpdateCollisionFromStaticMesh();
	pMesh->GetBodySetup()->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;

#if PHYSICS_INTERFACE_PHYSX
	if (loadResult.pCollisionMesh) {
		pMesh->GetBodySetup()->TriMeshes.Add(loadResult.pCollisionMesh);
		pMesh->GetBodySetup()->bCreatedPhysicsMeshes = true;
	}
#endif

	pMesh->SetMobility(EComponentMobility::Movable);

	//pMesh->bDrawMeshCollisionIfComplex = true;
	//pMesh->bDrawMeshCollisionIfSimple = true;
	pMesh->SetupAttachment(pGltf);
	pMesh->RegisterComponent();
}

/*static*/ void UCesiumGltfComponent::CreateOffGameThread(AActor* pActor, const tinygltf::Model& model, const glm::dmat4x4& transform, TFunction<void(UCesiumGltfComponent*)> callback) {
	std::vector<LoadModelResult> result = loadModelAnyThreadPart(
		model,
		transform
#if PHYSICS_INTERFACE_PHYSX
		,nullptr
#endif
	);

	AsyncTask(ENamedThreads::GameThread, [pActor, callback, result{ std::move(result) }]() mutable {
		UCesiumGltfComponent* Gltf = NewObject<UCesiumGltfComponent>(pActor);
		for (LoadModelResult& model : result) {
			loadModelGameThreadPart(Gltf, model, CesiumTransforms::unrealToOrFromCesium * CesiumTransforms::scaleToUnrealWorld);
		}
		Gltf->SetVisibility(false, true);
		callback(Gltf);
	});
}

namespace {
	class HalfConstructedReal : public UCesiumGltfComponent::HalfConstructed {
	public:
		virtual ~HalfConstructedReal() = default;
		std::vector<LoadModelResult> loadModelResult;
	};
}

/*static*/ std::unique_ptr<UCesiumGltfComponent::HalfConstructed>
UCesiumGltfComponent::CreateOffGameThread(
	const tinygltf::Model& model,
	const glm::dmat4x4& transform
#if PHYSICS_INTERFACE_PHYSX
	,IPhysXCooking* pPhysXCooking
#endif
) {
	auto pResult = std::make_unique<HalfConstructedReal>();
	pResult->loadModelResult = std::move(loadModelAnyThreadPart(
		model,
		transform
#if PHYSICS_INTERFACE_PHYSX
		,pPhysXCooking
#endif
	));
	return pResult;
}

/*static*/ UCesiumGltfComponent*
UCesiumGltfComponent::CreateOnGameThread(
	AActor* pParentActor,
	std::unique_ptr<HalfConstructed> pHalfConstructed,
	const glm::dmat4x4& cesiumToUnrealTransform
) {
	HalfConstructedReal* pReal = static_cast<HalfConstructedReal*>(pHalfConstructed.get());
	std::vector<LoadModelResult>& result = pReal->loadModelResult;
	if (result.size() == 0) {
		return nullptr;
	}

	UCesiumGltfComponent* Gltf = NewObject<UCesiumGltfComponent>(pParentActor);
	Gltf->SetUsingAbsoluteLocation(true);
	Gltf->SetFlags(RF_Transient);
	for (LoadModelResult& model : result) {
		loadModelGameThreadPart(Gltf, model, cesiumToUnrealTransform);
	}
	Gltf->SetVisibility(false, true);
	return Gltf;
}

UCesiumGltfComponent::UCesiumGltfComponent() 
	: USceneComponent()
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinder<UMaterial> BaseMaterial;
		FConstructorStatics() :
			BaseMaterial(TEXT("/Cesium/GltfMaterialWithOverlays.GltfMaterialWithOverlays"))
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

void UCesiumGltfComponent::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) {
	USceneComponent::ApplyWorldOffset(InOffset, bWorldShift);

	const FIntVector& originLocation = this->GetWorld()->OriginLocation;
	glm::dvec3 offset = glm::dvec3(InOffset.X, InOffset.Y, InOffset.Z);
	glm::dvec3 newOrigin = glm::dvec3(originLocation.X, originLocation.Y, originLocation.Z);
	newOrigin -= offset;


}

void UCesiumGltfComponent::UpdateTransformFromCesium(const glm::dmat4& cesiumToUnrealTransform) {
	for (USceneComponent* pSceneComponent : this->GetAttachChildren()) {
		UCesiumGltfPrimitiveComponent* pPrimitive = Cast<UCesiumGltfPrimitiveComponent>(pSceneComponent);
		if (pPrimitive) {
			pPrimitive->UpdateTransformFromCesium(cesiumToUnrealTransform);
		}
	}
}

void UCesiumGltfComponent::AttachRasterTile(
	const Cesium3DTiles::Tile& tile,
	const Cesium3DTiles::RasterOverlayTile& rasterTile,
	UTexture2D* pTexture,
	const CesiumGeometry::Rectangle& textureCoordinateRectangle,
	const glm::dvec2& translation,
	const glm::dvec2& scale
) {
	if (this->_overlayTiles.Num() == 0) {
		// First overlay tile, generate texture coordinates
		// TODO
	}

	this->_overlayTiles.Add(FRasterOverlayTile{
		pTexture,
		FLinearColor(textureCoordinateRectangle.minimumX, textureCoordinateRectangle.minimumY, textureCoordinateRectangle.maximumX, textureCoordinateRectangle.maximumY),
		FLinearColor(translation.x, translation.y, scale.x, scale.y)
	});

	if (this->_overlayTiles.Num() > 3) {
		UE_LOG(LogActor, Warning, TEXT("Too many raster overlays"));
	}

	this->updateRasterOverlays();
}

void UCesiumGltfComponent::DetachRasterTile(
	const Cesium3DTiles::Tile& tile,
	const Cesium3DTiles::RasterOverlayTile& rasterTile,
	UTexture2D* pTexture,
	const CesiumGeometry::Rectangle& textureCoordinateRectangle
) {
	this->_overlayTiles.RemoveAll([pTexture, &textureCoordinateRectangle](const FRasterOverlayTile& tile) {
		return
			tile.pTexture == pTexture &&
			tile.textureCoordinateRectangle.Equals(FLinearColor(textureCoordinateRectangle.minimumX, textureCoordinateRectangle.minimumY, textureCoordinateRectangle.maximumX, textureCoordinateRectangle.maximumY));
	});

	this->updateRasterOverlays();
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
		std::unique_ptr<Cesium3DTiles::Gltf::LoadResult> pLoadResult = std::make_unique<Cesium3DTiles::Gltf::LoadResult>(std::move(Cesium3DTiles::Gltf::load(data)));

		if (pLoadResult->warnings.length() > 0) {
			UE_LOG(LogActor, Warning, TEXT("Warnings while loading glTF: %s"), *utf8_to_wstr(pLoadResult->warnings));
		}

		if (pLoadResult->errors.length() > 0) {
			UE_LOG(LogActor, Error, TEXT("Errors while loading glTF: %s"), *utf8_to_wstr(pLoadResult->errors));
		}

		if (!pLoadResult->model) {
			UE_LOG(LogActor, Error, TEXT("glTF model could not be loaded."));
			return;
		}

		tinygltf::Model& model = pLoadResult->model.value();

		std::vector<LoadModelResult> result = loadModelAnyThreadPart(
			model,
			glm::dmat4x4(1.0)
#if PHYSICS_INTERFACE_PHYSX
			,nullptr
#endif
		);

		AsyncTask(ENamedThreads::GameThread, [this, pLoadResult{ std::move(pLoadResult) }, result{ std::move(result) }]() mutable {
			for (LoadModelResult& model : result) {
				loadModelGameThreadPart(this, model, CesiumTransforms::unrealToOrFromCesium * CesiumTransforms::scaleToUnrealWorld);
			}
		});
	});
}

void UCesiumGltfComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//this->Mesh->DestroyComponent();
	//this->Mesh = nullptr;
}

void UCesiumGltfComponent::updateRasterOverlays() {
	for (USceneComponent* pSceneComponent : this->GetAttachChildren()) {
		UCesiumGltfPrimitiveComponent* pPrimitive = Cast<UCesiumGltfPrimitiveComponent>(pSceneComponent);
		if (pPrimitive) {
			UMaterialInstanceDynamic* pMaterial = Cast<UMaterialInstanceDynamic>(pPrimitive->GetMaterial(0));

			for (size_t i = 0; i < this->_overlayTiles.Num(); ++i) {
				FRasterOverlayTile& overlayTile = this->_overlayTiles[i];
				std::string is = std::to_string(i + 1);
				pMaterial->SetTextureParameterValue(("OverlayTexture" + is).c_str(), overlayTile.pTexture);
				
				if (!overlayTile.pTexture) {
					// The texture is null so don't use it.
					pMaterial->SetVectorParameterValue(("OverlayRect" + is).c_str(), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
				} else {
					pMaterial->SetVectorParameterValue(("OverlayRect" + is).c_str(), overlayTile.textureCoordinateRectangle);
				}

				pMaterial->SetVectorParameterValue(("OverlayTranslationScale" + is).c_str(), overlayTile.translationAndScale);
			}

			for (size_t i = this->_overlayTiles.Num(); i < 3; ++i) {
				std::string is = std::to_string(i + 1);
				pMaterial->SetTextureParameterValue(("OverlayTexture" + is).c_str(), nullptr);
				pMaterial->SetVectorParameterValue(("OverlayRect" + is).c_str(), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
				pMaterial->SetVectorParameterValue(("OverlayTranslationScale" + is).c_str(), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			}
		}
	}
}

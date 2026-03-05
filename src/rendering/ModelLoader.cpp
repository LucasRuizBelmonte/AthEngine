#pragma region Includes
#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <vector>
#include <stdexcept>
#pragma endregion

#pragma region Function Definitions
ModelLoader::MeshData ModelLoader::LoadFirstMeshData(const std::string &path)
{
	Assimp::Importer importer;

	const aiScene *scene = importer.ReadFile(
		path, aiProcess_Triangulate |
		          aiProcess_CalcTangentSpace |
		          aiProcess_JoinIdenticalVertices |
		          aiProcess_GenNormals |
		          aiProcess_ImproveCacheLocality |
		          aiProcess_PreTransformVertices);

	if (!scene || !scene->HasMeshes())
		throw std::runtime_error("Assimp failed to load mesh: " + path);

	MeshData meshData;
	meshData.vertices.reserve(10000);
	meshData.indices.reserve(10000);

	uint32_t vertexOffset = 0;

	for (unsigned m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh *src = scene->mMeshes[m];

		for (unsigned i = 0; i < src->mNumVertices; ++i)
		{
			const aiVector3D &p = src->mVertices[i];
			const aiVector3D uv = src->HasTextureCoords(0) ? src->mTextureCoords[0][i] : aiVector3D(0.f, 0.f, 0.f);
			const aiVector3D n = src->HasNormals() ? src->mNormals[i] : aiVector3D(0.f, 0.f, 1.f);
			const aiVector3D t = src->HasTangentsAndBitangents() ? src->mTangents[i] : aiVector3D(1.f, 0.f, 0.f);

			meshData.vertices.push_back(VertexPUNT{
				p.x * 0.01f,
				p.y * 0.01f,
				p.z * 0.01f,
				uv.x, uv.y,
				n.x, n.y, n.z,
				t.x, t.y, t.z});
		}

		for (unsigned i = 0; i < src->mNumFaces; ++i)
		{
			const aiFace &f = src->mFaces[i];
			if (f.mNumIndices != 3)
				continue;

			meshData.indices.push_back(vertexOffset + f.mIndices[0]);
			meshData.indices.push_back(vertexOffset + f.mIndices[1]);
			meshData.indices.push_back(vertexOffset + f.mIndices[2]);
		}

		vertexOffset += src->mNumVertices;
	}

	return meshData;
}
#pragma endregion

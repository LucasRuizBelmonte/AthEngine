#pragma region Includes
#include "../platform/GL.h"
#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <vector>
#include <stdexcept>
#include <iostream>
#pragma endregion

#pragma region File Scope
namespace
{
    struct VertexPUNT
    {
        float px, py, pz;
        float u, v;
        float nx, ny, nz;
        float tx, ty, tz;
    };
}
#pragma endregion

#pragma region Function Definitions
Mesh ModelLoader::LoadFirstMesh(const std::string &path)
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

    std::vector<VertexPUNT> vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(10000);
    indices.reserve(10000);

    unsigned int vertexOffset = 0;

    for (unsigned m = 0; m < scene->mNumMeshes; ++m)
    {
        const aiMesh *src = scene->mMeshes[m];

        for (unsigned i = 0; i < src->mNumVertices; ++i)
        {
            const aiVector3D &p = src->mVertices[i];
            const aiVector3D uv = src->HasTextureCoords(0) ? src->mTextureCoords[0][i] : aiVector3D(0.f, 0.f, 0.f);
            const aiVector3D n = src->HasNormals() ? src->mNormals[i] : aiVector3D(0.f, 0.f, 1.f);
            const aiVector3D t = src->HasTangentsAndBitangents() ? src->mTangents[i] : aiVector3D(1.f, 0.f, 0.f);

            vertices.push_back(VertexPUNT{
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

            indices.push_back(vertexOffset + f.mIndices[0]);
            indices.push_back(vertexOffset + f.mIndices[1]);
            indices.push_back(vertexOffset + f.mIndices[2]);
        }

        vertexOffset += src->mNumVertices;
    }

    Mesh mesh{};

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(VertexPUNT),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPUNT), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPUNT), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPUNT), (void *)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPUNT), (void *)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    mesh.indexCount = static_cast<GLsizei>(indices.size());

    return mesh;
}
#pragma endregion

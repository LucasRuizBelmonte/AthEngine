#include "../platform/GL.h"
#include "ModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <vector>
#include <stdexcept>
#include <iostream>

namespace
{
    struct VertexPC
    {
        float px, py, pz;
        float cr, cg, cb;
    };
}

Mesh ModelLoader::LoadFirstMesh(const std::string &path)
{
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(
        path, aiProcess_Triangulate |
                  aiProcess_JoinIdenticalVertices |
                  aiProcess_GenNormals |
                  aiProcess_ImproveCacheLocality |
                  aiProcess_PreTransformVertices);

    if (!scene || !scene->HasMeshes())
        throw std::runtime_error("Assimp failed to load mesh: " + path);

    struct VertexPC
    {
        float px, py, pz;
        float cr, cg, cb;
    };

    std::vector<VertexPC> vertices;
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

            vertices.push_back(VertexPC{
                p.x * 0.01f,
                p.y * 0.01f,
                p.z * 0.01f,
                1.0f, 1.0f, 1.0f});
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
                 vertices.size() * sizeof(VertexPC),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPC), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPC), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    mesh.indexCount = static_cast<GLsizei>(indices.size());

    return mesh;
}
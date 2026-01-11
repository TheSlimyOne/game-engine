// MeshManager.cpp

#include "MeshManager.h"
#include "Vertex.h"
#include "MeshData.h"

#include <iostream>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <bgfx/bgfx.h>


std::shared_ptr<MeshData> MeshManager::load(const std::string &file_path) {
    // Check if the mesh is already in our cache
    auto it = m_mesh_cache.find(file_path);
    if (it != m_mesh_cache.end()) {
        // If it is in the cache, try to "lock" the weak_ptr to get a shared_ptr
        if (auto shared = it->second.lock()) {
            // Resource is still alive, return it
            return shared;
        } else {
            m_mesh_cache.erase(it); // clean up expired weak pointers
        }
    }

    // If it's not in the cache or has expired, load it from the file
    std::cout << "loading" << file_path << std::endl;
    auto new_mesh = load_from_file(file_path);
    if (new_mesh) {
        // Store a new weak pointer to the mesh in the cache
        m_mesh_cache[file_path] = new_mesh;
    }

    return new_mesh;
}

std::shared_ptr<MeshData> MeshManager::load_from_file(const std::string &file_path) {
     Assimp::Importer importer;

    // aiProcess_GenNormals ensures mNormals exists.
    // aiProcess_FlipUVs matches OpenGL/BGFX UV convention.
    const aiScene* scene = importer.ReadFile(
        file_path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return nullptr;
    }

    aiMesh* mesh = scene->mMeshes[0];

    std::vector<Vertex> vertices;
    vertices.reserve(mesh->mNumVertices); // optimization

    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        // 1. Copy Position
        vertex.x = mesh->mVertices[i].x;
        vertex.y = mesh->mVertices[i].y;
        vertex.z = mesh->mVertices[i].z;

        // 2. Copy Normals (MISSING IN YOUR CODE)
        // Assimp guarantees this exists because of aiProcess_GenNormals
        if (mesh->HasNormals()) {
            vertex.nx = mesh->mNormals[i].x;
            vertex.ny = mesh->mNormals[i].y;
            vertex.nz = mesh->mNormals[i].z;
        } else {
            vertex.nx = 0.0f; vertex.ny = 1.0f; vertex.nz = 0.0f;
        }

        // 3. Copy UVs / Texture Coordinates (MISSING IN YOUR CODE)
        // Check if texture channel 0 exists
        if (mesh->HasTextureCoords(0)) {
            vertex.u = mesh->mTextureCoords[0][i].x;
            vertex.v = mesh->mTextureCoords[0][i].y;
        } else {
            vertex.u = 0.0f;
            vertex.v = 0.0f;
        }

        vertices.push_back(vertex);
    }

    std::vector<uint16_t> indices;
    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            indices.push_back(static_cast<uint16_t>(face.mIndices[j]));
        }
    }

    auto layout = Vertex::vertex_layout();
    auto mesh_data = std::make_shared<MeshData>();

    // Copy to BGFX memory
    mesh_data->vbh = bgfx::createVertexBuffer(bgfx::copy(vertices.data(), vertices.size() * sizeof(Vertex)), layout);
    mesh_data->ibh = bgfx::createIndexBuffer(bgfx::copy(indices.data(), indices.size() * sizeof(uint16_t)));

    if (!bgfx::isValid(mesh_data->vbh) || !bgfx::isValid(mesh_data->ibh)) {
        std::cerr << "Failed to create vertex and/or index buffers." << std::endl;
        return nullptr;
    }

    return mesh_data;
}

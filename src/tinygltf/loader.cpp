#include "loader.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"


bool LoadModel(const std::string& filename, 
               std::vector<Vertex>& vertices, 
               std::vector<uint32_t>& indices) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    if (!warn.empty()) {
        std::cout << "Warning: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cerr << "Error: " << err << std::endl;
    }
    if (!ret) {
        std::cerr << "Failed to load GLTF: " << filename << std::endl;
        return false;
    }

    if (model.meshes.empty()) {
        std::cerr << "No meshes found in GLTF file" << std::endl;
        return false;
    }
    
    const tinygltf::Mesh& mesh = model.meshes[0];
    if (mesh.primitives.empty()) {
        std::cerr << "No primitives found in mesh" << std::endl;
        return false;
    }
    
    const tinygltf::Primitive& primitive = mesh.primitives[0];

    if (primitive.indices < 0) {
        std::cerr << "Primitive has no indices" << std::endl;
        return false;
    }
    
    const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
    const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
    const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
    
    const void* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
    const size_t indexCount = indexAccessor.count;

    indices.resize(indexCount);
    if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        const uint16_t* src = static_cast<const uint16_t*>(indexData);
        for (size_t i = 0; i < indexCount; ++i) {
            indices[i] = static_cast<uint32_t>(src[i]);
        }
    } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        memcpy(indices.data(), indexData, indexCount * sizeof(uint32_t));
    } else {
        std::cerr << "Unsupported index component type" << std::endl;
        return false;
    }

    auto posIt = primitive.attributes.find("POSITION");
    if (posIt == primitive.attributes.end()) {
        std::cerr << "Primitive has no POSITION attribute" << std::endl;
        return false;
    }
    
    const tinygltf::Accessor& posAccessor = model.accessors[posIt->second];
    const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
    const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
    
    const float* posData = reinterpret_cast<const float*>(
        &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);
    const size_t vertexCount = posAccessor.count;

    vertices.resize(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i) {
        vertices[i].pos = glm::vec3(
            posData[i * 3 + 0],
            posData[i * 3 + 1],
            posData[i * 3 + 2]
        );
    }

    return true;
}
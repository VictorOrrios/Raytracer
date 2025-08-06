#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <iostream>

enum LightTypes{
    AMBIENT = 0,
    SPHERE = 1,
    POINT = 2,
    DIRECTIONAL = 3,
    CONE = 4,
    AREA = 5,
};


struct alignas(16) Sphere{
    glm::vec3 pos;
    float r;
    int mat;
};

struct alignas(16) Triangle{
    alignas(16) glm::vec3 v0;
    alignas(16) glm::vec3 v1;
    alignas(16) glm::vec3 v2;
    alignas(16) glm::vec3 normal;
    int mat;
};

struct Vertex{
    alignas(16) glm::vec3 pos;
};

// Based on Blender 4.5LTS Principled BSDF
struct Material {
    glm::vec4 albedo;           // Surface color rgb. Alpha controls opacity: 0.0 = transparent 1.0 = opaque

    // Subsurface | Method: Christensen-Burley
    glm::vec4 subsurface;       // Average distance the ligths scatter below the surface
                                // Alpha controls weight: 0.0 = diffuse , 1.0 = subsurface

    // Specular | Method: GGX
    glm::vec4 specular_tint;    // Color tint for specular and metalic relfections
                                // Alpha controls IOR Level: 
                                // 0.0 = no reflections , 0.5 = no adjustment, 1.0 = double reflections

    // Emission
    glm::vec4 emission_color;   // Color of the light emited. Alpha controls strength

    // General
    float roughness;            // 0.0 = smooth , 1.0 = rough
    float metallic;             // 0.0 = dielectric , 1.0 = metalic
    float ior;                  // Index of refraction

    // Transmission
    float trs_weight;           // 0.0 = opaque , 1.0 = transmisive

    // Coat
    // TODO

    // Sheen
    // TODO
};


struct alignas(16) Light{
    glm::vec4 pos_angle_aux;
    glm::vec4 color_str;
    int type;
    float accumulated_str;
};

struct MeshInfo{
    uint index_start;
    uint index_end;
    int material;
};

struct Model{
    std::string file_name;
    glm::vec3 pos;
    float pitch;
    float yaw;
    float roll;
    float scale;
    int material;
};
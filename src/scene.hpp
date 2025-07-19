#include <glm/glm.hpp>
#include <vector>

enum MaterialsFunctions{
    DIFFUSE = 1,
    METAL = 2,
    TRANSMISSIVE = 3, 
    SUBSURFACE = 4, 
    EMITER = 5,
};

struct alignas(16) Sphere{
    glm::vec3 pos;
    float r;
    int mat;
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

struct Light{
    int type;
    float str;
    float aux1;
    float aux2;
};

class Scene{
public:
    std::vector<Sphere> sphereVec;
    std::vector<Material> materialVec;
    
    Scene();
private:
    void addSphere(Sphere s);
    int addMaterial(Material m);
};



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

struct Material {
    glm::vec4 albedo;           // Base color rgb

    glm::vec4 emission_color;   // Rgb color of emited light, 0.0 for non emited
    
    // Shared rgb color for transmision or subsurface scattering
    glm::vec4 transmission_subsurface_color;  
    
    float metalness;            // 0.0 = dielectric , 1.0 = metalic
    float roughness;            // 0.0 = smooth , 1.0 = rough
    
    // Shared parameter:
    // - If transmission => opacity: 0.0 = transparent, 1.0 = opaque
    // - If subsurface   => subsurface strength: intensity of dispersion
    float opacity_subsurface_strength;   
    
    float ior;                  // Index of refraction
    
    int type;                   // Flag to know what type of material is this
    int _padding3[3];
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



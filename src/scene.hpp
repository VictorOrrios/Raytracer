#include <glm/glm.hpp>
#include <vector>

enum MaterialsFunctions{
    LAMBERTIAN = 1,
    METAL = 2
};

struct alignas(16) Sphere{
    glm::vec3 pos;
    float r;
    int mat;
};

struct alignas(16) Material{
    glm::vec3 albedo;
    float reflectance;
    int function;
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



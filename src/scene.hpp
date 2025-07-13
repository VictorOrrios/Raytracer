#include <glm/glm.hpp>
#include <vector>

enum MaterialsFunctions{
    LAMBERTIAN = 1, // aux1 = unused
    METAL = 2, // aux1 = fuzziness
    DIFFUSE = 3, // aux1 = unused
    DIELECTRIC = 4, // aux1 = refraction index
};

struct alignas(16) Sphere{
    glm::vec3 pos;
    float r;
    int mat;
};

struct alignas(16) Material{
    glm::vec3 albedo;
    float aux1;
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



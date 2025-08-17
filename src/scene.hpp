#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include "definitions.hpp"

const std::string ASSETS_DIRECTORY = "assets/";

class Scene{
public:
    std::vector<Sphere> sphereVec;
    std::vector<Material> materialVec;
    std::vector<Light> lightsVec;
    std::vector<Triangle> triangleVec;
    std::vector<Vertex> vertexVec;
    std::vector<uint32_t> indexVec;
    std::vector<MeshInfo> meshVec;
    float lights_strength_sum = 0.0;
    int total_lights = 0;
    int total_spheres = 0;
    int total_triangles = 0;
    int total_meshes = 0;
    
    Scene();
    void createPreset1();
    void createCornellBox();

private:
    void addSphere(Sphere s);
    int addMaterial(Material m);
    void addLight(Light l);
    void addTriangle(Triangle t);
    void addQuad(Quad q);
    void printLight(const Light& light);
    void addModel(Model model);
    glm::vec3 calculateNormal(Triangle t);
    void printSceneInfo();
};



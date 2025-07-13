#include "scene.hpp"


Scene::Scene(){

    int ground = addMaterial({
        albedo: glm::vec3(0.129, 0.388, 0.082),
        reflectance: 0.5,
        function: LAMBERTIAN
    });

    int blueMatte = addMaterial({
        albedo: glm::vec3(0.208, 0.612, 0.8),
        reflectance: 0.5,
        function: LAMBERTIAN
    });

    int redMetal = addMaterial({
        albedo: glm::vec3(0.82, 0.118, 0.118),
        reflectance: 0.5,
        function: METAL
    });

    int yellowMetal = addMaterial({
        albedo: glm::vec3(0.945, 0.949, 0.212),
        reflectance: 0.5,
        function: METAL
    });

    int whiteMetal = addMaterial({
        albedo: glm::vec3(1.0, 1.0, 1.0),
        reflectance: 0.5,
        function: METAL
    });

    addSphere({
        pos: glm::vec3(0.0,-1000.0,-10.0),
        r: 999.0,
        mat: ground
    });

    addSphere({
        pos: glm::vec3(0.0,0.0,-10.0),
        r: 1.0,
        mat: blueMatte
    });

    addSphere({
        pos: glm::vec3(0.9,-0.8,-10.0),
        r: 0.2,
        mat: whiteMetal
    });

    addSphere({
        pos: glm::vec3(0.7,-0.8,-10.2),
        r: 0.2,
        mat: whiteMetal
    });

    addSphere({
        pos: glm::vec3(3.0,0.0,-10.0),
        r: 1.0,
        mat: redMetal
    });

    addSphere({
        pos: glm::vec3(-3.0,0.0,-10.0),
        r: 1.0,
        mat: yellowMetal
    });

}

void Scene::addSphere(Sphere s){
    sphereVec.push_back(s);
}

int Scene::addMaterial(Material m){
    materialVec.push_back(m);
    return materialVec.size()-1;
}
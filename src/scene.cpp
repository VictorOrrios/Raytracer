#include "scene.hpp"
#include <random>
#include <iostream>

int randomInt(int min, int max) {
    static std::random_device rd;  
    static std::mt19937 gen(rd());
    
    std::uniform_int_distribution<int> distrib(min, max);
    return distrib(gen);
}

float randomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_real_distribution<float> distrib(min, max);
    return distrib(gen);
}


Scene::Scene(){

    int ground = addMaterial({
        albedo: glm::vec3(0.129, 0.388, 0.082),
        aux1: 0.5,
        function: LAMBERTIAN
    });

    int blueMatte = addMaterial({
        albedo: glm::vec3(0.208, 0.612, 0.8),
        aux1: 0.5,
        function: LAMBERTIAN
    });

    int redMetal = addMaterial({
        albedo: glm::vec3(0.82, 0.118, 0.118),
        aux1: 0.8,
        function: METAL
    });

    int yellowMetal = addMaterial({
        albedo: glm::vec3(0.945, 0.949, 0.212),
        aux1: 0.1,
        function: METAL
    });

    int whiteMetal = addMaterial({
        albedo: glm::vec3(1.0, 1.0, 1.0),
        aux1: 0.0,
        function: METAL
    });

    int blueDiffuse = addMaterial({
        albedo: glm::vec3(0.208, 0.612, 0.8),
        aux1: 0.0,
        function: DIFFUSE
    });

    // Glass
    int dielectric150 = addMaterial({
        albedo: glm::vec3(0.0, 0.0, 0.0),
        aux1: 1.5,
        function: DIELECTRIC
    });

    // Hyper refractable
    int dielectric075 = addMaterial({
        albedo: glm::vec3(0.0, 0.0, 0.0),
        aux1: 0.75,
        function: DIELECTRIC
    });

    // Water
    int dielectric133 = addMaterial({
        albedo: glm::vec3(0.0, 0.0, 0.0),
        aux1: 1.33,
        function: DIELECTRIC
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

    addSphere({
        pos: glm::vec3(0.0,0.01,-7.8),
        r: 1.0,
        mat: dielectric150
    });

    addSphere({
        pos: glm::vec3(0.0,0.01,-7.8),
        r: 0.8,
        mat: dielectric075
    });

    addSphere({
        pos: glm::vec3(2.005,0.01,-7.8),
        r: 1.0,
        mat: dielectric133
    });

    addSphere({
        pos: glm::vec3(-2.005,0.01,-7.8),
        r: 1.0,
        mat: dielectric075
    });


    /*
    const float randomRange = 10.0; 
    for(int i = 0; i < 500; i++){
        addSphere({
            pos: glm::vec3(randomFloat(-randomRange,randomRange),randomFloat(-randomRange,randomRange),randomFloat(-randomRange,randomRange)),
            r: randomFloat(0.1,2.0),
            mat: randomInt(0,materialVec.size()-1)
        });
    }
    */

}

void Scene::addSphere(Sphere s){
    sphereVec.push_back(s);
}

int Scene::addMaterial(Material m){
    materialVec.push_back(m);
    return materialVec.size()-1;
}
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
        albedo: glm::vec4(0.129, 0.388, 0.082, 1.0),
        subsurface: glm::vec4(0.0),
        specular_tint: glm::vec4(0.0),
        emission_color: glm::vec4(0.0),
        roughness: 0.8,
        metallic: 0.0,
        ior: 1.0,
        trs_weight: 0.0,
    });

    int blueMatte = addMaterial({
        albedo: glm::vec4(0.133, 0.42, 0.878, 1.0),
        subsurface: glm::vec4(0.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,1.0),
        emission_color: glm::vec4(0.0),
        roughness: 0.3,
        metallic: 0.0,
        ior: 1.5,
        trs_weight: 0.0,
    });

    // Ground sphere
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



    /*

    addSphere({
        pos: glm::vec3(0.0,0.0,-10.0),
        r: 1.0,
        mat: gold
    });

    addSphere({
        pos: glm::vec3(2.1,0.0,-10.0),
        r: 1.0,
        mat: red_plastic
    });

    addSphere({
        pos: glm::vec3(4.2,0.0,-10.0),
        r: 1.0,
        mat: frosted_glass
    });

    addSphere({
        pos: glm::vec3(6.3,0.0,-10.0),
        r: 1.0,
        mat: neon_light
    });

    addSphere({
        pos: glm::vec3(-2.1,0.0,-10.0),
        r: 1.0,
        mat: human_skin
    });

    addSphere({
        pos: glm::vec3(-4.2,0.0,-10.0),
        r: 1.0,
        mat: water
    });

    addSphere({
        pos: glm::vec3(-6.3,0.0,-10.0),
        r: 1.0,
        mat: asphalt
    });

    */
    
    

    /*

    addSphere({
        pos: glm::vec3(0.0,0.0,-10.0),
        r: 1.0,
        mat: blueMatte
    });

    addSphere({
        pos: glm::vec3(0.9,-0.8,-10.0),
        r: 0.2,
        mat: mirror
    });

    addSphere({
        pos: glm::vec3(0.7,-0.8,-10.2),
        r: 0.2,
        mat: mirror
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
        mat: glass
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

    addSphere({
        pos: glm::vec3(-2.5,-0.7,-6.0),
        r: 0.3,
        mat: warm_ligth
    });

    addSphere({
        pos: glm::vec3(-1.0,-0.7,-10.0),
        r: 0.3,
        mat: white_ligth
    });

    addSphere({
        pos: glm::vec3(1.0,-0.7,-10.0),
        r: 0.3,
        mat: blue_ligth
    });

    */

}

void Scene::addSphere(Sphere s){
    sphereVec.push_back(s);
}

int Scene::addMaterial(Material m){
    materialVec.push_back(m);
    return materialVec.size()-1;
}
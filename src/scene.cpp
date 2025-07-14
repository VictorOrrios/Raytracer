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
        emission_color: glm::vec4(0.0),
        transmission_subsurface_color: glm::vec4(0.0),
        metalness: 0.0,
        roughness: 0.8,
        opacity_subsurface_strength: 1.0,
        ior: 1.5,
        type: DIFFUSE,
    });

    int diffuse = addMaterial({
        albedo: glm::vec4(0.76, 0.2, 0.16, 1.0),
        emission_color: glm::vec4(0.0),
        transmission_subsurface_color: glm::vec4(0.0),
        metalness: 0.0,
        roughness: 0.8,
        opacity_subsurface_strength: 1.0,
        ior: 1.5,
        type: DIFFUSE,
    });
    
    int mirror = addMaterial({
        albedo: glm::vec4(1.0),
        emission_color: glm::vec4(0.0),
        transmission_subsurface_color: glm::vec4(0.0),
        metalness: 1.0,
        roughness: 0.0,
        opacity_subsurface_strength: 1.0,
        ior: 1.5,
        type: METAL,
    });

    int glass = addMaterial({
        albedo: glm::vec4(1.0),
        emission_color: glm::vec4(0.0),
        transmission_subsurface_color: glm::vec4(1.0),
        metalness: 0.0,
        roughness: 0.0,
        opacity_subsurface_strength: 0.1,
        ior: 1.50,
        type: TRANSMISSIVE,
    });


    int subsurface = addMaterial({
        albedo: glm::vec4(0.799, 0.485, 0.347, 1.0),
        emission_color: glm::vec4(0.0),
        transmission_subsurface_color: glm::vec4(1.0, 0.7, 0.7, 1.0),
        metalness: 0.0,
        roughness: 0.3,
        opacity_subsurface_strength: 0.5,
        ior: 1.4,
        type: SUBSURFACE,
    });

    int warm_ligth = addMaterial({
        albedo: glm::vec4(1.0),
        emission_color: glm::vec4(0.984, 0.882, 0.337, 1.0),
        transmission_subsurface_color: glm::vec4(0.0),
        metalness: 0.0,
        roughness: 1.0,
        opacity_subsurface_strength: 1.0,
        ior: 1.0,
        type: EMITER,
    });

    int white_ligth = addMaterial({
        albedo: glm::vec4(1.0),
        emission_color: glm::vec4(1.0),
        transmission_subsurface_color: glm::vec4(0.0),
        metalness: 0.0,
        roughness: 1.0,
        opacity_subsurface_strength: 1.0,
        ior: 1.0,
        type: EMITER,
    });

    int blue_ligth = addMaterial({
        albedo: glm::vec4(1.0),
        emission_color: glm::vec4(0.101, 0.643, 0.835, 1.0),
        transmission_subsurface_color: glm::vec4(0.0),
        metalness: 0.0,
        roughness: 1.0,
        opacity_subsurface_strength: 1.0,
        ior: 1.0,
        type: EMITER,
    });

    int hybrid = addMaterial({
        albedo: glm::vec4(0.301, 0.533, 0.266, 1.0),
        emission_color: glm::vec4(0.0),
        transmission_subsurface_color: glm::vec4(0.227, 0.725, 0.850, 1.0),
        metalness: 0.0,
        roughness: 0.2,
        opacity_subsurface_strength: 0.3,
        ior: 1.333,
        type: TRANSMISSIVE,
    });

    int blueMatte = addMaterial({
        albedo: glm::vec4(0.208, 0.612, 0.8, 1.0),
        emission_color: glm::vec4(0.0),
        transmission_subsurface_color: glm::vec4(0.0),
        metalness: 0.0,
        roughness: 0.2,
        opacity_subsurface_strength: 1.0,
        ior: 1.333,
        type: DIFFUSE,
    });

    int redMetal = addMaterial({
        albedo: glm::vec4(0.82, 0.118, 0.118, 1.0),
        emission_color: glm::vec4(0.0),
        transmission_subsurface_color: glm::vec4(0.0),
        metalness: 1.0,
        roughness: 0.8,
        opacity_subsurface_strength: 1.0,
        ior: 1.333,
        type: METAL,
    });

    int yellowMetal = addMaterial({
        albedo: glm::vec4(0.945, 0.949, 0.212, 1.0),
        emission_color: glm::vec4(0.0),
        transmission_subsurface_color: glm::vec4(0.0),
        metalness: 1.0,
        roughness: 0.1,
        opacity_subsurface_strength: 1.0,
        ior: 1.333,
        type: METAL,
    });

    int dielectric075 = addMaterial({
        albedo: glm::vec4(0.0),
        emission_color: glm::vec4(0.0),
        transmission_subsurface_color: glm::vec4(1.0),
        metalness: 1.0,
        roughness: 0.1,
        opacity_subsurface_strength: 0.0,
        ior: 0.75,
        type: TRANSMISSIVE,
    });

    int dielectric133 = addMaterial({
        albedo: glm::vec4(0.0),
        emission_color: glm::vec4(0.0),
        transmission_subsurface_color: glm::vec4(1.0),
        metalness: 1.0,
        roughness: 0.1,
        opacity_subsurface_strength: 0.0,
        ior: 1.33,
        type: TRANSMISSIVE,
    });

    // Ground sphere
    addSphere({
        pos: glm::vec3(0.0,-1000.0,-10.0),
        r: 999.0,
        mat: ground
    });

    /*

    addSphere({
        pos: glm::vec3(0.0,0.0,-10.0),
        r: 1.0,
        mat: diffuse
    });

    addSphere({
        pos: glm::vec3(2.1,0.0,-10.0),
        r: 1.0,
        mat: mirror
    });

    addSphere({
        pos: glm::vec3(4.2,0.0,-10.0),
        r: 1.0,
        mat: subsurface
    });

    addSphere({
        pos: glm::vec3(6.3,0.0,-10.0),
        r: 1.0,
        mat: hybrid
    });

    addSphere({
        pos: glm::vec3(-2.1,0.0,-10.0),
        r: 1.0,
        mat: glass
    });

    addSphere({
        pos: glm::vec3(-4.2,0.0,-10.0),
        r: 1.0,
        mat: warm_ligth
    });
    
    */

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

}

void Scene::addSphere(Sphere s){
    sphereVec.push_back(s);
}

int Scene::addMaterial(Material m){
    materialVec.push_back(m);
    return materialVec.size()-1;
}
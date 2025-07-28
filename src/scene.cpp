#include "scene.hpp"
#include <random>
#include <iostream>
#include <algorithm>

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
    // Add skybox to light list

    // Sun
    /*
    addLight({
        pos_angle_aux: glm::vec4(0.33, -0.67, 0.67,0.0),
        color_str: glm::vec4(1.0,1.0,1.0,5.0),
        type: DIRECTIONAL
    });
    */

    // Daylight
    /*
    addLight({
        pos_angle_aux: glm::vec4(0.0,0.0,0.0,0.0),
        color_str: glm::vec4(0.231, 0.756, 0.945,1.0),
        type: AMBIENT
    });
    */

    // Moon
    
    addLight({
        pos_angle_aux: glm::vec4(-0.33, -0.67, 0.67,0.0),
        color_str: glm::vec4(0.1,0.1,0.1,5.0),
        type: DIRECTIONAL
    });
    
    


    int ground = addMaterial({
        albedo: glm::vec4(0.129, 0.388, 0.082, 1.0),
        subsurface: glm::vec4(0.0),
        specular_tint: glm::vec4(0.0),
        emission_color: glm::vec4(0.0),
        roughness: 1.0,
        metallic: 0.0,
        ior: 1.5,
        trs_weight: 0.0,
    });

    int redMatte = addMaterial({
        albedo: glm::vec4(1.0, 0.0, 0.0, 1.0),
        subsurface: glm::vec4(0.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0.0),
        roughness: 0.5,
        metallic: 0.0,
        ior: 1.5,
        trs_weight: 0.0,
    });

    int gold = addMaterial({
        albedo: glm::vec4(1.000,0.720,0.315, 1.0),
        subsurface: glm::vec4(0.0),
        specular_tint: glm::vec4(1.000,0.973,0.597,0.5),
        emission_color: glm::vec4(0.0),
        roughness: 0.0,
        metallic: 0.0,
        ior: 0.47,
        trs_weight: 0.0,
    });

    int blueLight = addMaterial({
        albedo: glm::vec4(1.0, 0.0, 0.0, 1.0),
        subsurface: glm::vec4(0.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0, 1, 0.984, 4.0),
        roughness: 0.0,
        metallic: 0.0,
        ior: 1.5,
        trs_weight: 0.0,
    });

    int cloudyGlass = addMaterial({
        albedo: glm::vec4(1.0),
        subsurface: glm::vec4(1.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0.0),
        roughness: 0.0,
        metallic: 0.0,
        ior: 1.52,
        trs_weight: 1.0,
    });

    int blueMatte = addMaterial({
        albedo: glm::vec4(0.208, 0.612, 0.8, 1.0),
        subsurface: glm::vec4(0.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0.0),
        roughness: 0.2,
        metallic: 0.0,
        ior: 1.5,
        trs_weight: 0.0,
    });

    int redMetal = addMaterial({
        albedo: glm::vec4(0.82, 0.118, 0.118, 1.0),
        subsurface: glm::vec4(0.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0.0),
        roughness: 0.8,
        metallic: 1.0,
        ior: 1.5,
        trs_weight: 0.0,
    });

    int yellowMetal = addMaterial({
        albedo: glm::vec4(0.945, 0.949, 0.212, 1.0),
        subsurface: glm::vec4(0.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0.0),
        roughness: 0.1,
        metallic: 1.0,
        ior: 1.5,
        trs_weight: 0.0,
    });

    int dielectric075 = addMaterial({
        albedo: glm::vec4(1.0),
        subsurface: glm::vec4(1.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0.0),
        roughness: 0.0,
        metallic: 0.0,
        ior: 0.75,
        trs_weight: 1.0,
    });

    int dielectric133 = addMaterial({
        albedo: glm::vec4(1.0),
        subsurface: glm::vec4(1.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0.0),
        roughness: 0.0,
        metallic: 0.0,
        ior: 1.33,
        trs_weight: 1.0,
    });

    int glass = addMaterial({
        albedo: glm::vec4(1.0),
        subsurface: glm::vec4(1.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0.0),
        roughness: 0.0,
        metallic: 0.0,
        ior: 1.5,
        trs_weight: 1.0,
    });

    int mirror = addMaterial({
        albedo: glm::vec4(1.0),
        subsurface: glm::vec4(1.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0.0),
        roughness: 0.0,
        metallic: 1.0,
        ior: 1.33,
        trs_weight: 0.0,
    });

    int white_ligth = addMaterial({
        albedo: glm::vec4(1.0),
        subsurface: glm::vec4(1.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(1.0,1.0,1.0,4.0),
        roughness: 0.0,
        metallic: 1.0,
        ior: 1.33,
        trs_weight: 0.0,
    });

    int blue_ligth = addMaterial({
        albedo: glm::vec4(1.0),
        subsurface: glm::vec4(1.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0.101, 0.643, 0.835,4.0),
        roughness: 0.0,
        metallic: 1.0,
        ior: 1.33,
        trs_weight: 0.0,
    });

    int warm_ligth = addMaterial({
        albedo: glm::vec4(1.0),
        subsurface: glm::vec4(1.0),
        specular_tint: glm::vec4(1.0,1.0,1.0,0.5),
        emission_color: glm::vec4(0.984, 0.882, 0.337,4.0),
        roughness: 0.0,
        metallic: 1.0,
        ior: 1.33,
        trs_weight: 0.0,
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
        mat: redMatte
    });

    addSphere({
        pos: glm::vec3(-3.0,0.0,-10.0),
        r: 1.0,
        mat: gold
    });

    addSphere({
        pos: glm::vec3(3.0,0.0,-10.0),
        r: 1.0,
        mat: cloudyGlass
    });

    */


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

    

    std::cout<<"Scene loaded"<<std::endl;
    std::cout<<"Number of spheres: "<<sphereVec.size()<<std::endl;
    std::cout<<"Number of materials: "<<materialVec.size()<<std::endl;
    std::cout<<"Number of lights: "<<lightsVec.size()<<std::endl;
    std::cout<<"Light 1: "<<lightsVec[0].color_str.r<<","<<lightsVec[0].color_str.g<<","<<lightsVec[0].color_str.b<<","<<lightsVec[0].color_str.a<<std::endl;
}

void Scene::addSphere(Sphere s){
    // If it emmits light add it to the list
    if(materialVec[s.mat].emission_color.a > 0.0){
        addLight({
            pos_angle_aux: glm::vec4(s.pos,s.r),
            color_str: materialVec[s.mat].emission_color,
            type: SPHERE 
        });
    }

    sphereVec.push_back(s);
}

glm::vec4 clampXYZ(glm::vec4 v, float min, float max){
    glm::vec3 clamped = glm::clamp(glm::vec3(v),glm::vec3(min),glm::vec3(max));
    return glm::vec4(clamped, v.a);
}

int Scene::addMaterial(Material m){
    m.albedo = glm::clamp(m.albedo,glm::vec4(0.0),glm::vec4(1.0));
    m.subsurface = glm::clamp(m.subsurface,glm::vec4(0.0),glm::vec4(1.0));
    m.specular_tint = glm::clamp(m.specular_tint,glm::vec4(0.0),glm::vec4(1.0));
    m.emission_color = clampXYZ(m.emission_color,0.0,1.0);
    m.roughness = glm::clamp(m.roughness,float(0.005),float(1.0));
    m.metallic = glm::clamp(m.metallic,float(0.0),float(1.0));
    m.ior = glm::max(m.ior,float(0.0));
    if(m.ior == 1.0) m.ior = 1.00001;
    m.trs_weight = glm::clamp(m.trs_weight,float(0.0),float(1.0));

    materialVec.push_back(m);
    return materialVec.size()-1;
}

void Scene::addLight(Light l){
    if(lightsVec.size() == 0){
        l.accumulated_str = l.color_str.a;
    }else{
        l.accumulated_str = lightsVec[lightsVec.size()-1].accumulated_str + l.color_str.a;
    }

    lightsVec.push_back(l);

    lights_strength_sum += l.color_str.a;
}
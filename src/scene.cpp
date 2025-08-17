#include "scene.hpp"
#include <random>
#include <algorithm>
#include "tinygltf/loader.hpp"
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>


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
    // Buffers can't be 0 bytes so the vectors need at least one member
    lightsVec.push_back({});
    sphereVec.push_back({});
    triangleVec.push_back({});
    meshVec.push_back({});
    vertexVec.push_back({});
    indexVec.push_back(0);

    createPreset1();
}

void Scene::createPreset1(){
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
    /*
    addLight({
        pos_angle_aux: glm::vec4(-0.33, -0.67, 0.67,0.0),
        color_str: glm::vec4(0.1,0.1,0.1,5.0),
        type: DIRECTIONAL
    });
    */
    


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
        roughness: 0.3,
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

    

    addTriangle({
        v0: glm::vec3(0.0,-1.0,-5.0),
        v1: glm::vec3(2.5,2.0,-5.0),
        v2: glm::vec3(-2.5,2.0,-5.0),
        mat: blueMatte
    });

    addTriangle({
        v0: glm::vec3(0.0,-1.0,-15.0),
        v1: glm::vec3(2.5,2.0,-15.0),
        v2: glm::vec3(-2.5,2.0,-15.0),
        mat: blueMatte
    });

    Model teapot = {
        file_name: "teapot.glb",
        pos: glm::vec3(0.0,-1.0,10.0),
        pitch: 90.0,
        yaw: 0.0,
        roll: 0.0,
        scale: 1.0,
        material: blueMatte
    };

    addModel(teapot);



    printSceneInfo();
}

void Scene::printLight(const Light& light) {
    std::cout << "Light {\n";
    std::cout << "  pos_angle_aux: ("
              << light.pos_angle_aux.x << ", "
              << light.pos_angle_aux.y << ", "
              << light.pos_angle_aux.z << ", "
              << light.pos_angle_aux.w << ")\n";
    
    std::cout << "  color_str: ("
              << light.color_str.r << ", "
              << light.color_str.g << ", "
              << light.color_str.b << ", "
              << light.color_str.a << ")\n";

    std::cout << "  type: " << light.type << "\n";
    std::cout << "  accumulated_str: " << light.accumulated_str << "\n";
    std::cout << "}\n";
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

    if(total_spheres == 0) sphereVec.pop_back();
    sphereVec.push_back(s);
    total_spheres = sphereVec.size();
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

    if(total_lights == 0) lightsVec.pop_back();
    lightsVec.push_back(l);
    total_lights= lightsVec.size();

    lights_strength_sum += l.color_str.a;
}

void Scene::addTriangle(Triangle t){
    glm::vec3 edge1 = t.v1 - t.v0;
    glm::vec3 edge2 = t.v2 - t.v0;
    t.normal = glm::normalize(glm::cross(edge1,edge2));

    if(total_triangles == 0) triangleVec.pop_back();
    triangleVec.push_back(t);
    total_triangles = triangleVec.size();

    // If it emmits light add it to the list
    if(materialVec[t.mat].emission_color.a > 0.0){
        addLight({
            pos_angle_aux: glm::vec4(total_triangles-1,0.0,0.0,0.0),
            color_str: materialVec[t.mat].emission_color,
            type: TRIANGLE 
        });
    }
}

void Scene::addModel(Model model){

    if(total_meshes == 0){
        meshVec.pop_back();
        vertexVec.pop_back();
        indexVec.pop_back();
    }

    std::vector<Vertex> vertexVecModel;
    std::vector<uint32_t> indexVecModel;

    if(!LoadModel(ASSETS_DIRECTORY+model.file_name, vertexVecModel, indexVecModel)){
        std::cerr<<"Error loading model "<<ASSETS_DIRECTORY+model.file_name<<std::endl;
    }else{
        std::cout << "Model "<<ASSETS_DIRECTORY+model.file_name<<" loaded successfully!" << std::endl;
        std::cout << "Vertex count: " << vertexVecModel.size() << std::endl;
        std::cout << "Index count: " << indexVecModel.size() << std::endl;
    }

    float pitchRad = glm::radians(model.pitch);
    float yawRad   = glm::radians(model.yaw);
    float rollRad  = glm::radians(model.roll);
    glm::mat4 rotationMatrix = glm::yawPitchRoll(yawRad, pitchRad, rollRad);

    glm::mat4 transformationMatrix = glm::translate(glm::mat4(1.0f), model.pos) *
                                        rotationMatrix *
                                        glm::scale(glm::mat4(1.0f), glm::vec3(model.scale));

    for(int i = 0; i<vertexVecModel.size(); i++){
        glm::vec4 transformed = transformationMatrix * glm::vec4(vertexVecModel[i].pos, 1.0f);
        vertexVecModel[i].pos = glm::vec3(transformed);
    }

    
    MeshInfo mi = {
        index_start: static_cast<uint>(indexVec.size()),
        index_end: static_cast<uint>(indexVec.size() + indexVecModel.size()),
        material: model.material
    };

    meshVec.push_back(mi);
    total_meshes = meshVec.size();

    vertexVec.insert(vertexVec.end(), vertexVecModel.begin(), vertexVecModel.end());
    indexVec.insert(indexVec.end(), indexVecModel.begin(), indexVecModel.end());
}

void Scene::printSceneInfo(){
    std::cout<<"Scene loaded"<<std::endl;
    std::cout<<"Number of spheres: "<<sphereVec.size()<<std::endl;
    std::cout<<"Number of materials: "<<materialVec.size()<<std::endl;
    std::cout<<"Number of lights: "<<lightsVec.size()<<std::endl;
    for(auto i: lightsVec){
        printLight(i);
    }
    std::cout<<"Number of triangles: "<<triangleVec.size()<<std::endl;
    std::cout<<"Number of models: "<<meshVec.size()<<std::endl;
    std::cout<<"Number of vertices: "<<vertexVec.size()<<std::endl;
    std::cout<<"Number of indices: "<<indexVec.size()<<std::endl;
}
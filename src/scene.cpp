#include "scene.hpp"


Scene::Scene(){

    addSphere({glm::vec3(-1.0,-1000.0,-10.0),999.0});

    addSphere({glm::vec3(0.0,0.0,-10.0),1.0});

    addSphere({glm::vec3(3.0,0.0,-10.0),1.0});

    addSphere({glm::vec3(-3.0,0.0,-10.0),1.0});

}

void Scene::addSphere(Sphere s){
    sphereVec.push_back(s);
}
#include <glm/glm.hpp>
#include <vector>


struct Sphere{
    glm::vec3 pos;
    float r;
};



class Scene{
public:
    std::vector<Sphere> sphereVec;
    
    Scene();
private:
    void addSphere(Sphere s);
};



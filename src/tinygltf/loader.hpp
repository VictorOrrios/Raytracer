#include <vector>
#include <iostream>
#include "definitions.hpp"


bool LoadModel(const std::string& filename, 
               std::vector<Vertex>& vertices, 
               std::vector<uint32_t>& indices);
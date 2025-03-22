#ifndef CUBE_H
#define CUBE_H

#include <vector>
#include <glm/glm.hpp>  // GLM for vector math

class Cube {
    public:
        Cube(int subdivisions, float size);
        std::vector<float> vertices;
        std::vector<float> normals;
    private:
        void generateFace(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 normal, int subdivisions);
        std::vector<float> generateCube(int subdivisions, float size);
};

#endif

#ifndef CUBE_H
#define CUBE_H

#include <vector>
#include <glm/glm.hpp>  // GLM for vector math

class Cube {
    public:
        Cube(int subdivisions, float size);
        std::vector<glm::vec3> getPositions() { return positions; };
        std::vector<uint32_t> getIndices() { return indices; };
        std::vector<glm::vec3> getNormals() { return normals; };
        std::vector<float> getInterleavedData();
        size_t getNumVertices() const;
        size_t getNumIndices() const;
    private:
        void generateFace( const glm::vec3 &faceCenter, const glm::vec3 &uAxis, const glm::vec3 &vAxis );
        void generateCube();
        int mSubdivisions;
        int mSize;
        std::vector<glm::vec3> positions;
        std::vector<uint32_t> indices;
        std::vector<glm::vec3> normals;
};

#endif

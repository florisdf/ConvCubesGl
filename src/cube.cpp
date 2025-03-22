#include <iostream>

#include "cube.h"

Cube::Cube(int subdivisions, float size) {
    generateCube(subdivisions, size);
}

// Function to generate vertices for a subdivided cube face (as a float array)
void Cube::generateFace(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 normal, int subdivisions) {
    float step = 1.0f / subdivisions;

    for (int i = 0; i < subdivisions; ++i) {
        for (int j = 0; j < subdivisions; ++j) {
            // Interpolate positions
            float u0 = i * step, u1 = (i + 1) * step;
            float v0_ = j * step, v1_ = (j + 1) * step;

            glm::vec3 p00 = glm::mix(glm::mix(v0, v1, u0), glm::mix(v2, v3, u0), v0_);
            glm::vec3 p10 = glm::mix(glm::mix(v0, v1, u1), glm::mix(v2, v3, u1), v0_);
            glm::vec3 p01 = glm::mix(glm::mix(v0, v1, u0), glm::mix(v2, v3, u0), v1_);
            glm::vec3 p11 = glm::mix(glm::mix(v0, v1, u1), glm::mix(v2, v3, u1), v1_);

            // Create two triangles per quad (6 vertices per face)
            std::vector<glm::vec3> quadVertices = {p00, p10, p01, p10, p11, p01};

            // Store vertices in float array format
            for (const auto& p : quadVertices) {
                vertices.push_back(p.x);
                vertices.push_back(p.y);
                vertices.push_back(p.z);
            }

            // Store the same normal for each vertex
            normals.push_back(normal.x);
            normals.push_back(normal.y);
            normals.push_back(normal.z);
        }
    }
}

// Function to generate a subdivided cube
std::vector<float> Cube::generateCube(int subdivisions, float size) {
    std::vector<float> vertices;

    auto s = size/2;
    glm::vec3 v[8] = {
        {-s, -s, -s}, {s, -s, -s}, {s, s, -s}, {-s, s, -s},
        {-s, -s, s},  {s, -s, s},  {s, s, s},  {-s, s, s}
    };

    // Generate vertices for each face
    generateFace(v[0], v[1], v[3], v[2], {0, 0, -1}, subdivisions); // Front
    generateFace(v[5], v[4], v[6], v[7], {0, 0, 1}, subdivisions);  // Back
    generateFace(v[1], v[5], v[2], v[6], {1, 0, 0}, subdivisions);  // Right
    generateFace(v[4], v[0], v[7], v[3], {-1, 0, 0}, subdivisions); // Left
    generateFace(v[3], v[2], v[7], v[6], {0, 1, 0}, subdivisions);  // Top
    generateFace(v[4], v[5], v[0], v[1], {0, -1, 0}, subdivisions); // Bottom

    return vertices;
}

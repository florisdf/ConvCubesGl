#include <iostream>

#include "cube.h"

Cube::Cube(int subdivisions, float size): mSubdivisions(subdivisions), mSize(size) {
    generateCube();
}

std::vector<float> Cube::getInterleavedData() {
    std::vector<float> data;

    for (auto idx: indices) {
        auto p = positions[idx];
        auto n = normals[idx];
        data.push_back(p.x);
        data.push_back(p.y);
        data.push_back(p.z);
        data.push_back(n.x);
        data.push_back(n.y);
        data.push_back(n.z);
    }

    return data;
}

void Cube::generateFace( const glm::vec3 &faceCenter, const glm::vec3 &uAxis, const glm::vec3 &vAxis )
{
    const glm::vec3 normal = normalize( faceCenter );

    const uint32_t baseIdx = (uint32_t)positions.size();

    // fill vertex data
    for( int vi = 0; vi <= mSubdivisions; vi++ ) {
        const float v = vi / float(mSubdivisions);
        for( int ui = 0; ui <= mSubdivisions; ui++ ) {
            const float u = ui / float(mSubdivisions);

            positions.emplace_back( faceCenter + ( u - 0.5f ) * 2.0f * uAxis + ( v - 0.5f ) * 2.0f * vAxis );
            normals.emplace_back( normal );
        }
    }

    // 'baseIdx' will correspond to the index of the first vertex we created in this call to generateFace()
//    const uint32_t baseIdx = indices->empty() ? 0 : ( indices->back() + 1 );
    for( int u = 0; u < mSubdivisions; u++ ) {
        for( int v = 0; v < mSubdivisions; v++ ) {
            const int i = u + v * ( mSubdivisions + 1 );

            indices.push_back( baseIdx + i );
            indices.push_back( baseIdx + i + mSubdivisions + 1 );
            indices.push_back( baseIdx + i + 1 );

            indices.push_back( baseIdx + i + 1 );
            indices.push_back( baseIdx + i + mSubdivisions + 1 );
            indices.push_back( baseIdx + i + mSubdivisions + 2 );
            // important the last is the highest idx due to determination of next face's baseIdx
        }
    }
}

size_t Cube::getNumVertices() const
{
    return 2 * ( (mSubdivisions+1) * (mSubdivisions+1) ) // +-Z
            + 2 * ( (mSubdivisions+1) * (mSubdivisions+1) ) // +-X
            + 2 * ( (mSubdivisions+1) * (mSubdivisions+1) ); // +-Y
}

size_t Cube::getNumIndices() const
{
    return 2 * 6 * ( mSubdivisions * mSubdivisions ) // +-Z
            + 2 * 6 * ( mSubdivisions * mSubdivisions ) // +-X
            + 2 * 6 * ( mSubdivisions * mSubdivisions ); // +-Y
}

void Cube::generateCube()
{
    const size_t numVertices = getNumVertices();
    
    // reserve room in vectors and set pointers to non-null for normals, texcoords and colors as appropriate
    positions.reserve( numVertices );
    indices.reserve( getNumIndices() );
    normals.reserve( numVertices );

    glm::vec3 sz = 0.5f * glm::vec3(mSize, mSize, mSize);
    
    // +X
    generateFace( glm::vec3(sz.x,0,0), glm::vec3(0,0,sz.z), glm::vec3(0,sz.y,0) );
    // +Y
    generateFace( glm::vec3(0,sz.y,0), glm::vec3(sz.x,0,0), glm::vec3(0,0,sz.z) );
    // +Z
    generateFace( glm::vec3(0,0,sz.z), glm::vec3(0,sz.y,0), glm::vec3(sz.x,0,0) );
    // -X
    generateFace( glm::vec3(-sz.x,0,0), glm::vec3(0,sz.y,0), glm::vec3(0,0,sz.z) );
    // -Y
    generateFace( glm::vec3(0,-sz.y,0), glm::vec3(0,0,sz.z), glm::vec3(sz.x,0,0) );
    // -Z
    generateFace( glm::vec3(0,0,-sz.z), glm::vec3(sz.x,0,0), glm::vec3(0,sz.y,0) );
}

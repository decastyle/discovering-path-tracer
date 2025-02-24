#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include "tiny_obj_loader.h"

struct BVHNode 
{
    glm::vec4 minBounds; // xyz = min, w = leftChild (child index or -1 to flag as leaf)
    glm::vec4 maxBounds; // xyz = max, w = rightChild (child index or triangle index if leaf)
    // triangle index is pointing to the first index of a triangle, or startTriangleIndex * 3
};

class BVH {
public:
    BVH(const std::vector<tinyobj::real_t>& objVertices, const std::vector<uint32_t>& objIndices);

    const std::vector<tinyobj::real_t>& getVertices() const { return vertices; }
    const std::vector<uint32_t>& getIndices() const { return indices; }
    const std::vector<BVHNode>& getNodes() const { return nodes; }

private:
    const std::vector<tinyobj::real_t>& vertices; // Reference vertex data (x, y, z per vertex)
    std::vector<uint32_t> indices;                // Flat index buffer (v0, v1, v2 per triangle)
    std::vector<BVHNode> nodes;                   // BVH hierarchy

    glm::vec3 getVertex(uint32_t index) const;

    void constructBVH(uint32_t startTriangleIndex, uint32_t endTriangleIndex, uint32_t& nodeIndex);
    void computeBounds(uint32_t startTriangleIndex, uint32_t endTriangleIndex, glm::vec3& minOut, glm::vec3& maxOut);
    glm::vec3 computeCentroid(uint32_t triangleIndex) const; // Helper for sorting
};
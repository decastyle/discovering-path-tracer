#include "BoundingVolumeHierarchy.h"
#include <QDebug>

BVH::BVH(const std::vector<tinyobj::real_t>& objVertices, const std::vector<uint32_t>& objIndices) 
    : vertices(objVertices), indices(objIndices)
{
    if (indices.size() % 3 != 0) 
    {
        qDebug() << "Error: Index array size must be a multiple of 3";
        return;
    }

    qDebug() << "Triangle count:" << indices.size() / 3;

    // for n leaves there will be 2n - 1 nodes
    uint32_t triangleCount = indices.size() / 3;
    uint32_t nodeCount = 2 * triangleCount - 1;
    nodes.resize(nodeCount);

    uint32_t nodeIndex = 0;
    constructBVH(0, indices.size() / 3, nodeIndex); // number of triangles = indices / 3
}

void BVH::constructBVH(uint32_t startTriangleIndex, uint32_t endTriangleIndex, uint32_t& nodeIndex) 
{
    BVHNode node;
    uint32_t currentIndex = nodeIndex++;

    glm::vec3 minBounds, maxBounds;
    computeBounds(startTriangleIndex, endTriangleIndex, minBounds, maxBounds);
    node.minBounds = glm::vec4(minBounds, 0.0f); // default w, to be overwritten
    node.maxBounds = glm::vec4(maxBounds, 0.0f); // default w, to be overwritten

    uint32_t numTriangles = endTriangleIndex - startTriangleIndex;
    if (numTriangles == 1) 
    {
        node.minBounds.w = -1.0f;                         // leaf flag
        node.maxBounds.w = float(startTriangleIndex * 3); // Offset into indices
    } 
    else 
    {
        // Precompute centroids for this range
        std::vector<std::pair<uint32_t, glm::vec3>> triangleCentroids;
        triangleCentroids.reserve(numTriangles);
        for (uint32_t triangleIndex = startTriangleIndex; triangleIndex < endTriangleIndex; triangleIndex++) 
        {
            triangleCentroids.emplace_back(triangleIndex, computeCentroid(triangleIndex));
        }

        // Sort triangle indices by centroid along the longest axis
        glm::vec3 minBounds = glm::vec3(node.minBounds.x, node.minBounds.y, node.minBounds.z);
        glm::vec3 maxBounds = glm::vec3(node.maxBounds.x, node.maxBounds.y, node.maxBounds.z);
        glm::vec3 size = maxBounds - minBounds;

        int axis = (size.x > size.y) ? ((size.x > size.z) ? 0 : 2) : ((size.y > size.z) ? 1 : 2);

        std::sort(triangleCentroids.begin(), triangleCentroids.end(),
            [axis](const auto& a, const auto& b) {
                return a.second[axis] < b.second[axis];
            });

        // Reorder the indices array based on sorted triangle order
        std::vector<uint32_t> tempIndices(numTriangles * 3);
        for (uint32_t i = 0; i < numTriangles; i++) 
        {
            uint32_t originalIndex = triangleCentroids[i].first * 3;
            std::copy_n(&indices[originalIndex], 3, &tempIndices[i * 3]);
        }
        std::copy_n(tempIndices.data(), numTriangles * 3, &indices[startTriangleIndex * 3]);

        uint32_t mid = (startTriangleIndex + endTriangleIndex) / 2;

        node.minBounds.w = float(nodeIndex); // Left child
        constructBVH(startTriangleIndex, mid, nodeIndex);

        node.maxBounds.w = float(nodeIndex); // Right child
        constructBVH(mid, endTriangleIndex, nodeIndex);
    }

    nodes[currentIndex] = node;
}

void BVH::computeBounds(uint32_t startTriangleIndex, uint32_t endTriangleIndex, glm::vec3& minOut, glm::vec3& maxOut) 
{
    minOut = glm::vec3(FLT_MAX);
    maxOut = glm::vec3(-FLT_MAX);

    for (uint32_t triangleIndex = startTriangleIndex; triangleIndex < endTriangleIndex; triangleIndex++) 
    {
        uint32_t vertexIndex = triangleIndex * 3;

        glm::vec3 v0 = glm::vec3(vertices[indices[vertexIndex] * 3], vertices[indices[vertexIndex] * 3 + 1], vertices[indices[vertexIndex] * 3 + 2]);
        glm::vec3 v1 = glm::vec3(vertices[indices[vertexIndex + 1] * 3], vertices[indices[vertexIndex + 1] * 3 + 1], vertices[indices[vertexIndex + 1] * 3 + 2]);
        glm::vec3 v2 = glm::vec3(vertices[indices[vertexIndex + 2] * 3], vertices[indices[vertexIndex + 2] * 3 + 1], vertices[indices[vertexIndex + 2] * 3 + 2]);

        minOut = glm::min(minOut, glm::min(v0, glm::min(v1, v2)));
        maxOut = glm::max(maxOut, glm::max(v0, glm::max(v1, v2)));
    }
}

glm::vec3 BVH::computeCentroid(uint32_t triangleIndex) const 
{
    uint32_t vertexIndex = triangleIndex * 3;

    glm::vec3 v0 = glm::vec3(vertices[indices[vertexIndex] * 3], vertices[indices[vertexIndex] * 3 + 1], vertices[indices[vertexIndex] * 3 + 2]);
    glm::vec3 v1 = glm::vec3(vertices[indices[vertexIndex + 1] * 3], vertices[indices[vertexIndex + 1] * 3 + 1], vertices[indices[vertexIndex + 1] * 3 + 2]);
    glm::vec3 v2 = glm::vec3(vertices[indices[vertexIndex + 2] * 3], vertices[indices[vertexIndex + 2] * 3 + 1], vertices[indices[vertexIndex + 2] * 3 + 2]);

    return (v0 + v1 + v2) / 3.0f;
}
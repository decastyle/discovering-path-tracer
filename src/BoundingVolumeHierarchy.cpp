#include "BoundingVolumeHierarchy.h"
#include <QDebug>
#include <algorithm>

BVH::BVH(const std::vector<tinyobj::real_t>& objVertices, const std::vector<uint32_t>& objIndices) 
    : vertices(objVertices), indices(objIndices)
{
    if (indices.size() % 3 != 0) 
    {
        qDebug() << "Error: Index array size must be a multiple of 3";
        return;
    }
    // for n leaves (triangles) there will be 2n - 1 nodes
    uint32_t triangleCount = indices.size() / 3;

    qDebug() << "Triangle count:" << triangleCount;

    uint32_t nodeCount = 2 * triangleCount - 1;
    nodes.resize(nodeCount);

    uint32_t nodeIndex = 0;
    constructBVH(0, triangleCount, nodeIndex); 
}

void BVH::constructBVH(uint32_t startTriangleIndex, uint32_t endTriangleIndex, uint32_t& nodeIndex) 
{
    BVHNode node;
    uint32_t currentIndex = nodeIndex++;

    glm::vec3 minBounds, maxBounds;
    computeBounds(startTriangleIndex, endTriangleIndex, minBounds, maxBounds);
    node.minBounds = glm::vec4(minBounds, 0.0f); // default w, to be overwritten
    node.maxBounds = glm::vec4(maxBounds, 0.0f); // default w, to be overwritten

    uint32_t currentTriangleCount = endTriangleIndex - startTriangleIndex;
    if (currentTriangleCount == 1) 
    {
        node.minBounds.w = -1.0f;                  // leaf flag
        node.maxBounds.w = startTriangleIndex * 3; // Offset into indices
    } 
    else 
    {
        // Precompute centroids for this range
        std::vector<std::pair<uint32_t, glm::vec3>> triangleCentroids;
        triangleCentroids.reserve(currentTriangleCount);
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
        std::vector<uint32_t> tempIndices(currentTriangleCount * 3);
        for (uint32_t i = 0; i < currentTriangleCount; i++) 
        {
            uint32_t originalIndex = triangleCentroids[i].first * 3;
            std::copy_n(&indices[originalIndex], 3, &tempIndices[i * 3]);
        }
        std::copy_n(tempIndices.data(), currentTriangleCount * 3, &indices[startTriangleIndex * 3]);

        uint32_t mid = (startTriangleIndex + endTriangleIndex) / 2;

        node.minBounds.w = nodeIndex; // Left child
        constructBVH(startTriangleIndex, mid, nodeIndex);

        node.maxBounds.w = nodeIndex; // Right child
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

        glm::vec3 v0 = getVertex(vertexIndex);
        glm::vec3 v1 = getVertex(vertexIndex + 1);
        glm::vec3 v2 = getVertex(vertexIndex + 2);

        minOut = glm::min(minOut, glm::min(v0, glm::min(v1, v2)));
        maxOut = glm::max(maxOut, glm::max(v0, glm::max(v1, v2)));
    }
}

glm::vec3 BVH::computeCentroid(uint32_t triangleIndex) const 
{
    uint32_t vertexIndex = triangleIndex * 3;

    glm::vec3 v0 = getVertex(vertexIndex);
    glm::vec3 v1 = getVertex(vertexIndex + 1);
    glm::vec3 v2 = getVertex(vertexIndex + 2);

    return (v0 + v1 + v2) / 3.0f;
}

glm::vec3 BVH::getVertex(uint32_t index) const 
{
    uint32_t base = indices[index] * 3;
    return glm::vec3(vertices[base], vertices[base + 1], vertices[base + 2]);
}

void BVH::printBVH(const BVH& bvh) {
    // Print number of nodes
    qDebug() << "BVH built with" << bvh.getNodes().size() << "nodes";

    // Print vertices
    const auto& vertices = bvh.getVertices();
    qDebug() << "Vertices (" << vertices.size() / 3 << "total):";
    for (size_t i = 0; i < vertices.size(); i += 3) {
        if (i + 2 < vertices.size()) { // Safety check
            qDebug() << "  Vertex" << (i / 3) << ": (" 
                     << vertices[i] << "," << vertices[i + 1] << "," << vertices[i + 2] << ")";
        } else {
            qDebug() << "  Vertex" << (i / 3) << ": (incomplete)";
        }
    }

    // Print indices
    const auto& indices = bvh.getIndices();
    qDebug() << "Indices (" << indices.size() / 3 << "triangles):";
    for (size_t i = 0; i < indices.size(); i += 3) {
        if (i + 2 < indices.size()) { // Safety check
            qDebug() << "  Triangle" << (i / 3) << ": (" 
                     << indices[i] << "," << indices[i + 1] << "," << indices[i + 2] << ")";
        } else {
            qDebug() << "  Triangle" << (i / 3) << ": (incomplete)";
        }
    }

    const auto& nodes = bvh.getNodes();
    qDebug() << "Nodes (" << nodes.size() << "total):";
    for (size_t i = 0; i < nodes.size(); ++i) {
        const BVHNode& node = nodes[i];
        qDebug() << "  Node" << i << ":";
        qDebug() << "    minBounds: (" << node.minBounds.x << "," << node.minBounds.y << ","
                 << node.minBounds.z << "," << node.minBounds.w << ")";
        qDebug() << "    maxBounds: (" << node.maxBounds.x << "," << node.maxBounds.y << ","
                 << node.maxBounds.z << "," << node.maxBounds.w << ")";
    }
}
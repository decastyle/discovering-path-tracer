#pragma once

#include <glm/glm.hpp>
#include <vector>

struct AreaLightData 
{
    glm::vec4 position;  // xyz = position, w = padding
    glm::vec4 normal;    // xyz = normal, w = padding
    glm::vec4 intensity; // xyz = intensity, w = padding
    glm::vec4 size;      // xy = width and height
};

class Light 
{
public:
    Light(const std::vector<glm::vec3>& positions,
          const std::vector<glm::vec3>& normals,
          const std::vector<glm::vec3>& intensities,
          const std::vector<glm::vec2>& sizes);

    Light();

    const std::vector<AreaLightData>& getLights() const { return lights; }

private:
    std::vector<AreaLightData> lights;

    void packData(const std::vector<glm::vec3>& positions,
                  const std::vector<glm::vec3>& normals,
                  const std::vector<glm::vec3>& intensities,
                  const std::vector<glm::vec2>& sizes);
};
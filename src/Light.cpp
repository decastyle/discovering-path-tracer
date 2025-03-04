#include "Light.h"

Light::Light(const std::vector<glm::vec3>& positions,
             const std::vector<glm::vec3>& normals,
             const std::vector<glm::vec3>& intensities,
             const std::vector<glm::vec2>& sizes)
{
    packData(positions, normals, intensities, sizes);
}

Light::Light()
{
    lights.clear();
}

void Light::packData(const std::vector<glm::vec3>& positions,
                     const std::vector<glm::vec3>& normals,
                     const std::vector<glm::vec3>& intensities,
                     const std::vector<glm::vec2>& sizes) 
{
    lights.clear();
    lights.reserve(positions.size());

    for (size_t i = 0; i < positions.size(); ++i) 
    {
        AreaLightData lightData;
        lightData.position  = glm::vec4(positions[i], 0.0f);
        lightData.normal    = glm::vec4(glm::normalize(normals[i]), 0.0f);
        lightData.intensity = glm::vec4(intensities[i], 0.0f);
        lightData.size      = glm::vec4(sizes[i], 0.0f, 0.0f);

        lights.push_back(lightData);
    }
}
#include "Light.h"
#include <stdexcept>

Light::Light(const std::vector<glm::vec3>& positions,
             const std::vector<glm::vec3>& normals,
             const std::vector<glm::vec2>& sizes,
             const std::vector<glm::vec3>& intensities)
{
    packData(positions, normals, sizes, intensities);
}

Light::Light()
{
    lights.clear();
}

void Light::packData(const std::vector<glm::vec3>& positions,
                     const std::vector<glm::vec3>& normals,
                     const std::vector<glm::vec2>& sizes,
                     const std::vector<glm::vec3>& intensities) 
{
    lights.clear();
    lights.reserve(positions.size());

    for (size_t i = 0; i < positions.size(); ++i) 
    {
        AreaLightData lightData;
        lightData.position = glm::vec4(positions[i], 0.0f);         // Pad position
        lightData.normal = glm::vec4(glm::normalize(normals[i]), 0.0f); // Normalize and pad normal
        lightData.size = sizes[i];
        lightData.intensity = glm::vec4(intensities[i], 0.0f);     // Pad intensity

        lights.push_back(lightData);
    }
}
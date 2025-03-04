#version 460

layout(location = 0) in vec3 v_vertPos; 
layout(location = 1) in vec3 v_normal; // Interpolated normal
layout(location = 2) in vec2 v_uv;
layout(location = 3) in vec3 v_cameraPos; 

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 textureColor = texture(texSampler, v_uv);
    vec4 specularColor = vec4(1.0, 1.0, 1.0, 1.0);
    vec4 ambientColor = vec4(1.0, 1.0, 1.0, 1.0) * textureColor;

    float Ka = 1.0;
    float Kd = 1.0;
    float Ks = 0.8;
    float shininessVal = 3.0;

    vec3 lightPos = v_cameraPos;
    vec3 N = normalize(v_normal);
    vec3 L = normalize(lightPos - v_vertPos);

    // Lambert's cosine law
    float lambertian = max(dot(N, L), 0.0);
    float specular = 0.0;
    
    if(lambertian > 0.0) {
        vec3 R = reflect(-L, N);      // Reflected light vector
        vec3 V = normalize(v_cameraPos - v_vertPos); // Vector to viewer
        // Compute the specular term
        float specAngle = max(dot(R, V), 0.0);
        specular = pow(specAngle, shininessVal);
    }
    // fragColor = Ka * ambientColor + 
    //             Kd * lambertian * textureColor + 
    //             Ks * specular * specularColor;
    fragColor = ambientColor;
}

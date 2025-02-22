#version 460

layout(location = 0) in vec3 position;  
layout(location = 1) in vec3 normal;    
layout(location = 2) in vec2 uv;     

layout(location = 0) out vec3 v_vertPos; 
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec3 v_cameraPos; 

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec3 cameraPos;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    v_vertPos = position;

    v_normal = normal;

    v_uv = uv;

    v_cameraPos = ubuf.cameraPos;
    
    gl_Position = ubuf.mvp * vec4(position, 1.0);
}

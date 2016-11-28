#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Light
{
    vec4 pos;
    vec4 ambiant;
    vec4 diffuse;
    vec4 specular;
    vec4 attenuation;
    vec4 dir;
    vec4 angle;
};

struct Material
{
    vec4 ambiant;
    vec4 diffuse;
    vec4 specular;
    vec4 hardness;
};

layout(binding = 0, set = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    Material material;
    Light point;
    Light sun;
    Light spot;
    vec4 eye;
} ubo;

layout(location = 0) in vec3 vp;
layout(location = 1) in vec3 vn;

out vec3 position;
out vec3 normal;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vp, 1.0);
    position = vp;
    normal = vn;
}
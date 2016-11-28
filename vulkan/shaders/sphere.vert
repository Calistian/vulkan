#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Light
{
	vec3 pos;
	vec3 ambiant;
	vec3 diffuse;
	vec3 specular;
	vec3 attenuation;
	vec3 dir;
	float angle;
};

layout(binding = 0, set = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    Light lights[3];
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
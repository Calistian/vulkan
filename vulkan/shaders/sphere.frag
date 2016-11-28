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

in vec3 position;
in vec3 normal;

layout(location = 0) out vec4 outColor;

vec4 point_light()
{
	vec4 result = vec4(0, 0, 0, 0);

	result += ubo.material.ambiant * ubo.point.ambiant;

	vec4 dir = vec4(position, 1) - ubo.point.pos;
	float dist = length(dir);
	dir = -normalize(dir);
	float a = dot(dir, vec4(normal, 0));
	result += a * ubo.material.diffuse * ubo.point.diffuse / (ubo.point.attenuation[0] + ubo.point.attenuation[1] * dist + ubo.point.attenuation[2] * dist * dist);

	vec4 V = vec4(position, 1) - ubo.point.pos;
	dist = length(V);
	V = normalize(V);
	vec3 R = reflect(vec3(V), normal);
	vec4 E = normalize(vec4(position, 1) - ubo.eye);
	a = dot(R, vec3(E));
	result += pow(a, ubo.material.hardness.x) * ubo.material.specular * ubo.point.specular / (ubo.point.attenuation[0] + ubo.point.attenuation[1] * dist + ubo.point.attenuation[2] * dist * dist);

	return vec4(result.xyz, 1);
}

void main() {
    outColor = clamp(point_light(), 0, 1);
}
#version 430

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

uniform Material material;
uniform Light point;
uniform vec4 eye;

in vec3 position;
in vec3 normal;

layout(location = 0) out vec4 outColor;

vec4 point_light()
{
	vec4 result = vec4(0, 0, 0, 0);

	result += material.ambiant * point.ambiant;

	vec4 dir = vec4(position, 1) - point.pos;
	float dist = length(dir);
	dir = -normalize(dir);
	float a = dot(dir, vec4(normal, 0));
	result += a * material.diffuse * point.diffuse / (point.attenuation[0] + point.attenuation[1] * dist + point.attenuation[2] * dist * dist);

	vec4 V = vec4(position, 1) - point.pos;
	dist = length(V);
	V = -normalize(V);
	vec3 R = reflect(vec3(V), normal);
	vec4 E = normalize(vec4(position, 1) - eye);
	a = dot(R, vec3(E));
	result += pow(a, material.hardness.x) * material.specular * point.specular / (point.attenuation[0] + point.attenuation[1] * dist + point.attenuation[2] * dist * dist);

	return vec4(result.xyz, 1);
}

void main()
{
    outColor = clamp(point_light(), 0, 1);
}

#version 450

#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 M;
	mat4 V;
	mat4 P;
} ubo;

in vec3 vp;
in vec3 vn;
in vec2 vt;

void main()
{
	gl_Position = ubo.P * ubo.V * ubo.M * vec4(vp, 1.0);
}
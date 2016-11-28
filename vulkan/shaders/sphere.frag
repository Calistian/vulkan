#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

in vec3 color;

void main() {
    outColor = abs(vec4(color, 1));
}
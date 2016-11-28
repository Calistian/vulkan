#version 430

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

layout(location = 0) in vec3 vp;
layout(location = 1) in vec3 vn;

out vec3 position;
out vec3 normal;

void main()
{
    gl_Position = proj * view * model * vec4(vp, 1.0);
    position = vp;
    normal = vn;
}

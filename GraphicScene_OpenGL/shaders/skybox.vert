#version 410 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoords;

out vec2 texCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    texCoords = vTexCoords;
    vec4 pos = projection * view * model * vec4(vPosition, 1.0);
    gl_Position = pos;
}
#version 330 core
layout (location = 0) in vec3 pos;
layout(location = 1) in vec2 txc;

out vec3 worldPos;
out vec2 vtxc;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main() 
{
    gl_Position = projectionMatrix * viewMatrix * vec4(pos, 1.0);
    worldPos = pos;
    vtxc = txc;
}
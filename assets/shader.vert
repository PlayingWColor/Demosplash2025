#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec2 screenUV;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    screenUV = (vec2(inPosition.x,inPosition.y * -1.0) + 1.0) * 0.5;
}
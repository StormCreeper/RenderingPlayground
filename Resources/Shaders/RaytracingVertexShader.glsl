#version 450 core

layout(location=0) in vec3 vPosition;

out vec2 fPos;

void main() {
    gl_Position =  vec4 (vPosition, 1.0);
    fPos = vPosition.xy;
}
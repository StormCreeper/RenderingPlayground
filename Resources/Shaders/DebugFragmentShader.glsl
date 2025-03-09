#version 410 core

uniform vec3 color;

out vec4 colorResponse;

void main() {
    colorResponse = vec4(color, 1.0f);
}
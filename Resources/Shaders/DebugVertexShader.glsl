#version 410 core

layout(location=0) in vec3 vPosition;

uniform mat4 projectionMat, modelViewMat;

uniform vec3 begin_corner;
uniform vec3 end_corner;


out vec3 fPos;

// x * (end-begin) + begin

void main() {
    vec3 pos = vPosition * (end_corner - begin_corner) + (end_corner + begin_corner) * 0.5;
    gl_Position =  projectionMat * modelViewMat * vec4 (pos, 1.0);
}
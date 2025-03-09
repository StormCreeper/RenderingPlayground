#version 410 core

uniform sampler2D imageTex;

in vec2 fTexCoord;

out vec4 colorResponse;

void main () {
	colorResponse = vec4 (texture(imageTex, fTexCoord).rgb, 1.0);	
}
#version 410 core // Minimal GL version support expected from the GPU

layout(location=0) in vec3 vPosition; // The 1st input attribute is the position (CPU side: glVertexAttrib 0)
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vUV;
layout(location=3) in vec3 vTangent;
layout(location=4) in vec3 vBitangent;

uniform mat4 projectionMat, modelViewMat, normalMat, modelMat; // Uniform variables, set from the CPU-side main program

struct Material {
    vec3 albedo;
    float roughness;
    vec3 F0;
    float metalness;

    bool transparent;
    float base_reflectance;
    float ior;
    float absorption;

    int albedoTex;
    int roughnessTex;
    int aoTex;
    int metalnessTex;

    int normalTex;
    int heightTex;
    float heightMult;
    int padding2;
};

uniform Material material;

#define MAX_TEXTURES 16
uniform sampler2D textures[MAX_TEXTURES];

out vec3 fNormal;
out vec3 fPos;
out vec2 fUV;
out vec3 fTangent;
out vec3 fBitangent;

void main() {
    vec3 pos = vPosition;
    if(material.heightTex != -1) {
        pos += normalize(vNormal) * ((texture(textures[material.heightTex], vUV).r - 0.5) * material.heightMult);
    }
	vec4 p = modelViewMat * vec4 (pos, 1.0);
    vec4 world_p = modelMat * vec4 (pos, 1.0);
    gl_Position =  projectionMat * p; // mandatory to fire rasterization properly
    fPos = world_p.xyz;
    mat4 normalMat = transpose(inverse(modelMat));
    fNormal = (normalMat * vec4 (normalize (vNormal), 0.0)).xyz;
    fUV = vUV;
    fTangent = (normalMat * vec4 (normalize (vTangent), 0.0)).xyz;
    fBitangent = (normalMat * vec4 (normalize (vBitangent), 0.0)).xyz;
}
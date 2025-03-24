#version 450 core // Minimal GL version support expected from the GPU

in vec3 fNormal; // Shader input, linearly interpolated by default from the previous stage (here the vertex shader)
in vec3 fPos;
in vec2 fUV;
in vec3 fTangent;
in vec3 fBitangent;

out vec4 colorResponse; // Shader output: the color response attached to this fragment

const float PI = 3.14159265358979323846;

#define MAX_LIGHTS 10
#define MAX_TEXTURES 16

struct LightSource {
    int type; // 0: directional, 1: point
    vec3 direction; // Directional light: direction, Point light: position
    vec3 color;
    float intensity;

    // For point lights
    float ac;
    float al;
    float aq;
};

uniform LightSource lights[MAX_LIGHTS];
uniform int numOfLights;

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

    // Iridescence
    float Dinc;
    float eta2;
    float eta3;
    float kappa3;
};

uniform Material material;

struct ImageParameters {
    bool colorCorrect;
    bool useSRGB;
    bool useToneMapping;
    bool useExposure;
    float exposure;
};

uniform ImageParameters imageParameters;

uniform vec3 eye;

uniform sampler2D textures[MAX_TEXTURES];

vec4 sampleTex(in vec2 uv, in int index, in vec4 fallback) {
    if(index < 0) return fallback;
    else return texture(textures[index], uv);
}

float sqr(float x) {
    return x * x;
}

vec3 pow3(vec3 v, int p) {
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

// From https://blog.demofox.org/2020/06/06/casual-shadertoy-path-tracing-2-image-improvement-and-glossy-reflections/
vec3 LessThan(vec3 f, float value) {
    return vec3(
        f.x < value ? 1.0 : 0.0,
        f.y < value ? 1.0 : 0.0,
        f.z < value ? 1.0 : 0.0
    );
}

vec3 LinearToSRGB(vec3 rgb) {
    rgb = clamp(rgb, 0.0, 1.0);
 
    return mix(
        pow(rgb, vec3(1.0 / 2.4)) * 1.055 - 0.055,
        rgb * 12.92,
        LessThan(rgb, 0.0031308)
    );
}
 
vec3 SRGBToLinear(vec3 rgb) {
    rgb = clamp(rgb, 0.0, 1.0);
 
    return mix(
        pow(((rgb + 0.055) / 1.055), vec3(2.4)),
        rgb / 12.92,
        LessThan(rgb, 0.04045)
    );
}

// ACES tone mapping curve fit to go from HDR to LDR
//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0f, 1.0f);
}


// Trowbridge-Reitz Normal Distribution
float NormalDistributionGGX(vec3 n, vec3 wh, float alpha) {
    float nom = sqr(alpha);
    float denom = PI * sqr(1 + (sqr(alpha) - 1.0) * sqr(dot(n, wh)));

    return nom / denom;
}

// Schlick approx for Fresnel term
vec3 Fresnel(vec3 wi, vec3 wh, vec3 F0) {
    return F0 + (1-F0) * pow(1 - max(0, dot(wi, wh)), 5);
}

// Schlick approximation for geometric term
float G_Schlick(vec3 n, vec3 w, float alpha) {
    float k = alpha * sqrt(2.0 / PI);
    float dotNW = dot(n, w);
    return dotNW / (dotNW * (1-k) + k);
}

float G_GGX(vec3 n, vec3 wi, vec3 wo, float alpha) {
    return G_Schlick(n, wi, alpha) * G_Schlick(n, wo, alpha);
}

// F = Fs (specular) + Fd (diffuse)

// Fs(wi, wo) = D*F*G / (4*dot(wi, n)*dot(wo, n))

vec3 F_Specular(vec3 n, vec3 wi, vec3 wo, vec3 wh, vec3 F0, float alpha) {
    float D = NormalDistributionGGX(n, wh, alpha);
    vec3 F = Fresnel(wi, wh, F0);
    float G = G_GGX(n, wi, wo, alpha);

    return D * F * G / (4*dot(wi, n)*dot(wo, n));
}

vec3 evaluateRadianceDirectional(Material mat, LightSource source, vec3 n, vec3 pos) {
    vec3 fd = sampleTex(fUV, mat.albedoTex, vec4(mat.albedo, 1.0)).rgb * source.color * source.intensity / PI;

    vec3 wi = -normalize(source.direction); // Light dir
    vec3 wo = normalize(eye - pos);       // Eye dir

    vec3 wh = normalize(wi + wo);

    float alpha = sqr(sampleTex(fUV, mat.roughnessTex, vec4(mat.roughness)).x);

    vec3 fs = F_Specular(n, wi, wo, wh, mat.F0, alpha) * source.color * source.intensity;

    float metalness = sampleTex(fUV, mat.metalnessTex, vec4(mat.metalness)).x;

    return (fd * (1.0 - metalness) + metalness * fs) * max(dot(wi, n), 0.0);
}

vec3 evaluateRadiancePointLight(Material mat, LightSource source, vec3 normal, vec3 pos) {
    float dist = length(source.direction - pos);
    float attenuation = 1.0 / (source.ac + dist * source.al + dist * dist * source.aq);
    vec3 lightDir = -normalize(source.direction - pos);

    LightSource new_source = LightSource(1, lightDir, source.color, source.intensity * attenuation, 0.0, 0.0, 0.0);

    return evaluateRadianceDirectional(mat, new_source, normal, pos);
}

vec3 evaluateRadiance(Material mat, LightSource source, vec3 normal, vec3 pos) {
    if (source.type == 0) {
        return evaluateRadianceDirectional(mat, source, normal, pos);
    } else {
        return evaluateRadiancePointLight(mat, source, normal, fPos);
    }
}

void main() {
    vec3 radiance = vec3(0.);

    vec3 normal = normalize(fNormal);
    if(material.normalTex != -1) {
        vec3 normalMap = sampleTex(fUV, material.normalTex, vec4(0.5, 0.5, 1.0, 1.0)).xyz * 2.0 - 1.0;
        normal = normalize(fTangent * normalMap.x + fBitangent * normalMap.y + fNormal * normalMap.z);
    }
    
    for (int i = 0; i < numOfLights; i++) {
        radiance += evaluateRadiance(material, lights[i], normal, fPos);
    }

    float ao = sampleTex(fUV, material.aoTex, vec4(1.0)).r;
    radiance *= ao;

    if(imageParameters.colorCorrect) {
        if(imageParameters.useExposure)
            radiance *= imageParameters.exposure;
        
        if(imageParameters.useToneMapping)
            radiance = ACESFilm(radiance);
        
        if(imageParameters.useSRGB)
            radiance = LinearToSRGB(radiance);
    }
        
    colorResponse = vec4 (radiance, 1.0);
}
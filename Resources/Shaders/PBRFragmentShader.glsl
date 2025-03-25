#version 450 core // Minimal GL version support expected from the GPU

in vec3 fNormal; // Shader input, linearly interpolated by default from the previous stage (here the vertex shader)
in vec3 fPos;
in vec2 fUV;
in vec3 fTangent;
in vec3 fBitangent;

out vec4 colorResponse; // Shader output: the color response attached to this fragment

const float PI = 3.14159265358979323846;

// XYZ to CIE 1931 RGB color space (using neutral E illuminant)
const mat3 XYZ_TO_RGB = mat3(2.3706743, -0.5138850, 0.0052982, -0.9000405, 1.4253036, -0.0146949, -0.4706338, 0.0885814, 1.0093968);

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

float sqr(float x) { return x * x; }
vec2 sqr(vec2 v) { return vec2(sqr(v.x), sqr(v.y)); }
vec3 sqr(vec3 v) { return vec3(sqr(v.x), sqr(v.y), sqr(v.z)); }

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
float NormalDistributionGGX(float dotNwH, float alpha) {
    float nom = sqr(alpha);
    float denom = PI * sqr(1 + (sqr(alpha) - 1.0) * sqr(dotNwH));

    return nom / denom;
}

// Schlick approximation for geometric term
float G_Schlick(float dotNW, float alpha) {
    float k = alpha * sqrt(2.0 / PI);
    return dotNW / (dotNW * (1-k) + k);
}

float G_GGX(float dotNWi, float dotNWo, float alpha) {
    return G_Schlick(dotNWi, alpha) * G_Schlick(dotNWo, alpha);
}

// Schlick approx for Fresnel term
vec3 Fresnel(vec3 wi, vec3 wh, vec3 F0) {
    return F0 + (1-F0) * pow(1 - max(0, dot(wi, wh)), 5);
}

// Fresnel equations for dielectric/dielectric interfaces.
void fresnelDielectric(in float ct1, in float n1, in float n2,
                       out vec2 R, out vec2 phi) {

  float st1  = (1 - ct1*ct1); // Sinus theta1 'squared'
  float nr  = n1/n2;

  if(sqr(nr)*st1 > 1) { // Total reflection

    vec2 R = vec2(1, 1);
    phi = 2.0 * atan(vec2(- sqr(nr) *  sqrt(st1 - 1.0/sqr(nr)) / ct1,
                        - sqrt(st1 - 1.0/sqr(nr)) / ct1));
  } else {   // Transmission & Reflection

    float ct2 = sqrt(1 - sqr(nr) * st1);
    vec2 r = vec2((n2*ct1 - n1*ct2) / (n2*ct1 + n1*ct2),
        	     (n1*ct1 - n2*ct2) / (n1*ct1 + n2*ct2));
    phi.x = (r.x < 0.0) ? PI : 0.0;
    phi.y = (r.y < 0.0) ? PI : 0.0;
    R = sqr(r);
  }
}

// Fresnel equations for dielectric/conductor interfaces.
void fresnelConductor(in float ct1, in float n1, in float n2, in float k,
                       out vec2 R, out vec2 phi) {

	if (k==0) { // use dielectric formula to avoid numerical issues
		fresnelDielectric(ct1, n1, n2, R, phi);
		return;
	}

	float A = sqr(n2) * (1-sqr(k)) - sqr(n1) * (1-sqr(ct1));
	float B = sqrt( sqr(A) + sqr(2*sqr(n2)*k) );
	float U = sqrt((A+B)/2.0);
	float V = sqrt((B-A)/2.0);

	R.y = (sqr(n1*ct1 - U) + sqr(V)) / (sqr(n1*ct1 + U) + sqr(V));
	phi.y = atan( 2*n1 * V*ct1, sqr(U)+sqr(V)-sqr(n1*ct1) ) + PI;

	R.x = ( sqr(sqr(n2)*(1-sqr(k))*ct1 - n1*U) + sqr(2*sqr(n2)*k*ct1 - n1*V) ) 
			/ ( sqr(sqr(n2)*(1-sqr(k))*ct1 + n1*U) + sqr(2*sqr(n2)*k*ct1 + n1*V) );
	phi.x = atan( 2*n1*sqr(n2)*ct1 * (2*k*U - (1-sqr(k))*V), sqr(sqr(n2)*(1+sqr(k))*ct1) - sqr(n1)*(sqr(U)+sqr(V)) );
}

// Depolarization functions for natural light
float depol (vec2 polV){ return 0.5 * (polV.x + polV.y); }
vec3 depolColor (vec3 colS, vec3 colP){ return 0.5 * (colS + colP); }

// Evaluation XYZ sensitivity curves in Fourier space
vec3 evalSensitivity(float opd, float shift) {

	// Use Gaussian fits, given by 3 parameters: val, pos and var
	float phase = 2*PI * opd * 1.0e-6;
	vec3 val = vec3(5.4856e-13, 4.4201e-13, 5.2481e-13);
	vec3 pos = vec3(1.6810e+06, 1.7953e+06, 2.2084e+06);
	vec3 var = vec3(4.3278e+09, 9.3046e+09, 6.6121e+09);
	vec3 xyz = val * sqrt(2*PI * var) * cos(pos * phase + shift) * exp(- var * phase*phase);
	xyz.x   += 9.7470e-14 * sqrt(2*PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift) * exp(- 4.5282e+09 * phase*phase);
	return xyz / 1.0685e-7;
}

vec3 IridescentTerm(in float dotNWi, in float dotNWo, in float dotNWh, in float cosTheta1, in Material material) {
    float alpha = sqr(material.roughness);

	float cosTheta2 = sqrt(1.0 - sqr(1.0/material.eta2)*(1.0-sqr(cosTheta1))); // Refracted angle

    // First interface

    vec2 R12, phi12;
	fresnelDielectric(cosTheta1, 1.0, material.eta2, R12, phi12);
	vec2 R21  = R12;
	vec2 T121 = vec2(1.0) - R12;
	vec2 phi21 = vec2(PI) - phi12;

    // Second interface
	vec2 R23, phi23;
	fresnelConductor(cosTheta2, material.eta2, material.eta3, material.kappa3, R23, phi23);

    // Phase shift
	float OPD = material.Dinc*cosTheta2;
	vec2 phi2 = phi21 + phi23;

    // Compound terms
	vec3 I = vec3(0);
	vec2 R123 = R12*R23;
	vec2 r123 = sqrt(R123);
	vec2 Rs   = sqr(T121)*R23 / (1-R123);

    // Reflectance term for m=0 (DC term amplitude)
	vec2 C0 = R12 + Rs;
	vec3 S0 = evalSensitivity(0.0, 0.0);
	I += depol(C0) * S0;

    // Reflectance term for m>0 (pairs of diracs)
	vec2 Cm = Rs - T121;
	for (int m=1; m<=3; ++m){
		Cm *= r123; // Compute Cm in the loop 
		vec3 SmS = 2.0 * evalSensitivity(m*OPD, m*phi2.x);
		vec3 SmP = 2.0 * evalSensitivity(m*OPD, m*phi2.y);
		I += depolColor(Cm.x*SmS, Cm.y*SmP);
	}

    // Convert back to RGB reflectance
	I = clamp(XYZ_TO_RGB * I, vec3(0.0), vec3(1.0));

    return I;

}

vec3 F_Specular(vec3 n, vec3 wi, vec3 wo, in Material material) {
    float alpha = sqr(material.roughness);

    float dotNWi = dot(n, wi);
    float dotNWo = dot(n, wo);
    
    if (dotNWi <= 0 || dotNWo <= 0) return vec3(0.0);
    vec3 wh = normalize(wi + wo);
    float dotNWh = dot(n, wh);

    float cosTheta1 = dot(wi, wh);

    float D = NormalDistributionGGX(dotNWh, alpha);
    float G = G_GGX(dotNWi, dotNWo, alpha);
    vec3 I = IridescentTerm(dotNWi, dotNWo, dotNWh, cosTheta1, material);

    return D * G * I / (4*dotNWi*dotNWo);
}

vec3 evaluateRadianceDirectional(Material mat, LightSource source, vec3 n, vec3 pos) {
    vec3 fd = sampleTex(fUV, mat.albedoTex, vec4(mat.albedo, 1.0)).rgb * source.color * source.intensity / PI;

    vec3 wi = -normalize(source.direction); // Light dir
    vec3 wo = normalize(eye - pos);       // Eye dir

    mat.roughness = sampleTex(fUV, mat.roughnessTex, vec4(mat.roughness)).x;

    vec3 fs = F_Specular(n, wi, wo, mat) * source.color * source.intensity;

    mat.metalness = sampleTex(fUV, mat.metalnessTex, vec4(mat.metalness)).x;

    return (fd * (1.0 - mat.metalness) + mat.metalness * fs) * max(dot(wi, n), 0.0);
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
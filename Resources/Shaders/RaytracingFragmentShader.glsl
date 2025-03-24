#version 450 core

const float PI = 3.14159265358979323846;
#define MAX_LIGHTS 10
#define MAX_TEXTURES 16

struct ImageParameters {
    bool colorCorrect;
    bool useSRGB;
    bool useToneMapping;
    bool useExposure;
    float exposure;
    bool raytracedShadows;
    bool raytracedReflections;
    int numRefractions;
};

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

struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 inv_direction;
};

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

struct Hit {
    bool hit;
    float t;
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 uv;
    int triangle_index;
    int model_index;
    bool backface;
};

struct Triangle {
    vec3 a;
    vec3 b;
    vec3 c;
};

struct Model {
    int bvh_root;
    int triangle_offset;
    int triangle_count;
    int vertex_offset;

    Material material;
    
    mat4 transform;
    mat4 inv_transform;
};

struct BVH_Node {
    vec3 min;
    int triangle_count;
    vec3 max;
    int offset;
};

struct Vertex {
    vec3 position;
    float u;
    vec3 normal;
    float v;
    vec3 tangent;
    float p;
};

uniform mat4 inv_view_mat;
uniform mat4 inv_proj_mat;
uniform mat4 proj_mat;
uniform mat4 view_mat;
uniform vec2 dim;
uniform vec3 eye;

uniform vec3 skyColor;

uniform ImageParameters imageParameters;

uniform LightSource lights[MAX_LIGHTS];
uniform int numOfLights;

uniform float zNear;
uniform float zFar;

uniform sampler2D textures[MAX_TEXTURES];

in vec2 fPos;
out vec4 colorResponse;

layout(binding = 0, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(binding = 1, std430) readonly buffer TriangleBuffer {
    uvec4 triangles[];
};

layout(binding = 2, std430) readonly buffer ModelBuffer {
    Model models[];
};

layout(binding = 3, std430) buffer BVHBuffer {
    BVH_Node bvh_nodes[];
};

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

vec3 evaluateRadianceDirectional(Material mat, LightSource source, vec3 n, vec3 pos, Ray ray, vec2 uv) {
    vec3 albedo = sampleTex(uv, mat.albedoTex, vec4(mat.albedo, 1.0)).rgb;
    vec3 fd = albedo * source.color * source.intensity / PI;

    vec3 wi = -normalize(source.direction); // Light dir
    vec3 wo = normalize(ray.origin - pos);       // Eye dir

    vec3 wh = normalize(wi + wo);

    float alpha = sqr(sampleTex(uv, mat.roughnessTex, vec4(mat.roughness)).x);

    vec3 fs = F_Specular(n, wi, wo, wh, mat.F0, alpha) * source.color * source.intensity;

    float metalness = sampleTex(uv, mat.metalnessTex, vec4(mat.metalness)).x;

    return (fd * (1.0 - metalness) + metalness * fs) * max(dot(wi, n), 0.0);
}

vec3 evaluateRadiancePointLight(Material mat, LightSource source, vec3 normal, vec3 pos, Ray ray, vec2 uv) {
    float dist = length(source.direction - pos);
    float attenuation = 1.0 / (source.ac + dist * source.al + dist * dist * source.aq);
    vec3 lightDir = -normalize(source.direction - pos);

    LightSource new_source = LightSource(1, lightDir, source.color, source.intensity * attenuation, 0.0, 0.0, 0.0);

    return evaluateRadianceDirectional(mat, new_source, normal, pos, ray, uv);
}

vec3 evaluateRadiance(Material mat, LightSource source, vec3 normal, vec3 pos, Ray ray, vec2 uv) {
    if (source.type == 0) {
        return evaluateRadianceDirectional(mat, source, normal, pos, ray, uv);
    } else {
        return evaluateRadiancePointLight(mat, source, normal, pos, ray, uv);
    }
}

vec3 getBarycentric(in vec3 p, in Triangle t) {
    vec3 v0 = t.b - t.a, v1 = t.c - t.a, v2 = p - t.a;
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);

    float denom_inv = 1.0 / (d00 * d11 - d01 * d01);

    float v = (d11 * d20 - d01 * d21) * denom_inv;
    float w = (d00 * d21 - d01 * d20) * denom_inv;
    float u = 1.0 - v - w;

    return vec3(u, v, w);
}

bool triangleIntersection(in Ray ray, in Triangle triangle, inout Hit hit) {
    float epsilon = 0.0000001;

    vec3 edge1 = triangle.b - triangle.a;
    vec3 edge2 = triangle.c - triangle.a;
    vec3 ray_cross_e2 = cross(ray.direction, edge2);
    float det = dot(edge1, ray_cross_e2);

    if (det > -epsilon && det < epsilon)
        return false;    // This ray is parallel to this triangle.

    float inv_det = 1.0 / det;
    vec3 s = ray.origin - triangle.a;
    float u = inv_det * dot(s, ray_cross_e2);

    if ((u < 0 && abs(u) > epsilon) || (u > 1 && abs(u - 1) > epsilon))
        return false;

    vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(ray.direction, s_cross_e1);

    if ((v < 0 && abs(v) > epsilon) || (u + v > 1 && abs(u + v - 1) > epsilon))
        return false;

    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = inv_det * dot(edge2, s_cross_e1);

    if (t > epsilon) {
        if (!hit.hit || t < hit.t) {
            hit.hit = true;
            hit.t = t;
            hit.position = ray.origin + ray.direction * t;
            hit.normal = normalize(cross(edge1, edge2));
            // if (dot(hit.normal, ray.direction) > 0) {
            //     hit.normal = -hit.normal;
            //     hit.backface = true;
            // }

            return true;
        }

    }
    return false;
}

float AABBIntersection(in Ray ray, vec3 minp, vec3 maxp) {
    if (ray.origin.x >= minp.x && ray.origin.x <= maxp.x &&
        ray.origin.y >= minp.y && ray.origin.y <= maxp.y &&
        ray.origin.z >= minp.z && ray.origin.z <= maxp.z) {
        return 0.0;
    }


    float tx1 = (minp.x - ray.origin.x) * ray.inv_direction.x;
    float tx2 = (maxp.x - ray.origin.x) * ray.inv_direction.x;

    float tmin = min(tx1, tx2);
    float tmax = max(tx1, tx2);

    float ty1 = (minp.y - ray.origin.y) * ray.inv_direction.y;
    float ty2 = (maxp.y - ray.origin.y) * ray.inv_direction.y;

    tmin = max(tmin, min(ty1, ty2));
    tmax = min(tmax, max(ty1, ty2));

    float tz1 = (minp.z - ray.origin.z) * ray.inv_direction.z;
    float tz2 = (maxp.z - ray.origin.z) * ray.inv_direction.z;

    tmin = max(tmin, min(tz1, tz2));
    tmax = min(tmax, max(tz1, tz2));

    if (tmax >= tmin && tmin >= 0) {
        return tmin;
    }

    return -1.0;
}

Hit traceRayBVH(in Ray ray) {
    Hit hit = Hit(false, 1000000.0, vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), vec2(0.0), -1, -1, false);
    
    int node_stack[64];

    for(int i=0; i<models.length(); i++) {
        
        Model model = models[i];
        Ray transformed_ray;
        transformed_ray.origin = vec3(model.inv_transform * vec4(ray.origin, 1.0));
        transformed_ray.direction = vec3(model.inv_transform * vec4(ray.direction, 0.0));
        transformed_ray.inv_direction = 1.0 / transformed_ray.direction;

        Hit transformed_hit = hit;
        if(hit.hit) {
            transformed_hit.position = vec3(model.inv_transform * (models[hit.model_index].transform * vec4(hit.position, 1.0)));
            transformed_hit.t = length(transformed_hit.position - transformed_ray.origin) / length(transformed_ray.direction); // The normal is not normalized in object space
        }

        int stack_pointer = 0;
        node_stack[stack_pointer++] = model.bvh_root;

        while(stack_pointer > 0) {
            int node_index = node_stack[--stack_pointer];
            BVH_Node node = bvh_nodes[node_index];

            float t = AABBIntersection(transformed_ray, node.min, node.max);
            if(t < 0.0 || t > hit.t)
                continue;

            if(node.triangle_count > 0) {
                for(int j = 0; j < node.triangle_count; j++) {
                    uvec4 triangle_indices = triangles[model.triangle_offset + node.offset + j] + model.vertex_offset;
                    Triangle triangle = Triangle(
                        vertices[triangle_indices.x].position,
                        vertices[triangle_indices.y].position,
                        vertices[triangle_indices.z].position
                    );
                    if(triangleIntersection(transformed_ray, triangle, transformed_hit)) {
                        hit = transformed_hit;
                        hit.triangle_index = node.offset + j;
                        hit.model_index = i;
                    }
                }
            } else {
                int first = model.bvh_root + node.offset + 0;
                int second = model.bvh_root + node.offset + 1;

                vec3 left_center = 0.5 * (bvh_nodes[first].min + bvh_nodes[first].max);
                vec3 right_center = 0.5 * (bvh_nodes[second].min + bvh_nodes[second].max);

                float dist_left = dot(left_center - transformed_ray.origin, left_center - transformed_ray.origin);
                float dist_right = dot(right_center - transformed_ray.origin, right_center - transformed_ray.origin);

                if(dist_left < dist_right) {
                    first = model.bvh_root + node.offset + 1;
                    second = model.bvh_root + node.offset + 0;
                } else {
                    first = model.bvh_root + node.offset + 0;
                    second = model.bvh_root + node.offset + 1;
                }
                
                node_stack[stack_pointer++] = first;
                node_stack[stack_pointer++] = second; 
            }
        }
    }

    if(hit.hit) {
        Model model = models[hit.model_index];
        // Compute normal with barycentric coordinates
        uvec4 triangle_indices = triangles[model.triangle_offset + hit.triangle_index] + model.vertex_offset;
        Triangle triangle = Triangle(
            vertices[triangle_indices.x].position,
            vertices[triangle_indices.y].position,
            vertices[triangle_indices.z].position
        );

        vec3 barycentric = getBarycentric(hit.position, triangle);
        hit.normal = normalize(
            barycentric.x * vertices[triangle_indices.x].normal +
            barycentric.y * vertices[triangle_indices.y].normal +
            barycentric.z * vertices[triangle_indices.z].normal
        );

        hit.tangent = normalize(
            barycentric.x * vertices[triangle_indices.x].tangent +
            barycentric.y * vertices[triangle_indices.y].tangent +
            barycentric.z * vertices[triangle_indices.z].tangent
        );

        hit.bitangent = cross(hit.normal, hit.tangent);

        hit.uv = (
            barycentric.x * vec2(vertices[triangle_indices.x].u, vertices[triangle_indices.x].v) +
            barycentric.y * vec2(vertices[triangle_indices.y].u, vertices[triangle_indices.y].v) +
            barycentric.z * vec2(vertices[triangle_indices.z].u, vertices[triangle_indices.z].v));

        // Transform hit info to world space

        hit.position = vec3(model.transform * vec4(hit.position, 1.0));
        hit.normal = normalize(vec3(transpose(model.inv_transform) * vec4(hit.normal, 0.0)));
        hit.t = length(hit.position - ray.origin);

        if(dot(hit.normal, ray.direction) > 0) {
            hit.normal = -hit.normal;
            hit.backface = true;
        }
    }

    return hit;    
}

void rayAt(out Ray ray, vec2 uv) {
    vec4 clip = vec4(uv, -1.0, 1.0);
    vec4 eye = vec4(vec2(inv_proj_mat * clip), -1.0, 0.0);
    ray.direction = normalize(vec3(inv_view_mat * eye));
    ray.origin = vec3(inv_view_mat[3]);
    ray.inv_direction = 1.0 / ray.direction;
}

float normalizeDepth(float depth) {
    return (1.0 / depth - 1.0 / zNear) / (1.0 / zFar - 1.0 / zNear);
}

vec3 shade(vec3 normal, Hit hit, Material material, Ray ray, vec2 uv, bool shadows = false) {
    vec3 radiance = vec3(0.0);

    for (int i = 0; i < numOfLights; i++) {
        bool contribute = true; // Shadow ray

        if(imageParameters.raytracedShadows && shadows) {
            LightSource light = lights[i];
            Ray shadow_ray;
            shadow_ray.origin = hit.position + 0.001 * normal;
            if(light.type == 0)
                shadow_ray.direction = -normalize(light.direction);
            else
                shadow_ray.direction = normalize(light.direction - hit.position);
            shadow_ray.inv_direction = 1.0 / shadow_ray.direction;

            if(dot(shadow_ray.direction, normal) > 0) {
                Hit shadow_hit = traceRayBVH(shadow_ray);
                if (shadow_hit.hit){
                    if(light.type == 0) {
                        contribute = false;
                    } else {
                        if(shadow_hit.t < length(light.direction - hit.position))
                            contribute = false;
                    }
                }
            } else
                contribute = false;
        }
        
        if(contribute)
            radiance += evaluateRadiance(material, lights[i], normal, hit.position, ray, uv);
    }
    return radiance;
}

float FresnelReflectAmount (float n1, float n2, vec3 normal, vec3 incident, float reflectance = 0.0) {
    // Schlick aproximation
    float r0 = (n1-n2) / (n1+n2);
    r0 *= r0;
    float cosX = -dot(normal, incident);
    if (n1 > n2) {
        float n = n1/n2;
        float sinT2 = n*n*(1.0-cosX*cosX);
        // Total internal reflection
        if (sinT2 > 1.0)
            return 1.0;
        cosX = sqrt(1.0-sinT2);
    }
    float x = 1.0-cosX;
    float ret = r0+(1.0-r0)*x*x*x*x*x;

    // adjust reflect multiplier for object reflectivity
    ret = (reflectance + (1.0-reflectance) * ret);
    return ret;
}

vec3 dirToSun = -normalize(lights[0].direction);

vec3 SampleSky(vec3 dir) {
    const vec3 colGround = vec3(0.35, 0.3, 0.35) * 0.53;
    const vec3 colSkyHorizon = vec3(1, 1, 1);
    const vec3 colSkyZenith = vec3(0.08, 0.37, 0.73);

    float sun = pow(max(0, dot(dir, dirToSun)), 500) * 10;
    float skyGradientT = pow(smoothstep(0.0, 0.4, dir.y), 0.35);
    float groundToSkyT = smoothstep(-0.01, 0.0, dir.y);
    vec3 skyGradient = mix(colSkyHorizon, colSkyZenith, skyGradientT);

    return mix(colGround, skyGradient, groundToSkyT) + sun * (groundToSkyT >= 1 ? 1.0 : 0.0);
}
Ray getReflectionRay(vec3 normal, vec3 incident, vec3 position) {
    Ray reflection_ray;
    reflection_ray.origin = position + 0.001 * normal;
    reflection_ray.direction = reflect(incident, normal);
    reflection_ray.inv_direction = 1.0 / reflection_ray.direction;
    return reflection_ray;
}

Ray getRefractionRay(vec3 normal, vec3 incident, vec3 position, float ior, bool backface = false) {
    Ray refraction_ray;
    refraction_ray.origin = position - 0.001 * normal;
    float eta = ior;
    if(!backface)
        eta = 1.0 / eta;
    refraction_ray.direction = normalize(refract(incident, normal, eta));
    refraction_ray.inv_direction = 1.0 / refraction_ray.direction;
    return refraction_ray;
}

void main() {
    Ray ray;
    rayAt(ray, fPos);
    
    Hit hit = traceRayBVH(ray);

    Hit first_hit = hit;

    vec3 radiance;

    if (hit.hit) {
        Model model = models[hit.model_index];

        float energy = 1.0;
        
        for(int i=0; i<imageParameters.numRefractions+1; i++) {
            model = models[hit.model_index];

            if(model.material.normalTex != -1) {
            vec3 normalMap = sampleTex(hit.uv, model.material.normalTex, vec4(0.5, 0.5, 1.0, 1.0)).xyz * 2.0 - 1.0;
            hit.normal = normalize(hit.tangent * normalMap.x + hit.bitangent * normalMap.y + hit.normal * normalMap.z);
        }

            if(model.material.transparent) {
                float reflectance = FresnelReflectAmount(1.0, 1.3, hit.normal, ray.direction, model.material.base_reflectance);
                float transmittance = 1.0 - reflectance;
                
                Ray reflection_ray = getReflectionRay(hit.normal, ray.direction, hit.position);
                vec3 reflectedColor = SampleSky(reflection_ray.direction);

                Ray refraction_ray = getRefractionRay(hit.normal, ray.direction, hit.position, model.material.ior, hit.backface);
                vec3 refractedColor = SampleSky(refraction_ray.direction);

                bool is_refracted = transmittance > reflectance;

                if(is_refracted) {
                    radiance += energy * reflectance * reflectedColor;
                    energy *= transmittance;
                    Hit refraction_hit = traceRayBVH(refraction_ray);
                    if(refraction_hit.hit) {
                        ray = refraction_ray;
                        hit = refraction_hit;
                    } else {
                        radiance += energy * transmittance * refractedColor;
                        break;
                    }
                } else {
                    radiance += energy * transmittance * refractedColor;
                    energy *= reflectance;
                    Hit reflection_hit = traceRayBVH(reflection_ray);
                    if(reflection_hit.hit) {
                        ray = reflection_ray;
                        hit = reflection_hit;
                    } else {
                        radiance += energy * reflectance * reflectedColor;
                        break;
                    }
                }
            } else {
                radiance += energy * shade(hit.normal, hit, model.material, ray, hit.uv, true);
                break;
            }
        }
        
    } else
        radiance = SampleSky(ray.direction);

    if(imageParameters.colorCorrect) {
        if(imageParameters.useExposure)
            radiance *= imageParameters.exposure;
        
        if(imageParameters.useToneMapping)
            radiance = ACESFilm(radiance);
        
        if(imageParameters.useSRGB)
            radiance = LinearToSRGB(radiance);
    }
        
    colorResponse = vec4 (radiance, 1.0);

    if(first_hit.hit) {
        vec4 projected = proj_mat * view_mat * vec4(first_hit.position, 1.0);    
        gl_FragDepth = (projected.z/projected.w + 1.0) * 0.5;
    }
    else
        gl_FragDepth = 1.0;
}
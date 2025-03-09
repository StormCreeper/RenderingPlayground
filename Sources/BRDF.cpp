#include "BRDF.h"

float sqr(float x) {
    return x * x;
}

glm::vec3 pow3(glm::vec3 v, int p) {
    return glm::vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

// Trowbridge-Reitz Normal Distribution
float BRDF::NormalDistributionGGX(glm::vec3 n, glm::vec3 wh, float alpha) {
    float nom = sqr(alpha);
    float denom = glm::pi<float>() * sqr(1 + (sqr(alpha) - 1.0f) * sqr(glm::dot(n, wh)));

    return nom / denom;
}

// F0 from the index of refraction n
glm::vec3 BRDF::GetF0(glm::vec3 n) {
    return pow3((n - glm::vec3(1.0)) / (n + glm::vec3(1.0f)), 2);
}

// Schlick approx for Fresnel term
glm::vec3 BRDF::Fresnel(glm::vec3 wi, glm::vec3 wh, glm::vec3 F0) {
    return F0 + (glm::vec3(1.0f) - F0) * pow3(glm::vec3(1.0f) - glm::max(glm::vec3(0.0f), glm::dot(wi, wh)), 5);
}

// Schlick approximation for geometric term
float BRDF::G_Schlick(glm::vec3 n, glm::vec3 w, float alpha) {
    float k = alpha * sqrt(2.0f / glm::pi<float>());
    float dotNW = glm::dot(n, w);
    return dotNW / (dotNW * (1 - k) + k);
}

float BRDF::G_GGX(glm::vec3 n, glm::vec3 wi, glm::vec3 wo, float alpha) {
    return G_Schlick(n, wi, alpha) * G_Schlick(n, wo, alpha);
}

glm::vec3 BRDF::F(const glm::vec3& wi, const glm::vec3& wo, const glm::vec3& n) {
    glm::vec3 fd = material.albedo() / glm::pi<float>();

    glm::vec3 wh = normalize(wi + wo);

    float alpha = sqr(material.roughness());

    float D = NormalDistributionGGX(n, wh, alpha);

    glm::vec3 F0 = material.F0();

    glm::vec3 F = Fresnel(wi, wh, F0);

    float G = G_GGX(n, wi, wo, alpha);

    glm::vec3 fs = D * F * G / (4 * glm::dot(wi, n) * glm::dot(wo, n));

    return (fd * (1.0f - material.metalness()) + fs) * glm::max(glm::dot(wi, n), 0.0f);
}

glm::vec3 evaluateRadiance(const Material& mat, const std::shared_ptr<AbstractLight>& source, const glm::vec3& normal, const glm::vec3& wo, const glm::vec3& frag_pos) {
    float intensity = source->intensity(frag_pos);
    glm::vec3 wi = source->wi(frag_pos);

    return BRDF(mat).F(wi, wo, normal) * intensity * source->color();
}

glm::vec3 evaluateTotalRadiance(const Scene& scene, const glm::vec3& frag_pos, const glm::vec3& fNormal, const Material& material, const glm::vec3& eye) {
    glm::vec3 colorResponse(0.0f);
    glm::vec3 wo = glm::normalize(eye - frag_pos);

    for (int i = 0; i < scene.numOfLights(); i++) {
        colorResponse += evaluateRadiance(material, scene.light(i), normalize(fNormal), wo, frag_pos);
    }

    return colorResponse;
}
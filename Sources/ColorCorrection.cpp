#include "ColorCorrection.h"


// From https://blog.demofox.org/2020/06/06/casual-shadertoy-path-tracing-2-image-improvement-and-glossy-reflections/
glm::vec3 LessThan(glm::vec3 f, float value) {
    return glm::vec3(
        f.x < value ? 1.0f : 0.0f,
        f.y < value ? 1.0f : 0.0f,
        f.z < value ? 1.0f : 0.0f
    );
}

glm::vec3 LinearToSRGB(glm::vec3 rgb) {
    rgb = glm::clamp(rgb, 0.0f, 1.0f);

    return mix(
        glm::pow(rgb, glm::vec3(1.0f / 2.4f)) * 1.055f - 0.055f,
        rgb * 12.92f,
        LessThan(rgb, 0.0031308f)
    );
}

glm::vec3 SRGBToLinear(glm::vec3 rgb) {
    rgb = glm::clamp(rgb, 0.0f, 1.0f);

    return mix(
        glm::pow(((rgb + 0.055f) / 1.055f), glm::vec3(2.4f)),
        rgb / 12.92f,
        LessThan(rgb, 0.04045f)
    );
}


// ACES tone mapping curve fit to go from HDR to LDR
//https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
glm::vec3 ACESFilm(glm::vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}
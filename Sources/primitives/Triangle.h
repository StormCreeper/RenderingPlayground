#pragma once

#include <glm/glm.hpp>

struct Triangle {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;

    glm::vec3 centroid() const {
        return (a + b + c) / 3.0f;
    }
};
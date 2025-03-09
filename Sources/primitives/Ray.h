#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class BVH;

struct Hit {
    bool hit = false;
    float t = std::numeric_limits<float>::max();
    glm::vec3 position;
    glm::vec3 normal;
    size_t meshIndex;
    size_t triangleIndex;
};

class Ray {
    glm::vec3 _origin;
    glm::vec3 _direction;
    glm::vec3 _inv_direction;

public:
    Ray(const glm::vec3& origin, const glm::vec3& direction) : _origin(origin), _direction(direction), _inv_direction(1.0f / direction) {}

    inline const glm::vec3& origin() const { return _origin; }
    inline glm::vec3& origin() { return _origin; }

    inline const glm::vec3& direction() const { return _direction; }
    inline void setDirection(const glm::vec3& direction) { _direction = direction; _inv_direction = 1.0f / direction; }

    inline const glm::vec3& inv_direction() const { return _inv_direction; }
};


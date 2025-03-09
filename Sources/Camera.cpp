#include "Camera.h"


Ray Camera::rayAt(glm::vec2 pixel, glm::vec2 dim) {
    glm::vec2 uv = (pixel + 0.5f) / dim;

    glm::vec4 clip = glm::vec4(uv * 2.0f - 1.0f, -1.0, 1.0);
    glm::vec4 eye = glm::vec4(glm::vec2(inv_proj_mat * clip), -1.0, 0.0);
    glm::vec3 rayDir = glm::normalize(glm::vec3(inv_view_mat * eye));
    glm::vec3 rayOrigin = glm::vec3(inv_view_mat[3]);

    return Ray{ rayOrigin, rayDir };
}
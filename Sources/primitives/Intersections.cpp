#include "Intersections.h"

#include <set>

bool triangleIntersection(const Ray& ray, const Triangle& triangle, Hit& hit) {
    constexpr float epsilon = std::numeric_limits<float>::epsilon();

    glm::vec3 edge1 = triangle.b - triangle.a;
    glm::vec3 edge2 = triangle.c - triangle.a;
    glm::vec3 ray_cross_e2 = glm::cross(ray.direction(), edge2);
    float det = glm::dot(edge1, ray_cross_e2);

    if (det > -epsilon && det < epsilon)
        return false;    // This ray is parallel to this triangle.

    float inv_det = 1.0f / det;
    glm::vec3 s = ray.origin() - triangle.a;
    float u = inv_det * dot(s, ray_cross_e2);

    if ((u < 0 && abs(u) > epsilon) || (u > 1 && abs(u - 1) > epsilon))
        return false;

    glm::vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * glm::dot(ray.direction(), s_cross_e1);

    if ((v < 0 && abs(v) > epsilon) || (u + v > 1 && abs(u + v - 1) > epsilon))
        return false;

    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = inv_det * dot(edge2, s_cross_e1);

    if (t > epsilon) // ray intersection
    {
        hit.hit = true;
        if (t < hit.t) {
            hit.t = t;
            hit.position = ray.origin() + ray.direction() * t;
            hit.normal = glm::normalize(glm::cross(edge1, edge2));

            return true;
        }

    }
    return false;
}

bool AABBIntersection(const Ray& ray, const AABB& box, Hit& hit) {
    // if origin inside the box
    if (ray.origin().x >= box.begin_corner.x && ray.origin().x <= box.end_corner.x &&
        ray.origin().y >= box.begin_corner.y && ray.origin().y <= box.end_corner.y &&
        ray.origin().z >= box.begin_corner.z && ray.origin().z <= box.end_corner.z) {
        hit.hit = true;
        hit.t = 0;
        hit.position = ray.origin();
        hit.normal = glm::vec3(0.0f);
        return true;
    }


    double tx1 = (box.begin_corner.x - ray.origin().x) * ray.inv_direction().x;
    double tx2 = (box.end_corner.x - ray.origin().x) * ray.inv_direction().x;

    double tmin = std::min(tx1, tx2);
    double tmax = std::max(tx1, tx2);

    double ty1 = (box.begin_corner.y - ray.origin().y) * ray.inv_direction().y;
    double ty2 = (box.end_corner.y - ray.origin().y) * ray.inv_direction().y;

    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));

    double tz1 = (box.begin_corner.z - ray.origin().z) * ray.inv_direction().z;
    double tz2 = (box.end_corner.z - ray.origin().z) * ray.inv_direction().z;

    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));

    if (tmax >= tmin && tmin >= 0) {
        hit.hit = true;
        hit.t = tmin;
        hit.position = ray.origin() + ray.direction() * static_cast<float>(tmin);
        hit.normal = glm::vec3(0.0f);
        return true;
    }

    return false;
}

const float distSq(const glm::vec3& a, const glm::vec3& b) {
    return glm::dot(a - b, a - b);
}

const void traverseBVH_Rec(const Ray& ray, int nodeIndex, const BVH& bvh, std::set<size_t>& res) {
    auto node = bvh.nodes()[nodeIndex];
    if (!node || node->num_triangles == 0)
        return;

    Hit hit;
    if (AABBIntersection(ray, *node->aabb, hit)) {
        if (node->num_triangles == 1 || node->childIndex == 0) {
            for (size_t i = node->first; i < node->first + node->num_triangles; i++)
                res.emplace(i);
            return;
        }

        traverseBVH_Rec(ray, node->childIndex, bvh, res);
        traverseBVH_Rec(ray, node->childIndex + 1, bvh, res);
    }
}

const std::vector<size_t> traverseBVH(const Ray& ray, const BVH& bvh) {
    std::set<size_t> res;
    traverseBVH_Rec(ray, 0, bvh, res);

    return std::vector<size_t>(res.begin(), res.end());
}

bool BVHIntersection_Rec(const Ray& ray, int nodeIndex, const BVH& bvh, Hit& hit) {
    auto node = bvh.nodes()[nodeIndex];
    if (!node || node->num_triangles == 0)
        return false;

    if (node->childIndex == 0) {
        bool new_hit = false;
        for (size_t i = node->first; i < node->first + node->num_triangles; i++) {
            glm::uvec3 triangle = bvh.triangles[i];
            const glm::vec3& a = node->parent->vertexPositions()[triangle.x];
            const glm::vec3& b = node->parent->vertexPositions()[triangle.y];
            const glm::vec3& c = node->parent->vertexPositions()[triangle.z];

            if (triangleIntersection(ray, { a, b, c }, hit)) {
                hit.triangleIndex = i;
                new_hit = true;
            }
        }
        return new_hit;
    }

    glm::vec3 left_center = bvh.nodes()[node->childIndex]->aabb->center();
    glm::vec3 right_center = bvh.nodes()[node->childIndex + 1]->aabb->center();

    int firstIndex, secondIndex;

    if (distSq(ray.origin(), left_center) < distSq(ray.origin(), right_center)) {
        firstIndex = node->childIndex;
        secondIndex = node->childIndex + 1;
    }
    else {
        firstIndex = node->childIndex + 1;
        secondIndex = node->childIndex;
    }
    Hit AABB1_hit, AABB2_hit;
    bool hit1 = AABBIntersection(ray, *bvh.nodes()[firstIndex]->aabb, AABB1_hit);
    bool hit2 = AABBIntersection(ray, *bvh.nodes()[secondIndex]->aabb, AABB2_hit);

    bool first_hit = false;

    if (hit1)
        if (BVHIntersection_Rec(ray, firstIndex, bvh, hit)) {
            first_hit = true;
            if (!hit2 || hit.t < AABB2_hit.t)
                return true;
        }

    if (hit2)
        if (BVHIntersection_Rec(ray, secondIndex, bvh, hit))
            first_hit = true;

    return first_hit;
}

bool BVHIntersection(const Ray& ray, const BVH& bvh, Hit& hit) {
    Hit temp_hit;
    if (!AABBIntersection(ray, *bvh.getRoot()->aabb, temp_hit))
        return false;
    return BVHIntersection_Rec(ray, 0, bvh, hit);
}
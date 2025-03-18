#include "Ray.h"

#include "primitives/Triangle.h"
#include "primitives/AABB.h"
#include "acceleration/BVH.h"

bool triangleIntersection(const Ray& ray, const Triangle& triangle, Hit& hit);

bool AABBIntersection(const Ray& ray, const AABB& box, Hit& hit);

const std::vector<size_t> traverseBVH(const Ray& ray, const BVH& bvh);

bool BVHIntersection(const Ray& ray, const BVH& bvh, Hit& hit);
#include "BVH.h"

#include <algorithm>

int BVH::BUILD_TYPE = 1;
int BVH::NUM_SPLIT_CANDIDATES = 10;

Triangle getTriangle(glm::uvec3 tri_i, const std::vector<glm::vec3>& positions) {
    return {
        positions[tri_i.x],
        positions[tri_i.y],
        positions[tri_i.z],
    };
}

void BVH_Node::recomputeAABB() {
    aabb = std::make_shared<AABB>();

    for (size_t i = first; i < first + num_triangles; i++) {
        Triangle tri = getTriangle(
            parent->bvh()->triangles[i],
            parent->vertexPositions());

        aabb->extend(tri);
    }
}

BVH::BVH(std::shared_ptr<Mesh> meshPtr) {
    parent = meshPtr;

    root = std::make_shared<BVH_Node>();

    root->first = 0;
    root->num_triangles = meshPtr->triangleIndices().size();

    _nodes.push_back(root);

    triangles.assign(meshPtr->triangleIndices().begin(), meshPtr->triangleIndices().end());
}

float evaluateSplit(std::shared_ptr<BVH_Node> node, size_t splitAxis, float splitPos) {
    AABB leftAABB, rightAABB;
    int numTrianglesLeft = 0, numTrianglesRight = 0;

    // Build left and right AABBs
    for (size_t i = node->first; i < node->first + node->num_triangles; i++) {
        Triangle tri = getTriangle(
            node->parent->bvh()->triangles[i],
            node->parent->vertexPositions());

        if (tri.centroid()[splitAxis] < splitPos) {
            leftAABB.extend(tri);
            numTrianglesLeft++;
        }
        else {
            rightAABB.extend(tri);
            numTrianglesRight++;
        }
    }
    // Cost is proportional to the area of the aabb times the number of triangles

    float leftCost = leftAABB.halfSurfaceArea() * numTrianglesLeft;
    float rightCost = rightAABB.halfSurfaceArea() * numTrianglesRight;

    return leftCost + rightCost;
}

void BVH::build(std::shared_ptr<BVH_Node> node, int depth) {
    node->parent = parent;
    node->recomputeAABB();
    node->aabb->depth = depth;
    this->m_depth = std::max(this->m_depth, depth);
    node->childIndex = 0;

    if (node->num_triangles <= 1)
        return;

    auto left = std::make_shared<BVH_Node>();
    auto right = std::make_shared<BVH_Node>();

    size_t split_axis = -1;
    float split_position = 0;

    if (BUILD_TYPE == 0 || node->num_triangles == 2) { // longest axis + median split 
        size_t axis = node->aabb->longestAxis();

        std::vector<size_t> sorted_indices;
        for (size_t i = node->first; i < node->first + node->num_triangles; i++)
            sorted_indices.push_back(i);

        // sort indices along axis
        std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t a, size_t b) {
            Triangle tri_a = getTriangle(
                parent->bvh()->triangles[a],
                parent->vertexPositions());

            Triangle tri_b = getTriangle(
                parent->bvh()->triangles[b],
                parent->vertexPositions());

            return tri_a.centroid()[axis] < tri_b.centroid()[axis];
            });

        // split indices

        size_t half = sorted_indices.size() / 2;

        Triangle tri_a = getTriangle(
            parent->bvh()->triangles[sorted_indices[half - 1]],
            parent->vertexPositions());

        Triangle tri_b = getTriangle(
            parent->bvh()->triangles[sorted_indices[half]],
            parent->vertexPositions());

        split_axis = axis;
        split_position = (tri_a.centroid()[axis] + tri_b.centroid()[axis]) * 0.5f;
    }
    else { // Surface area heuristic
        size_t bestAxis = -1;
        float bestCost = std::numeric_limits<float>::max();
        float bestSplitPos = 0.0f;

        // Choose best axis and split position 

        glm::vec3 minPos = node->aabb->begin_corner;
        glm::vec3 maxPos = node->aabb->end_corner;

        glm::vec3 step = (maxPos - minPos) / (float)(BVH::NUM_SPLIT_CANDIDATES + 1);

        for (size_t axis = 0; axis < 3; axis++) {
            for (size_t i = 1; i <= BVH::NUM_SPLIT_CANDIDATES; i++) {
                float splitPos = minPos[axis] + i * step[axis];
                float cost = evaluateSplit(node, axis, splitPos);

                if (cost < bestCost) {
                    bestCost = cost;
                    bestAxis = axis;
                    bestSplitPos = splitPos;
                }
            }
        }

        split_axis = bestAxis;
        split_position = bestSplitPos;
    }

    left->first = node->first;
    left->num_triangles = 0;
    right->first = node->first;
    right->num_triangles = 0;

    for (size_t i = node->first; i < node->first + node->num_triangles; i++) {
        Triangle tri = getTriangle(
            parent->bvh()->triangles[i],
            parent->vertexPositions());
        if (tri.centroid()[split_axis] < split_position) {
            left->num_triangles++;
            size_t swap = left->first + left->num_triangles - 1;
            std::swap(triangles[swap], triangles[i]);
            right->first++;
        }
        else {
            right->num_triangles++;
        }
    }

    if (left->num_triangles == 0 || right->num_triangles == 0) {
        node->childIndex = 0;
        return;
    }

    node->childIndex = _nodes.size();

    _nodes.push_back(left);
    _nodes.push_back(right);

    build(left, depth + 1);
    build(right, depth + 1);
}
#pragma once

#include <memory>
#include <vector>
#include <iostream>
#include <chrono>

#include "Mesh.h"
#include "primitives/AABB.h"
#include "Scene.h"

class Mesh;
class Scene;

struct BVH_Node {
    std::shared_ptr<Mesh> parent;
    std::shared_ptr<AABB> aabb;
    size_t childIndex = 0;
    size_t first;
    size_t num_triangles;

    void recomputeAABB();
};

class BVH {
    std::vector<std::shared_ptr<BVH_Node>> _nodes;
    std::shared_ptr<Mesh> parent;
    std::shared_ptr<BVH_Node> root;
    int m_depth = 0;

public:
    std::vector<glm::uvec3> triangles;
    BVH(std::shared_ptr<Mesh> meshPtr);

    void build() {
        std::chrono::high_resolution_clock clock;
        std::chrono::time_point<std::chrono::high_resolution_clock> before = clock.now();

        build(root);

        std::chrono::time_point<std::chrono::high_resolution_clock> after = clock.now();
        double elapsedTime = (double)std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
        std::cout << "BVH built in " << elapsedTime << "ms" << std::endl;
        std::cout << "Depth: " << m_depth << std::endl;
    }

    void build(std::shared_ptr<BVH_Node> node, int depth = 0);

    inline const std::shared_ptr<BVH_Node> getRoot() const { return root; }

    inline int depth() const { return m_depth; }

    inline const std::vector<std::shared_ptr<BVH_Node>>& nodes() const { return _nodes; }
public:
    static int BUILD_TYPE; // 0 = median split, 1 = surface area heuristic
    static int NUM_SPLIT_CANDIDATES;
};
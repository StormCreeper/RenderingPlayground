#include "Scene.h"
#include "Mesh.h"

void Scene::recomputeBVHs() {
    for (int i = 0; i < numOfModels(); i++) {
        model(i)->mesh()->recomputeBVH(model(i)->mesh());
        model(i)->mesh()->recomputeUVs(glm::vec2(1.0));
    }
}
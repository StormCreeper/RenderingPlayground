#include "core/Scene.h"
#include "core/Mesh.h"
#include "core/Model.h"

void Scene::recomputeBVHs() {
	for (int i = 0; i < numOfModels(); i++) {
		model(i)->mesh()->recomputeBVH(model(i)->mesh());
		model(i)->mesh()->recomputeUVs(glm::vec2(1.0));
	}
}
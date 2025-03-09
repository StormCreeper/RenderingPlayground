#pragma once

#include <memory>

#include "Image.h"

class Scene;
class Ray;
struct Hit;

class RayTracer {
public:

    RayTracer();
    virtual ~RayTracer();

    inline void setResolution(int width, int height) { m_imagePtr = std::make_shared<Image>(width, height); }
    inline std::shared_ptr<Image> image() { return m_imagePtr; }
    void init(const std::shared_ptr<Scene> scenePtr);
    Hit traceRay(const Ray& ray, const std::shared_ptr<Scene> scenePtr);
    Hit traceRayBVH(const Ray& ray, const std::shared_ptr<Scene> scenePtr);
    void render(const std::shared_ptr<Scene> scenePtr);

private:
    std::shared_ptr<Image> m_imagePtr;
};
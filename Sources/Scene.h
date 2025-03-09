#pragma once

#include <vector>
#include <memory>

#include "Light.h"
#include "Model.h"
#include "ColorCorrection.h"
#include "Texture.h"

class Camera;


class Scene {
public:
    inline Scene() : m_backgroundColor(0.f, 0.f, 0.f) {}
    virtual ~Scene() {}

    inline const glm::vec3& backgroundColor() const { return m_backgroundColor; }

    inline void setBackgroundColor(const glm::vec3& color) { m_backgroundColor = color; }

    // Camera

    inline void set(std::shared_ptr<Camera> camera) { m_camera = camera; }

    inline const std::shared_ptr<Camera> camera() const { return m_camera; }

    inline std::shared_ptr<Camera> camera() { return m_camera; }

    // Models

    inline void add(std::shared_ptr<Model> model) { m_models.push_back(model); }

    inline size_t numOfModels() const { return m_models.size(); }

    inline const std::shared_ptr<Model> model(size_t index) const { return m_models[index]; }

    inline std::shared_ptr<Model> model(size_t index) { return m_models[index]; }

    // Textures

    inline void add(std::shared_ptr<Texture> texture) { m_textures.push_back(texture); }

    inline size_t numOfTextures() const { return m_textures.size(); }

    inline const std::shared_ptr<Texture> texture(size_t index) const { return m_textures[index]; }

    inline std::shared_ptr<Texture> texture(size_t index) { return m_textures[index]; }

    // Lights

    inline void add(std::shared_ptr<AbstractLight> light) { m_lights.push_back(light); }

    inline size_t numOfLights() const { return m_lights.size(); }

    inline const std::shared_ptr<AbstractLight> light(size_t index) const { return m_lights[index]; }

    inline std::shared_ptr<AbstractLight> light(size_t index) { return m_lights[index]; }

    inline void removeLight(size_t index) { m_lights.erase(m_lights.begin() + index); }

    inline void replaceLight(size_t index, std::shared_ptr<AbstractLight> light) { m_lights[index] = light; }

    // Image parameters

    inline void set(const ImageParameters& imageParameters) { m_imageParameters = imageParameters; }

    inline const ImageParameters& imageParameters() const { return m_imageParameters; }

    inline ImageParameters& imageParameters() { return m_imageParameters; }

    inline void clear() {
        m_camera.reset();
        m_models.clear();
        m_lights.clear();
    }

    void recomputeBVHs();

private:
    glm::vec3 m_backgroundColor;
    std::shared_ptr<Camera> m_camera;
    std::vector<std::shared_ptr<Model> > m_models;
    std::vector<std::shared_ptr<AbstractLight> > m_lights;
    std::vector<std::shared_ptr<Texture>> m_textures;
    ImageParameters m_imageParameters;
};
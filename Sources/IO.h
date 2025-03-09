#pragma once

#include <string>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class Mesh;

class IO {
public:
    static void loadOFF(const std::string& filename, std::shared_ptr<Mesh> meshPtr);
    static std::string file2String(const std::string& filename);
    static void savePPM(const std::string& filename, int width, int height, const std::vector<glm::vec3>& pixels);
};
cmake_minimum_required(VERSION 3.9)

project(ToyRenderer LANGUAGES CXX)

find_package(OpenMP REQUIRED)

add_subdirectory(dep)

file(GLOB_RECURSE PROJECT_SRC
     "src/*.cpp"
)

add_executable (ToyRenderer ${PROJECT_SRC})

set_target_properties(ToyRenderer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# Copy the shader files in the binary location.

include_directories(${CMAKE_CURRENT_SOURCE_DIR} include/)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} dep/stb_image/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} dep/obj_loader/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} Sources/)

target_link_libraries(ToyRenderer LINK_PRIVATE glad)

target_link_libraries(ToyRenderer LINK_PRIVATE glfw)

target_link_libraries(ToyRenderer LINK_PRIVATE glm)

target_link_libraries(ToyRenderer LINK_PRIVATE IMGUI)

target_link_libraries(ToyRenderer PRIVATE OpenMP::OpenMP_CXX)
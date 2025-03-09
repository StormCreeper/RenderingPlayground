cmake_minimum_required(VERSION 3.9)

project(ToyRenderer LANGUAGES CXX)

find_package(OpenMP REQUIRED)

add_subdirectory(External)

file(GLOB_RECURSE PROJECT_SRC
     "Sources/*.h"
     "Sources/*.cpp"
)

add_executable (ToyRenderer ${PROJECT_SRC})

set_target_properties(ToyRenderer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

# Copy the shader files in the binary location.

include_directories(${CMAKE_CURRENT_SOURCE_DIR} External/stb_image/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} Sources/)

target_link_libraries(ToyRenderer LINK_PRIVATE glad)

target_link_libraries(ToyRenderer LINK_PRIVATE glfw)

target_link_libraries(ToyRenderer LINK_PRIVATE glm)

target_link_libraries(ToyRenderer LINK_PRIVATE IMGUI)

target_link_libraries(ToyRenderer PRIVATE OpenMP::OpenMP_CXX)
# minimum CMake version required to use vcpkg manifest mode
cmake_minimum_required(VERSION 3.21)
project(SilkEngine VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main main.cpp)

# link vcpkg manifest dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)

target_link_libraries(main PRIVATE glfw glm::glm Vulkan::Vulkan)

set(GLSLC "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/tools/shaderc/glslc.exe")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/vert.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/shaders/shader.vert -o ${CMAKE_BINARY_DIR}/shaders/vert.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/shader.vert
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/shaders/frag.spv
    COMMAND ${GLSLC} ${CMAKE_SOURCE_DIR}/shaders/shader.frag -o ${CMAKE_BINARY_DIR}/shaders/frag.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/shader.frag
)

add_custom_target(
    compile_shaders
    DEPENDS ${CMAKE_BINARY_DIR}/shaders/vert.spv ${CMAKE_BINARY_DIR}/shaders/frag.spv
)

add_dependencies(main compile_shaders)
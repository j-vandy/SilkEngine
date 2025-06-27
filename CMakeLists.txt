# minimum CMake version required to use vcpkg manifest mode
cmake_minimum_required(VERSION 3.21)
project(SilkEngine VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# create silk_engine library
add_library(silk_engine STATIC engine/Engine.cpp)

# include paths for engine headers
target_include_directories(silk_engine PUBLIC engine)

# link vcpkg manifest dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)

target_link_libraries(silk_engine PUBLIC glfw glm::glm Vulkan::Vulkan)

add_subdirectory(examples)
add_subdirectory(tests)
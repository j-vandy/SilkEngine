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

target_link_libraries(main PRIVATE glfw glm::glm Vulkan::Vulkan)
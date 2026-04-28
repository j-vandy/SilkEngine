# minimum CMake version required to use vcpkg manifest mode
cmake_minimum_required(VERSION 3.21)
project(SilkEngine VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# silk engine
add_library(silk STATIC
    src/Engine.cpp
    src/Transform.cpp
)

# include paths for engine headers
target_include_directories(silk PUBLIC include)

if (MSVC)
    target_compile_options(silk PRIVATE /W4 /WX)
else()
    target_compile_options(silk PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

# link vcpkg manifest dependencies
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

target_link_libraries(silk PUBLIC
    glfw
    glm::glm
    Vulkan::Vulkan
)

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(silk PUBLIC ${TINYGLTF_INCLUDE_DIRS})

# shader compilation helper function
function(silk_compile_shader TARGET SHADER)
    find_program(GLSLC glslc REQUIRED)

    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILE_NAME}.spv)

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${GLSLC}
                ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER}
                -o ${OUTPUT_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER}
    )

    target_sources(${TARGET} PRIVATE ${OUTPUT_FILE})
endfunction()

# silk consumers
add_subdirectory(examples)
add_subdirectory(tests)
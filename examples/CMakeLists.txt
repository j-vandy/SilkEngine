add_executable(bunny main.cpp)

target_link_libraries(bunny PRIVATE silk_engine)

# shader compilation
set(GLSLC "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/tools/shaderc/glslc.exe")
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/vert.spv
    COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/vert.spv
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/frag.spv
    COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/frag.spv
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag
)

add_custom_target(
    compile_shaders_bunny
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/vert.spv ${CMAKE_CURRENT_BINARY_DIR}/shaders/frag.spv
)

add_dependencies(bunny compile_shaders_bunny)
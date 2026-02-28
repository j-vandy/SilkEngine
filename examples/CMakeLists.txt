add_executable(bunny main.cpp)
target_link_libraries(bunny PRIVATE silk)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
silk_compile_shader(bunny shader.vert)
silk_compile_shader(bunny shader.frag)
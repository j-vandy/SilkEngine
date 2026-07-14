add_executable(paint main.cpp)
target_link_libraries(paint PRIVATE silk)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
silk_compile_shader(paint shader.vert)
silk_compile_shader(paint shader.frag)
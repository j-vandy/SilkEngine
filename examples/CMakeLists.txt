add_executable(model_viewer main.cpp)
target_link_libraries(model_viewer PRIVATE silk)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
silk_compile_shader(model_viewer shader.vert)
silk_compile_shader(model_viewer shader.frag)

# copy model assets
add_custom_command(TARGET model_viewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/model
        $<TARGET_FILE_DIR:model_viewer>/model
)
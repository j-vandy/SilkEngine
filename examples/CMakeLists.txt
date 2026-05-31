add_executable(ducky main.cpp)
target_link_libraries(ducky PRIVATE silk)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)
silk_compile_shader(ducky shader.vert)
silk_compile_shader(ducky shader.frag)

# copy model assets
add_custom_command(TARGET ducky POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/model
        $<TARGET_FILE_DIR:ducky>/model
)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC)

add_executable(ex_glfw_imgui_game ${SRC})
target_link_libraries(ex_glfw_imgui_game
    PRIVATE
    xc::ecs
    xc::ui
    xc::opengl_wrapper
    xc_plugin::glfw_support
    xc_plugin::opengl_support
    xc_plugin::xcal_opengl_render
    imgui_glfw_opengl3
    imgui-node-editor
    glbinding::glbinding
    glbinding::glbinding-aux
)
target_compile_options(ex_glfw_imgui_game PRIVATE "-Wmismatched-tags")
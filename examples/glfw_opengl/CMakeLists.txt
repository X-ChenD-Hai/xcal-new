

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC)

add_executable(ex_glfw_opengl ${SRC})
target_link_libraries(ex_glfw_opengl
    PRIVATE
    xcal2
    xc::ecs
    xc::ui
    xc::opengl_wrapper
    xc_plugin::glfw_support
    xc_plugin::opengl_support
    xc_plugin::xcal_opengl_render
    imgui_glfw_opengl3
)

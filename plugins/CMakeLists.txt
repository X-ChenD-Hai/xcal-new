xc_add_header_plugin(ui_protocol)
xc_add_static_plugin(glfw_support
    PUBLIC_LINK
    glfw xc::ecs xc_plugin::ui_protocol
)
xc_add_static_plugin(opengl_support
    PUBLIC_LINK
    xc::opengl_wrapper xc::ecs
)
add_subdirectory(xcal2)
xc_add_static_plugin(xcal_opengl_render
    PUBLIC_LINK
    xc::ecs xcal2
)
install(
    EXPORT xc_plugin
    NAMESPACE xc_plugin::
    FILE "xc_pluginTargets.cmake"
    DESTINATION ${XC_CMAKE_INSTALL_DIR}
)
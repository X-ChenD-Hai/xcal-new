xc_add_header_library(libxc common)
xc_add_header_library(log log)
xc_add_header_library(ecs2 ecs2)

xc_add_static_library(cmdline cmdline)
xc_add_static_library(ecs ecs PUBLIC_LINK libxc)
xc_add_static_library(event event PUBLIC_LINK libxc)
xc_add_static_library(opengl_wrapper opengl_wrapper
    PUBLIC_LINK ${XC_OPENGL_LOADER_LIBRARY}
)

if(XC_OpenGL_Loader STREQUAL "glbinding")
    target_compile_definitions(opengl_wrapper PUBLIC USE_GLBINDING)
elseif(XC_OpenGL_Loader STREQUAL "glad")
    target_compile_definitions(opengl_wrapper PUBLIC USE_GLAD)
endif()
xc_add_static_library(xcal xcal
    EXCLUDE_PATH "xcal/render/backend"
    PUBLIC_LINK
    xcmath::xcmath xc::libxc xc::ecs
)
xc_add_static_library(xcal_opengl_backend xcal/render/backend/opengl
    PUBLIC_LINK
    xcmath::xcmath xc::xcal opengl_wrapper
)
xc_add_static_library(ui ui
    PUBLIC_LINK libxc xc::ecs xc::event xcmath::xcmath
    PRIVATE_LINK glfw opengl_wrapper
    PRIVATE_INCLUDE ${imgui_INCDIR}
    EXTSRC 
    ${imgui_SOURCES} ${imgui_glfw_BACKEND_SOURCES} 
    ${imgui_opengl3_BACKEND_SOURCES}
)


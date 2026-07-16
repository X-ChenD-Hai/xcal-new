
add_executable(ex-glfw-wgpu main.cc)
target_link_libraries(ex-glfw-wgpu
    PRIVATE
    glfw
    glfw3webgpu
    webgpu-cpp
    webgpu
    xc::log
)
get_target_property(DIR ex-glfw-wgpu RUNTIME_OUTPUT_DIRECTORY)

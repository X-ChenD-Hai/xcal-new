

target("ex-glfw-wgpu")
    add_files("main.cc")
    add_packages("glfw","glfw3webgpu")
    add_deps("webgpu-cpp","xc-log")

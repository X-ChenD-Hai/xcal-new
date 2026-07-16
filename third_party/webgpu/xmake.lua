add_requires("wgpu-native","glfw3webgpu")

target("webgpu-cpp")
    set_kind("static")
    add_packages("wgpu-native",{public = true})
    add_files("webgpu.cc")
    add_includedirs("./",{public = true})

target("ex-game")
    set_kind("binary")
    add_files("*.cc")
    add_packages("glbinding","glfw")
    add_deps("imgui-glfw-backend","imgui-opengl3-backend")
    add_deps("ecs", "imgui-node-editor","imgui-wrapper")



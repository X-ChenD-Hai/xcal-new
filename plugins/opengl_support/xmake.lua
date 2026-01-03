target("xc-opengl-support")
    set_kind("static")
    add_files("*.cc")
    add_includedirs(".", { public = true })
    add_deps("ecs","opengl-wrapper")
    


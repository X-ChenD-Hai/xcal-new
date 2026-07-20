
target("xc-ecs2")
    set_kind("static")
    add_includedirs("../../",{public = true})
    add_files("*.cc")

target("xcal-transform")
    set_kind("static")
    add_files("*.cc")
    add_deps("ecs","xcmath","xcal-events",{public = true})
    
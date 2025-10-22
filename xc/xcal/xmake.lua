target("xcal")
    set_kind("static")
    add_files("./**.cc")
    remove_files("./render/backend/**.cc")
    add_deps("ecs","xcmath",{public = true})
    add_includedirs("..",{public = true})


includes("render/backend/*")
target("ui-protocol")
    set_kind("headeronly")
    add_deps("ecs")
    add_includedirs("..",{public = true})
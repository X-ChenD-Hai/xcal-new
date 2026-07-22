target("xc-xc_executor")
    set_kind("headeronly")
    add_includedirs("../../",{public=true})
    add_deps("xc-async","xc-ecs2")
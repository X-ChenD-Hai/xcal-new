target("libxc")
    set_kind("phony")
    add_includedirs("./common",{public = true})

includes("cmdline")
includes("event")
includes("ecs")
includes("xcal")
includes("ui")

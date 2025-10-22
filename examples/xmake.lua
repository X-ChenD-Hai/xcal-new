-- includes("objsys")


target("test")
    set_kind("binary")
    add_files("data.cc")
target("ref")
    set_kind("binary")
    add_files("ref.cc")
    add_deps("libxc")
    add_deps("event")

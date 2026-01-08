target("xc-async-scheduler")
    set_kind("static")
    add_files("*.cc")
    add_includedirs("../..",{public = true})
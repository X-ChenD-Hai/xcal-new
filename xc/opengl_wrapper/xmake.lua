target("opengl-wrapper")
    set_kind("static")
    add_files("*.cc")
    add_includedirs("../", { public = true })
    -- 根据平台选择OpenGL后端，与根目录xmake.lua的opengl规则保持一致
    if is_plat("linux") then
        add_defines("USE_GLAD", { public = true })
        add_packages("glad", { public = true })
    else
        add_defines("USE_GLBINDING", { public = true })
        add_packages("glbinding", { public = true })
    end


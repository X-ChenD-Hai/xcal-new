add_requireconfs("*.python", {
    override = true,
    -- system = true,
    on_fetch = function(package, opt)
        -- 直接返回系统 python 可执行文件路径
        local result = import("lib.detect.find_tool")("python3",{system = true})
        if result then
            print("fetch python")
            return {
                program = result.program,
                bindir = result.bindir,
            }
        end
    end
})
add_requires("glfw","glbinding","glm","glad")
add_requires("imgui",{configs={ glfw_opengl3=true }})
target("ui")
    set_kind("static")
    add_files("*.cc")
    add_rules("opengl")
    add_packages("glfw","imgui","imgui-wrapper")
    add_packages("glm",{public=true})
    add_deps("libxc","event","ecs","xcmath","xcal","xcal-opengl-render")
    add_includedirs("../",{public=true})
    add_includedirs("../common",{public=true})

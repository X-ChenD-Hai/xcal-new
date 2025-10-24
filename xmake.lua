add_rules("mode.debug", "mode.release","mode.minsizerel")
set_project("libxc")
set_languages("c++23")
add_cxxflags("-std=c++23")
if is_plat("linux") then
    add_requireconfs("*",{configs ={runtimes = "c++_static"} })
end
add_requires("gtest",{configs = {main = true }})

if is_mode("debug") then
    add_defines("_DEBUG")
end

rule("opengl")
    on_load(function (target) 
        if is_plat("linux") then 
            target:add("defines","USE_GLAD")
            target:add("packages", "glad")
        else 
            target:add("defines","USE_GLBINDING")
            target:add("packages", "glbinding")
        end
    end)
rule_end()

includes("third_party/xcmath")
includes("xc")
includes("bin")
includes("test")

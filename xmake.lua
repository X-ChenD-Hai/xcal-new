add_rules("mode.debug", "mode.release","mode.minsizerel")
set_project("libxc")
set_languages("c++23")
add_cxxflags("-std=c++23")
add_requires("gtest",{configs = {main = true }})
if is_mode("debug") then
    add_defines("_DEBUG")
end


includes("xc")
includes("bin")
includes("test")
includes("examples")
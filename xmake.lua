add_rules("mode.debug", "mode.release","mode.minsizerel")
set_project("libxc")
set_languages("c++23")
add_cxxflags("-std=c++23")
add_cxxflags("/std:c++latest")
if is_plat("linux") then
    add_requireconfs("*",{configs ={runtimes = "c++_static"} })
end
add_requires("gtest",{configs = {main = true }})
add_requires("benchmark")

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

includes("third_party")
includes("xc")
includes("plugins")
includes("bin")
includes("test")
includes("examples")

task("format")
    set_category("plugin")
    on_run(function ()
       import("lib.detect.find_tool")

        -- 查找 clang-format
        local tool = find_tool("clang-format")
        if not tool then
            raise("clang-format not found! please install it first.")
        end

        -- os.files 支持 ** 递归匹配，跨平台安全
        local dirs = {
            "xc","examples","plugins","test"
        }
        local patterns = {}
        for _,d in ipairs(dirs) do 
            table.insert(patterns, d.."/**.cpp")
            table.insert(patterns, d.."/**.cc")
            table.insert(patterns, d.."/**.cxx")
            table.insert(patterns, d.."/**.h")
            table.insert(patterns, d.."/**.hpp")
        end
        local files = {}
        for _, pat in ipairs(patterns) do
            for _, file in ipairs(os.files(pat)) do
                table.insert(files, file)
            end
        end
        local all = #files

        for id, file in ipairs(files) do
            printf("\27[K[%d/%d] ", id, all)
            printf("Formatting " .. file)
            printf("\r")
            os.execv(tool.program, {"-i", file})
        end

        print("\nFormat done.")
    end)
    set_menu {
        usage = "xmake format",
        description = "Run clang-format on project sources"
    }
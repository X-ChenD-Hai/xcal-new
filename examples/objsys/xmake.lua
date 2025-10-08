
rule("dynexport")
    on_load(function (target)
        print("loading ".. target:name() .. " kind " .. target:kind())
        local  api = target:name()
        if target:values("dll.api") then
            api = target:values("dll.api") 
        end
        local ex_api = ""
        local im_api = ""
        if target:get("kind") == "shared" then
            if is_plat("windows") then
                ex_api = "__declspec(dllexport)"
                im_api = "__declspec(dllimport)"
            elseif is_plat("linux") then
                ex_api = "__attribute__((visibility(\"default\")))"
                im_api = ""
            else
                ex_api = ""
                im_api = ""
            end
        end
        if ex_api == im_api then
            target:add("defines", string.upper(api) .. "_API=" .. ex_api,{ public=true })
        else 
            target:add("defines", string.upper(api) .. "_API=" .. ex_api,{ public=false })
            target:add("defines", string.upper(api) .. "_API=" .. im_api,{interface=true})
        end
     end)


target("objsys")
    set_kind("shared")
    add_files("**.cc")
    remove_files("test/**.cc")
    add_rules("dynexport")
    add_values("dll.api","XC_OBJSYS")
    add_deps("libxc",{public=true})
    add_includedirs(os.curdir(),{public=true})

for _,file in ipairs(os.files("test/**.cc")) do
    target("objsys_" .. path.basename(file))
        set_kind("binary")
        add_files(file)
        add_deps("objsys",{public=true})
end 
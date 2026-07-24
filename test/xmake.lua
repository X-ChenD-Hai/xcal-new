set_languages("c++23")
for _,filename in ipairs(os.files("*.cc")) do
    if filename ~= "main.cc" then
    target("test-" .. filename:gsub("%.cc", ""))
        set_kind("binary")
        add_files(filename)
        add_files("main.cc")
        add_deps("libxc")
        add_deps("event")
        add_includedirs(".")
        add_includedirs("../")
        add_packages("gtest")
        add_packages("glfw")
        add_deps("xc-glfw-support")
        
        add_tests("test_" .. filename:gsub("%.cc", ""))
    end
end


includes(os.dirs("./*"))
set_languages("c++23")
for _, filename in ipairs(os.files("*.cc")) do
    if filename ~= "main.cc" then
        target("bench-" .. filename:gsub("%.cc", ""))
            set_kind("binary")
            add_files(filename)
            add_deps("libxc")
            add_includedirs(".")
            add_includedirs("../")
            add_includedirs("../../")
            add_packages("benchmark")
            add_tests("bench_" .. filename:gsub("%.cc", ""))
    end
end

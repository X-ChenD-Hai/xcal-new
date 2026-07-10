add_executable(ecs2_dev main.cc)

target_link_libraries(ecs2_dev PRIVATE xc::ecs2 xc::libxc)
# set(OPTIONS "-fsanitize=address,undefined")
# target_compile_options(ecs2_dev PRIVATE ${OPTIONS} "-fno-omit-frame-pointer")
# target_link_options(ecs2_dev PRIVATE "-fsanitize=address,undefined")

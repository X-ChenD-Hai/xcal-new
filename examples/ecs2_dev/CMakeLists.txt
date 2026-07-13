
macro(add_ecs2_example name src)
    add_executable(${name} ${src})

    target_link_libraries(${name} PRIVATE xc::ecs2 xc::libxc)
endmacro()

add_ecs2_example(ecs2-dev main.cc)
add_ecs2_example(ecs2-timeout timeout.cc)

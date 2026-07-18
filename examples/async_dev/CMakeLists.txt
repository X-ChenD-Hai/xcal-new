
macro(add_async_example name src)
    add_executable(${name} ${src})

    target_link_libraries(${name} PRIVATE xc::async xc::libxc)
endmacro()

add_async_example(async-dev main.cc)
add_async_example(async-timeout timeout.cc)

add_executable(ecs2_dev main.cc)

target_link_libraries(ecs2_dev PRIVATE xc::ecs2 xc::libxc)

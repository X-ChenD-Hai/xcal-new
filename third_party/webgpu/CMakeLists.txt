

add_library(webgpu-cpp STATIC webgpu.cc)

target_link_libraries(webgpu-cpp PUBLIC webgpu)
target_include_directories(webgpu-cpp PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

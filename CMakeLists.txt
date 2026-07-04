cmake_minimum_required(VERSION 3.10..4.1)

option(XC_BUILD_TESTS "Build tests" OFF)

if(XC_BUILD_TESTS)
    enable_testing()
endif()

project(xc LANGUAGES CXX)
set(XC_OpenGL_Loader "glad" CACHE STRING "OpenGL backend")
set_property(CACHE XC_OpenGL_Loader PROPERTY STRINGS "glad" "glbinding")
option(XC_BUILD_EXAMPLES "Build examples" OFF)
option(XC_BUILD_BINARIES "Build binaries" OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
set(XC_CMAKE_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/xc)

include(cmake/formate.cmake)
include(cmake/xctarget.cmake)
include(cmake/install_deps.cmake)

add_subdirectory(third_party)
add_subdirectory(xc)
add_subdirectory(plugins)

if(XC_BUILD_BINARIES)
    add_subdirectory(bin)
endif()

if(XC_BUILD_TESTS)
    include(cmake/install_test_deps.cmake)
    add_subdirectory(test)
endif()

if(XC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

install(
    EXPORT xc
    NAMESPACE xc::
    FILE "xcTargets.cmake"
    DESTINATION ${XC_CMAKE_INSTALL_DIR}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/xcConfig.cmake.in
    xcConfig.cmake
    INSTALL_DESTINATION ${XC_CMAKE_INSTALL_DIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/xcConfig.cmake
    DESTINATION ${XC_CMAKE_INSTALL_DIR}
)
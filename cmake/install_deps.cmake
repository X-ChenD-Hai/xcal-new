include(FetchContent)
FetchContent_Declare(
    webgpu
    URL https://github.com/gfx-rs/wgpu-native/releases/download/v27.0.4.1/wgpu-windows-x86_64-msvc-debug.zip
)
FetchContent_MakeAvailable(webgpu)

find_library(
    wgpu_native
    NAMES wgpu_native
    PATHS ${webgpu_SOURCE_DIR}/lib
    NO_DEFAULT_PATH
)

if(NOT wgpu_native)
    message(FATAL_ERROR "webgpu not found")
    else()
    message(STATUS "find wgpu_native: ${wgpu_native}")
endif()

add_library(webgpu UNKNOWN IMPORTED)
set_target_properties(webgpu PROPERTIES
    IMPORTED_LOCATION ${wgpu_native}
    IMPORTED_INCLUDE_DIRECTORIES ${webgpu_SOURCE_DIR}/include
    IMPORTED_LIBRARY_DIRECTORIES ${webgpu_SOURCE_DIR}/lib
)
target_include_directories(webgpu INTERFACE ${webgpu_SOURCE_DIR}/include)
FetchContent_Declare(
    tiny_process_library
    GIT_REPOSITORY https://gitlab.com/eidheim/tiny-process-library.git
    GIT_TAG v2.0.4
    SYSTEM
)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(tiny_process_library)
unset(CMAKE_POLICY_VERSION_MINIMUM)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    SYSTEM
)
FetchContent_MakeAvailable(glfw)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.89.9-docking
    SYSTEM
)
FetchContent_MakeAvailable(imgui)

set(imgui_INCDIR ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
set(imgui_glfw_BACKEND_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp)
set(imgui_opengl3_BACKEND_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
file(GLOB imgui_SOURCES ${imgui_SOURCE_DIR}/*.cpp)

add_library(imgui_glfw_opengl3 STATIC)
target_sources(imgui_glfw_opengl3 PRIVATE ${imgui_SOURCES} ${imgui_glfw_BACKEND_SOURCES} ${imgui_opengl3_BACKEND_SOURCES})
target_include_directories(imgui_glfw_opengl3 PUBLIC ${imgui_INCDIR})
target_link_libraries(
    imgui_glfw_opengl3
    PRIVATE
    glfw
)
FetchContent_MakeAvailable(glfw)
FetchContent_Declare(
    imgui_node_editor
    GIT_REPOSITORY https://github.com/thedmd/imgui-node-editor.git
    GIT_TAG v0.9.3
    SYSTEM
)
FetchContent_MakeAvailable(imgui_node_editor)

add_library(imgui-node-editor STATIC)
target_sources(imgui-node-editor PRIVATE
    ${imgui_node_editor_SOURCE_DIR}/imgui_node_editor.cpp
    ${imgui_node_editor_SOURCE_DIR}/imgui_node_editor_api.cpp
    ${imgui_node_editor_SOURCE_DIR}/imgui_canvas.cpp
    ${imgui_node_editor_SOURCE_DIR}/crude_json.cpp
)
target_include_directories(imgui-node-editor PUBLIC
    ${imgui_node_editor_SOURCE_DIR}
    ${imgui_INCDIR}
)
FetchContent_Declare(
    glbinding
    GIT_REPOSITORY https://github.com/cginternals/glbinding.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(glbinding)

if(XC_OpenGL_Loader STREQUAL "glad")
    FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad
        GIT_TAG v2.0.6
    )
    FetchContent_Populate(glad) # 只下载，不构建

    # 2. 包含 Glad 的 CMake 模块
    include(${glad_SOURCE_DIR}/cmake/CMakeLists.txt)

    # 3. 手动调用生成函数
    # 生成一个名为 "glad" 的库，使用 OpenGL 4.6 Core Profile
    glad_add_library(xc_glad
        MERGE
        LOADER
        API gl:compatibility=4.6

        # API gl:core=4.6
        REPRODUCIBLE
        EXTENSIONS ""
    )
    get_target_property(GLAD_INC_DIRS xc_glad INTERFACE_INCLUDE_DIRECTORIES)

    # 如果路径不为空，将其标记为构建接口（BUILD_INTERFACE）
    if(GLAD_INC_DIRS)
        # 移除旧的包含路径
        set_target_properties(xc_glad PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "")

        message(STATUS "GLAD_INC_DIRS: ${GLAD_INC_DIRS}")

        # 重新添加，并明确指定为构建时路径
        target_include_directories(xc_glad INTERFACE
            $<BUILD_INTERFACE:${GLAD_INC_DIRS}>

            # 如果需要安装，可以添加安装路径
            # $<INSTALL_INTERFACE:include>
        )
    endif()

    set(XC_OPENGL_LOADER_LIBRARY xc_glad)
    install(
        TARGETS xc_glad
        EXPORT xc
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

elseif(XC_OpenGL_Loader STREQUAL "glbinding")
    set(XC_OPENGL_LOADER_LIBRARY
        glbinding::glbinding
        glbinding::glbinding-aux
    )
else()
    message(FATAL_ERROR "Unknown OpenGL loader: ${XC_OpenGL_Loader}")
endif()

FetchContent_Declare(
    GLFW3WGPU
    GIT_REPOSITORY https://github.com/eliemichel/glfw3webgpu.git
    GIT_TAG  v1.3.0-alpha
    SYSTEM
)
FetchContent_MakeAvailable(GLFW3WGPU)

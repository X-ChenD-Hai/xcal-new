function(xc_link_include TARGET)
    cmake_parse_arguments(
        ARG
        ""
        ""
        "PUBLIC_LINK;PRIVATE_LINK;INTERFACE_LINK;PUBLIC_INCLUDE;PRIVATE_INCLUDE;INTERFACE_INCLUDE"
        "${ARGN}"
    )

    if(DEFINED ARG_PUBLIC_LINK)
        target_link_libraries(${TARGET} PUBLIC ${ARG_PUBLIC_LINK})
    endif()

    if(DEFINED ARG_PRIVATE_LINK)
        target_link_libraries(${TARGET} PRIVATE ${ARG_PRIVATE_LINK})
    endif()

    if(DEFINED ARG_INTERFACE_LINK)
        target_link_libraries(${TARGET} INTERFACE ${ARG_INTERFACE_LINK})
    endif()

    if(DEFINED ARG_PUBLIC_INCLUDE)
        target_include_directories(${TARGET} PUBLIC ${ARG_PUBLIC_INCLUDE})
    endif()

    if(DEFINED ARG_PRIVATE_INCLUDE)
        target_include_directories(${TARGET} PRIVATE ${ARG_PRIVATE_INCLUDE})
    endif()

    if(DEFINED ARG_INTERFACE_INCLUDE)
        target_include_directories(${TARGET} INTERFACE ${ARG_INTERFACE_INCLUDE})
    endif()
endfunction(xc_link_include)

function(xc_get_namespace OUT_VAR)
    cmake_parse_arguments(
        ARG
        ""
        "NAMESPACE"
        ""
        "${ARGN}"
    )
    set(${OUT_VAR} xc PARENT_SCOPE)

    if(DEFINED ARG_NAMESPACE)
        set(${OUT_VAR} ${ARG_NAMESPACE} PARENT_SCOPE)
    endif()
endfunction(xc_get_namespace)

function(xc_install TARGET DIR NAMESPACE)
    install(
        TARGETS ${TARGET}
        EXPORT ${NAMESPACE}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    install(
        DIRECTORY ${DIR}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${NAMESPACE}/
        FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h"
    )
endfunction(xc_install)

function(xc_add_alias TARGET NAMESPACE)
    set(ALIAS_NAME "${NAMESPACE}::${TARGET}")

    if("${TARGET}" MATCHES "^${NAMESPACE}_.*")
        string(REPLACE "${NAMESPACE}_" "${NAMESPACE}::" ALIAS_NAME ${TARGET})
    endif()

    add_library("${ALIAS_NAME}" ALIAS ${TARGET})
    message(STATUS "add alias ${ALIAS_NAME} to ${TARGET}")
endfunction(xc_add_alias)

function(xc_set_output TARGET NAMESPACE)
    if("${TARGET}" MATCHES "^${NAMESPACE}_.*")
        set_target_properties(
            ${TARGET}
            PROPERTIES OUTPUT_NAME "${TARGET}"
        )
    else()
        set_target_properties(
            ${TARGET}
            PROPERTIES OUTPUT_NAME "${NAMESPACE}_${TARGET}"
        )
    endif()
endfunction(xc_set_output TARGET NAMESPACE)

function(xc_target_source TARGET DIR)
    cmake_parse_arguments(
        ARG
        "NORECURSIVE"
        ""
        "EXCLUDE_PATH;EXTSRC"
        ${ARGN}
    )

    if(ARG_NORECURSIVE)
        file(GLOB SRC_FILES ${DIR}/*.cc)
    else()
        file(GLOB_RECURSE SRC_FILES ${DIR}/*.cc)
    endif()

    if(DEFINED ARG_EXCLUDE_PATH)
        foreach(EXCLUDE ${ARG_EXCLUDE_PATH})
            list(FILTER SRC_FILES EXCLUDE REGEX ".*${EXCLUDE}.*")
        endforeach()
    endif()
    if(DEFINED ARG_EXTSRC)
        list(APPEND SRC_FILES ${ARG_EXTSRC})
    endif()

    target_sources(${TARGET} PRIVATE ${SRC_FILES})
endfunction(xc_target_source)

function(xc_add_header_library TARGET INCDIR)
    add_library(${TARGET} INTERFACE)
    xc_get_namespace(NAMESPACE ${ARGN})
    xc_add_alias(${TARGET} ${NAMESPACE})
    xc_add_alias(${TARGET} ${NAMESPACE})
    target_include_directories(${TARGET} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}>
    )
    xc_link_include(${TARGET} ${ARGN})
    xc_install(${TARGET} ${INCDIR} ${NAMESPACE})
endfunction(xc_add_header_library)

function(xc_add_static_library TARGET SRCDIR)
    add_library(${TARGET} STATIC)
    xc_get_namespace(NAMESPACE ${ARGN})
    xc_add_alias(${TARGET} ${NAMESPACE})
    xc_set_output(${TARGET} ${NAMESPACE})
    target_include_directories(${TARGET} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}>
    )
    xc_target_source(${TARGET} ${SRCDIR} ${ARGN})
    xc_link_include(${TARGET} ${ARGN})
    xc_install(${TARGET} ${SRCDIR} ${NAMESPACE})
endfunction(xc_add_static_library)

function(xc_add_test NAME SOURCES)
    set(TEST_NAME ${NAME}_test)
    add_executable(${TEST_NAME} ${SOURCES})
    target_link_libraries(${TEST_NAME} PRIVATE ${TEST_SUPPORT_LIBRARIES})
    add_test(NAME ${NAME} COMMAND ${TEST_NAME})
    xc_link_include(${TEST_NAME} ${ARGN})
endfunction(xc_add_test SOURCES)

function(xc_add_static_plugin DIR)
    xc_add_static_library(${DIR} ${DIR}
        NAMESPACE xc_plugin
        INTERFACE_INCLUDE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/xc_plugin>
        ${ARGN}
    )
endfunction(xc_add_static_plugin)

function(xc_add_header_plugin DIR)
    xc_add_header_library(${DIR} ${DIR}
        NAMESPACE xc_plugin
        INTERFACE_INCLUDE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/xc_plugin>
        ${ARGN}
    )
endfunction(xc_add_header_plugin)

function(xc_add_bin NAME FILES)
    add_executable("bin_${NAME}" ${FILES})
    target_link_libraries("bin_${NAME}" PRIVATE ${ARGN})
    set_target_properties("bin_${NAME}" PROPERTIES OUTPUT_NAME ${NAME})
    message(STATUS "add bin ${NAME}")
endfunction(xc_add_bin)
#[[

[cmake] -- SRC C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/animation/animation.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/animation/translate.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/camera/camera.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/render/backend/opengl/event.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/render/backend/opengl/mesh.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/render/backend/opengl/mesh/axis.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/render/backend/opengl/mesh/trangle.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/render/backend/opengl/render.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/render/backend/opengl/shader.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/render/backend/opengl/uniform.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/transform/transform.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/transform/transformable.cc;C:/Users/xcdh/workspace/xcrtp/xcal-new/xc/xcal/xcal.cc
]]
macro(add_xcal2_static_linrary NAME)
    xc_add_static_library("xcal2_${NAME}"
        ${NAME}
        NAMESPACE xcal2
        PUBLIC_LINK
        ${ARGN}
    )
endmacro(add_xcal2_static_linrary)

macro(add_xcal2_header_linrary NAME)
    xc_add_header_library("xcal2_${NAME}"
        ${NAME}
        NAMESPACE xcal2
        PUBLIC_LINK
        ${ARGN}
    )
endmacro(add_xcal2_header_linrary)

add_xcal2_static_linrary(events xc::ecs)
add_xcal2_header_linrary(ooject)
add_xcal2_static_linrary(transform xc::ecs xcmath::xcmath)
xc_add_static_library(xcal2_camera
    camera
    NORECURSIVE
    NAMESPACE xcal2
    PUBLIC_LINK
    xc::ecs xcal2::events xcmath::xcmath
)
xc_add_static_library(xcal2_camera_uicontroler
    camera/ui_controler
    NORECURSIVE
    NAMESPACE xcal2
    PUBLIC_LINK
    xc::ecs xcal2::camera xc_plugin::ui_protocol
)
add_library(xcal2 INTERFACE)
target_link_libraries(xcal2 INTERFACE
    xcal2::events
    xcal2::ooject
    xcal2::transform
    xcal2::camera
    xcal2::camera_uicontroler
)

install(
    TARGETS xcal2
    EXPORT xcal2
)
install(
    EXPORT xcal2
    NAMESPACE xcal2::
    FILE "xcal2Targets.cmake"
    DESTINATION ${XC_CMAKE_INSTALL_DIR}
)
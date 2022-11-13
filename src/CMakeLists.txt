add_executable(hidergodm hidergod.c)
target_sources(hidergodm PRIVATE hedev.c)
target_sources(hidergodm PRIVATE heapi.c)
target_sources(hidergodm PRIVATE zmk_control.c)


# NOTE: no `find_package` is required, since HIDAPI targets are already a part of the project tree
if (UNIX)
    target_link_libraries(hidergodm PRIVATE hidapi::hidraw)
endif (UNIX)
if (WIN32)
    target_link_libraries(hidergodm PRIVATE hidapi::hidapi)
endif (WIN32)
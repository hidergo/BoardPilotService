add_executable(hidergodm hidergod.c)
target_sources(hidergodm PRIVATE hedev.c)
target_sources(hidergodm PRIVATE heapi.c)
target_sources(hidergodm PRIVATE hedev_msg.c)


# NOTE: no `find_package` is required, since HIDAPI targets are already a part of the project tree
#target_link_libraries(hidergodm PRIVATE hidapi::hidapi)
target_link_libraries(hidergodm PRIVATE hidapi::hidraw)
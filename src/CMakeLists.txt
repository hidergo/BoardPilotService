add_executable(hidergodm hidergod.c)
target_sources(hidergodm PRIVATE hedev.c)
target_sources(hidergodm PRIVATE heapi.c)
target_sources(hidergodm PRIVATE zmk_control.c)
target_sources(hidergodm PRIVATE heapi_msg.c)


if(WIN32)
  target_compile_options(hidergodm PRIVATE /W4 /WX)
else()
  target_compile_options(hidergodm PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# NOTE: no `find_package` is required, since HIDAPI targets are already a part of the project tree
if (UNIX)
    target_link_libraries(hidergodm PRIVATE hidapi::hidraw)
endif (UNIX)
if (WIN32)
    target_link_libraries(hidergodm PRIVATE hidapi::hidapi)
endif (WIN32)
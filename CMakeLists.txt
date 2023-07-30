cmake_minimum_required(VERSION 3.4.3 FATAL_ERROR)
cmake_policy(SET CMP0076 NEW)
project(boardpilotservice)

set(HIDAPI_WITH_LIBUSB FALSE)
set(BUILD_SHARED_LIBS FALSE)
add_subdirectory(hidapi)

add_executable(bpservice src/bpservice.c)

target_include_directories(bpservice PRIVATE include)
target_include_directories(bpservice PRIVATE include/cJSON)

target_sources(bpservice PRIVATE src/bpdev.c)
target_sources(bpservice PRIVATE src/bpapi.c)
target_sources(bpservice PRIVATE src/zmk_control.c)
target_sources(bpservice PRIVATE src/bpapi_msg.c)
target_sources(bpservice PRIVATE src/cJSON/cJSON.c)


# NOTE: no `find_package` is required, since HIDAPI targets are already a part of the project tree
if (UNIX)
  target_compile_options(bpservice PRIVATE -Wall -Wextra -Wpedantic -Wno-format-truncation)
  target_link_libraries(bpservice PRIVATE hidapi::hidraw)
endif (UNIX)
if (WIN32)
  target_link_libraries(bpservice PRIVATE hidapi::hidapi)
  target_compile_options(bpservice PRIVATE /W4)
endif (WIN32)
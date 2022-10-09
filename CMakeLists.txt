cmake_minimum_required(VERSION 3.4.3 FATAL_ERROR)
cmake_policy(SET CMP0076 NEW)
project(hidergodm)

set(HIDAPI_WITH_LIBUSB FALSE)
set(BUILD_SHARED_LIBS FALSE)
add_subdirectory(hidapi)
add_subdirectory(src)
add_subdirectory(src/cJSON)
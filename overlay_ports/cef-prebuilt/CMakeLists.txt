cmake_minimum_required(VERSION 3.18.2)
project(cef-prebuilt)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CEF_ROOT "${CMAKE_SOURCE_DIR}")
find_package(CEF REQUIRED)
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

install(TARGETS libcef_dll_wrapper RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib PUBLIC_HEADER DESTINATION include)
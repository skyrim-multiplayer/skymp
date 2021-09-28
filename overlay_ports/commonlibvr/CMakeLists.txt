cmake_minimum_required(VERSION 3.18.2)
project(CommonLibVR)

find_path(third_party xbyak/xbyak.h)

file(GLOB_RECURSE lib_src "src/*" "include/*")
add_library(CommonLibVR STATIC ${lib_src})
target_include_directories(CommonLibVR PUBLIC "include" "${third_party}")
target_compile_options(CommonLibVR PUBLIC "/FI\"ForceInclude.h\"" "/FI\"SKSE/Logger.h\"")
target_link_libraries(CommonLibVR PUBLIC Version)
set_target_properties(CommonLibVR PROPERTIES CXX_STANDARD 17)

install(TARGETS CommonLibVR)

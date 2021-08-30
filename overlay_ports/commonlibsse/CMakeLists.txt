cmake_minimum_required(VERSION 3.18.2)
project(CommonLibSSE)

find_path(third_party xbyak/xbyak.h)

file(GLOB_RECURSE lib_src "src/*" "include/*")
add_library(CommonLibSSE STATIC ${lib_src})
target_include_directories(CommonLibSSE PUBLIC "include" "${third_party}")
target_compile_options(CommonLibSSE PUBLIC "/FI\"ForceInclude.h\"" "/FI\"SKSE/Logger.h\"")
target_link_libraries(CommonLibSSE PUBLIC Version)
set_target_properties(CommonLibSSE PROPERTIES CXX_STANDARD 17)

install(TARGETS CommonLibSSE)

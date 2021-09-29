cmake_minimum_required(VERSION 3.18.2)
project(sksevr)

file(GLOB_RECURSE common_src "sksevr_2_00_12/src/common/*")
add_library(commonvr STATIC ${common_src})
target_compile_options(commonvr PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(commonvr PRIVATE "sksevr_2_00_12/src/common" "sksevr_2_00_12/src")

file(GLOB_RECURSE sksevr_common_src "sksevr_2_00_12/src/sksevr/skse64_common/*")
add_library(sksevr_common STATIC ${sksevr_common_src})
target_compile_options(sksevr_common PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(sksevr_common PRIVATE "sksevr_2_00_12/src/common" "sksevr_2_00_12/src/sksevr/skse64_common" "sksevr_2_00_12/src" "sksevr_2_00_12/src/sksevr")

file(GLOB_RECURSE sksevr_src "sksevr_2_00_12/src/sksevr/*")
add_library(sksevr STATIC ${sksevr_src})
target_compile_options(sksevr PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(sksevr PUBLIC "sksevr_2_00_12/src/common" "sksevr_2_00_12/src/sksevr/skse64" "sksevr_2_00_12/src" "sksevr_2_00_12/src/sksevr")
target_compile_definitions(sksevr PRIVATE RUNTIME=1 RUNTIME_VERSION=0x01050610)
target_link_libraries(sksevr PUBLIC commonvr sksevr_common)

install(TARGETS commonvr sksevr_common sksevr)

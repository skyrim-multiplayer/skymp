cmake_minimum_required(VERSION 3.18.2)
project(skse64)

file(GLOB_RECURSE common_src "skse64_2_02_03/src/common/*")
add_library(common STATIC ${common_src})
target_compile_options(common PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(common PRIVATE "skse64_2_02_03/src/common" "skse64_2_02_03/src")

file(GLOB_RECURSE skse64_common_src "skse64_2_02_03/src/skse64/skse64_common/*")
add_library(skse64_common STATIC ${skse64_common_src})
target_compile_options(skse64_common PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(skse64_common PRIVATE "skse64_2_02_03/src/common" "skse64_2_02_03/src/skse64/skse64_common" "skse64_2_02_03/src" "skse64_2_02_03/src/skse64")

file(GLOB_RECURSE skse_src "skse64_2_02_03/src/skse64/*")
add_library(skse64 STATIC ${skse_src})
target_compile_options(skse64 PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(skse64 PUBLIC "skse64_2_02_03/src/common" "skse64_2_02_03/src/skse64/skse64" "skse64_2_02_03/src" "skse64_2_02_03/src/skse64")
target_compile_definitions(skse64 PRIVATE RUNTIME=1 RUNTIME_VERSION=0x01050610)
target_link_libraries(skse64 PUBLIC common skse64_common)

install(TARGETS common skse64_common skse64)

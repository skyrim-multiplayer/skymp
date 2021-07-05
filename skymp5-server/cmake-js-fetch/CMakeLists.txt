cmake_minimum_required(VERSION 3.19)
project(cmake-js-fetch)

set(f ${CMAKE_CURRENT_BINARY_DIR}/node.cmake)

file(WRITE ${f} "add_library(node INTERFACE)\n")

foreach(p ${CMAKE_JS_INC})
  string(REPLACE "\\" "\\\\" p ${p})
  file(APPEND ${f} "target_include_directories(node INTERFACE \"${p}\")\n")
endforeach()

foreach(p ${CMAKE_JS_SRC})
  string(REPLACE "\\" "\\\\" p ${p})
  file(APPEND ${f} "target_sources(node INTERFACE \"${p}\")\n")
endforeach()

foreach(p ${CMAKE_JS_LIB})
  string(REPLACE "\\" "\\\\" p ${p})
  file(APPEND ${f} "target_link_libraries(node INTERFACE \"${p}\")\n")
endforeach()
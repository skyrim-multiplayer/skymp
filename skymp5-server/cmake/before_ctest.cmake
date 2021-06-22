file(GLOB_RECURSE tmp ${CMAKE_CURRENT_BINARY_DIR}/*.tmp)
if (NOT "${tmp}" STREQUAL "")
  file(REMOVE ${tmp})
endif()

string(RANDOM LENGTH 5 tag)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cppcov_tag.tmp ${tag})

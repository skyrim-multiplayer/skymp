### Project ###
project(sp_interface_ae VERSION 0.1.0 LANGUAGES CXX)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h	@ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc	@ONLY)

### Dependencies ###
find_package(CommonLibSSE REQUIRED)
find_package(Boost MODULE REQUIRED)
find_package(spdlog REQUIRED CONFIG)

### Sources ###
include(cmake/headerlist.cmake)
include(cmake/sourcelist.cmake)

source_group(TREE	${CMAKE_CURRENT_SOURCE_DIR}	FILES	${headers}	${sources})
source_group(TREE	${CMAKE_CURRENT_BINARY_DIR}	FILES	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h)

### Library ###
add_library(${PROJECT_NAME}	SHARED ${headers}	${sources}	${CMAKE_CURRENT_BINARY_DIR}/include/Version.h	${CMAKE_CURRENT_BINARY_DIR}/version.rc)
target_include_directories(${PROJECT_NAME}	PRIVATE	${CMAKE_CURRENT_BINARY_DIR}/include	${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME}	PRIVATE	Boost::headers CommonLibSSE::CommonLibSSE	spdlog::spdlog)
target_precompile_headers(${PROJECT_NAME}	PRIVATE	src/PCH.h)

include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)
apply_default_settings(TARGETS ${PROJECT_NAME})


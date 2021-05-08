vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO skyrim-multiplayer/JsEngine
	REF 86fbd1854234c021c429d40dca561f7130434482
	SHA512 76132d2910d510438cce85e0a557d38829289e4174fde0e757d4dc0042e3ee6c2bee561e90abe7d3a3bf4513d3ec95143efddb308c177b678ae11bae28e8f8c5
	HEAD_REF master
)

file(GLOB_RECURSE sources ${SOURCE_PATH}/*.h)
foreach(file ${sources})
    file(
        COPY ${file}
        DESTINATION ${CURRENT_PACKAGES_DIR}/include
    )
endforeach()

file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
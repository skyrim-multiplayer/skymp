vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO skyrim-multiplayer/JsEngine
	REF 3ebf615e7d0df9042cac9415f0a6a022f8eb5bee
	SHA512 1acd0e73a4951af0045f8cc34a2e067471cfdf89d0ef886291666f03fab14bdb4a545aef7606bd9ea1175b9d4d0f13468d95c98fbf8a25ebe8828edc2344a249
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
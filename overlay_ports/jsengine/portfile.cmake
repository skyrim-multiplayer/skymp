vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO skyrim-multiplayer/JsEngine
	REF 67913e2fd0ed4d5665f9260b140f0f1e1166b4cd
	SHA512 65c4982b7636a1b3d837ba89f6fdca64d1eb915c6fe64542d20897c1084453730919b4c2364c2520b0c09605fdb79ad31ffe547c012edac7377bec06c0b94b50
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
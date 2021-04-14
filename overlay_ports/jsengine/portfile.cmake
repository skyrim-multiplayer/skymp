vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO skyrim-multiplayer/JsEngine
	REF 523151bb3cc67254fa29eda695c82f302d2a8189
	SHA512 d44bebcece66e060282000e1c93e86646287f7ab5b955758236009b1775c7c009038f04fdec38555eabb0cf5836fada6b681b3c88fa524ef309d2391d2b78ad4
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
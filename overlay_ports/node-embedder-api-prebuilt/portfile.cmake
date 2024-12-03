set(NODE_EMBEDDER_API_URL "https://node-embedder-api-prebuilt-vcpkg-port-v2.b-cdn.net/archive.zip")
set(NODE_EMBEDDER_API_FILENAME "node-embedder-api-prebuilt.zip")
set(NODE_EMBEDDER_API_SHA512 31aa1c29b0c2cf4da4f6b2fa98471d0c692928d2efbe5d547e1910124db783483e8a899333fb116030b0d93b1896cba13dcd6ddb11b96e8308b02b90a36612f0)

vcpkg_download_distfile(ARCHIVE
    URLS ${NODE_EMBEDDER_API_URL}
    FILENAME ${NODE_EMBEDDER_API_FILENAME}
    SHA512 ${NODE_EMBEDDER_API_SHA512}
)

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${ARCHIVE}
    NO_REMOVE_ONE_LEVEL
)

file(GLOB_RECURSE files ${SOURCE_PATH}/*)

foreach(file ${files})
  file(RELATIVE_PATH relative_path ${SOURCE_PATH} ${file})

  set(destination_path ${CURRENT_PACKAGES_DIR}/${relative_path})

  get_filename_component(destination_dir ${destination_path} DIRECTORY)
  file(MAKE_DIRECTORY ${destination_dir})

  message(STATUS "Copying ${file} to ${destination_path}")
  file(COPY ${file} DESTINATION ${destination_dir})
endforeach()

file(DOWNLOAD https://raw.githubusercontent.com/frida/frida-gum/698b356fef0ecfc3ac2818f0b387be90e93deeda/COPYING ${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright)

set(NODE_EMBEDDER_API_URL "https://node-embedder-api-prebuilt-vcpkg-port-v2.b-cdn.net/archive2_1.zip")
set(NODE_EMBEDDER_API_FILENAME "node-embedder-api-prebuilt.zip")
set(NODE_EMBEDDER_API_SHA512 bea6dbf02783e3cafbe8a752f3ce245e85189d3946902aece5a5ff6373cec684a986aa65df279f1c79fc886be380edac0926fc3808930a29a013d7fefeec67da)

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

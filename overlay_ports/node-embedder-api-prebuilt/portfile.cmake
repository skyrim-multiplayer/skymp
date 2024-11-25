set(NODE_EMBEDDER_API_URL "https://node-embedder-api-prebuilt-vcpkg-port.b-cdn.net/node-embedder-api-prebuilt.zip")
set(NODE_EMBEDDER_API_FILENAME "node-embedder-api-prebuilt.zip")
set(NODE_EMBEDDER_API_SHA512 0082a1537052d2343a867cac2eb9acb9ba2da1607c3766ef5ada6fd2498e0fc3df754cabf3a47825358e441a7f594addcf8fdf3be84b05f27fa6eb8aefa1e2ed)

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

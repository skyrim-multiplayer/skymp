project(platform_se)

if(BUILD_SKYRIM_PLATFORM)
  set(DEV_SERVICE_CFG_PATH "tools/dev_service/config.js")
  file(WRITE ${DEV_SERVICE_CFG_PATH} "")
  file(APPEND ${DEV_SERVICE_CFG_PATH} "module.exports = {\n")
  file(APPEND ${DEV_SERVICE_CFG_PATH} "  SkyrimSEFolder:\n")
  file(APPEND ${DEV_SERVICE_CFG_PATH} "    '${SKYRIM_DIR}',\n")
  file(APPEND ${DEV_SERVICE_CFG_PATH} "};\n")

  set(SKYRIM_PLATFORM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
  add_subdirectory(src/platform_lib _platform_lib)
  add_subdirectory(src/platform_se _platform_se)
else()
  add_custom_target(skyrim-platform ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Building skyrim-platform is disabled. To enable it, set BUILD_SKYRIM_PLATFORM to ON."
  )
endif()

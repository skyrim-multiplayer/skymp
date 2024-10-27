project(platform_se)

set(DEV_SERVICE_CFG_PATH "tools/dev_service/config.js")
file(WRITE ${DEV_SERVICE_CFG_PATH} "")
file(APPEND ${DEV_SERVICE_CFG_PATH} "module.exports = {\n")
file(APPEND ${DEV_SERVICE_CFG_PATH} "  SkyrimSEFolder:\n")
file(APPEND ${DEV_SERVICE_CFG_PATH} "    '${SKYRIM_DIR}',\n")
file(APPEND ${DEV_SERVICE_CFG_PATH} "};\n")

set(SKYRIM_PLATFORM_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(src/platform_lib _platform_lib)
add_subdirectory(src/platform_se _platform_se)
#add_subdirectory(src/skyrim_nodejs_plugin _skyrim_nodejs_plugin)

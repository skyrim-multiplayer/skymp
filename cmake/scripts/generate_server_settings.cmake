# Usage: "cmake -P generate_server_settings.cmake -DSERVER_SETTINGS_JSON_PATH=<path_to_server_settings.json> -DOFFLINE_MODE=<true_or_false>"

# read current server-settings.json
if(EXISTS "${SERVER_SETTINGS_JSON_PATH}")
    file(READ "${SERVER_SETTINGS_JSON_PATH}" SERVER_SETTINGS_JSON)
else()
    set(SERVER_SETTINGS_JSON "{}")
endif()

if(OFFLINE_MODE)
    string(JSON SERVER_SETTINGS_JSON SET "${SERVER_SETTINGS_JSON}" "offlineMode" "true")
    message(STATUS "Setting offline_mode to true")
else()
    string(JSON SERVER_SETTINGS_JSON SET "${SERVER_SETTINGS_JSON}" "offlineMode" "true")
    message(STATUS "Setting offline_mode to false")
endif()

# write new server-settings.json
message(STATUS "Writing ${SERVER_SETTINGS_JSON} to ${SERVER_SETTINGS_JSON_PATH}")
file(WRITE "${SERVER_SETTINGS_JSON_PATH}" "${SERVER_SETTINGS_JSON}")

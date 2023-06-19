# Usage: "cmake -P generate_client_settings.cmake -DCLIENT_SETTINGS_JSON_PATH=<path_to_client_settings.json> -DOFFLINE_MODE=<true_or_false>"

# read current server-settings.json
if(EXISTS "${CLIENT_SETTINGS_JSON_PATH}")
    file(READ "${CLIENT_SETTINGS_JSON_PATH}" CLIENT_SETTINGS_JSON)
else()
    set(CLIENT_SETTINGS_JSON "{}")
    string(JSON CLIENT_SETTINGS_JSON SET "${CLIENT_SETTINGS_JSON}" "server-ip" "\"127.0.0.1\"")
    string(JSON CLIENT_SETTINGS_JSON SET "${CLIENT_SETTINGS_JSON}" "server-port" "7777")

    set(lobby_location "{
        \"pos\": [133710, -61252, 14605],
        \"rot\": [0, 0, 0],
        \"worldOrCell\": 60
    }")
    string(JSON CLIENT_SETTINGS_JSON SET "${CLIENT_SETTINGS_JSON}" "lobbyLocation" "${lobby_location}")
endif()

if(OFFLINE_MODE)
    set(profile_id "")
    string(JSON profile_id ERROR_VARIABLE dummy GET "${CLIENT_SETTINGS_JSON}" "gameData" "profileId")

    # check if profile_id is number
    if(NOT profile_id MATCHES "^[0-9]+$")
        set(profile_id 1)
    endif()
    
    string(JSON CLIENT_SETTINGS_JSON SET "${CLIENT_SETTINGS_JSON}" "gameData" "{ \"profileId\": ${profile_id} }")
    string(JSON CLIENT_SETTINGS_JSON SET "${CLIENT_SETTINGS_JSON}" "master" "\"\"")
    string(JSON CLIENT_SETTINGS_JSON SET "${CLIENT_SETTINGS_JSON}" "server-master-key" "null")
else()
    string(JSON CLIENT_SETTINGS_JSON REMOVE "${CLIENT_SETTINGS_JSON}" "gameData")
    string(JSON CLIENT_SETTINGS_JSON SET "${CLIENT_SETTINGS_JSON}" "master" "\"https://sweetpie.nic11.xyz\"")
    file(DOWNLOAD "https://api.ipify.org" "${CMAKE_CURRENT_BINARY_DIR}/ip.txt")
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/ip.txt" ip)
    string(JSON CLIENT_SETTINGS_JSON SET "${CLIENT_SETTINGS_JSON}" "server-master-key" "\"${ip}:7777\"")
endif()

file(WRITE "${CLIENT_SETTINGS_JSON_PATH}" "${CLIENT_SETTINGS_JSON}")

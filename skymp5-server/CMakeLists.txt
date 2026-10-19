project(skymp5-server)
enable_testing()

set(SKYMP5_SERVER_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/package_manager.cmake)

find_package(CMakeRC CONFIG REQUIRED)

if(WIN32)
  npm_execute_command(
    COMMAND ci
    WORKING_DIRECTORY ${SKYMP5_SERVER_SOURCE_DIR}
  )
endif()

# Find node-addon-api
if(NOT EMSCRIPTEN)
  find_package(unofficial-node-addon-api REQUIRED)
endif()

# Add C++ part of the server (except cpp/addon)
add_subdirectory(cpp)

# Add TypeScript part of the server
add_subdirectory(ts)

# Add C++ addon for NodeJS
if(NOT EMSCRIPTEN)
  file(GLOB_RECURSE sources "${SKYMP5_SERVER_SOURCE_DIR}/cpp/addon/*")
  if(MSVC)
    list(APPEND sources "${CMAKE_SOURCE_DIR}/.clang-format")
  endif()
  add_library(${PROJECT_NAME} SHARED ${sources})

  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "scam_native" PREFIX "" SUFFIX ".node")
  target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::node-addon-api::node-addon-api)
  target_include_directories(${PROJECT_NAME} PRIVATE ${SKYMP5_SERVER_SOURCE_DIR}/cpp/addon)
  target_compile_definitions(${PROJECT_NAME} PRIVATE NAPI_CPP_EXCEPTIONS)

  # Linux: explicitly export `napi_register_module_v1` and hide everything else to avoid collisions with symbols defined in the `node` binary.
  # macOS: the same thing should be done via -exported_symbols_list in the future.
  # Windows: probably(?) safe, because symbols aren't exported by default.
  # TODO: macOS?
  if(NOT WIN32 AND NOT APPLE)
    set(version_script_path "${CMAKE_CURRENT_BINARY_DIR}/exports.map")
    file(WRITE "${version_script_path}" "{ global: napi_register_module_v1; local: *; };\n")

    target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--version-script,${version_script_path}")
  endif()

  if(WIN32)
    find_library(node_api-headers_LIB NAMES node PATHS "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows-sp/lib/" NO_DEFAULT_PATH REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${node_api-headers_LIB})
  endif()

  # Make 'cmake --build . --target skymp5-server' building typescript part as well
  add_dependencies(${PROJECT_NAME} skymp5-server-ts)

  # Ensure runtime artifacts are available in clean builds.
  if(TARGET skymp5-functions-lib)
    add_dependencies(${PROJECT_NAME} skymp5-functions-lib)
  endif()

  if(TARGET skymp5-scripts)
    add_dependencies(${PROJECT_NAME} skymp5-scripts)
  endif()

  # Put build artifacts into build/dist/server
  if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/dist/server"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/dist/server"
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/dist/server"
      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/dist/server"
    )
  else()
    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/dist/server/$<TARGET_FILE_NAME:${PROJECT_NAME}>
    )
  endif()

  target_link_libraries(${PROJECT_NAME} PRIVATE mp_common server_guest_lib localization-provider)
  list(APPEND VCPKG_DEPENDENT ${PROJECT_NAME})
  apply_default_settings(TARGETS ${PROJECT_NAME})
endif()

# All targets are ready

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/dist/server)

set(launch_server_sh_dist "#!/usr/bin/env sh\n")
set(launch_server_sh_dist "${launch_server_sh_dist}set -eu\n")
set(launch_server_sh_dist "${launch_server_sh_dist}SCRIPT_DIR=\"$(CDPATH= cd -- \"$(dirname -- \"$0\")\" && pwd)\"\n")
set(launch_server_sh_dist "${launch_server_sh_dist}cd \"$SCRIPT_DIR\"\n")
set(launch_server_sh_dist "${launch_server_sh_dist}ensure_node() {\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  if command -v node >/dev/null 2>&1; then\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    return 0\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  fi\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  echo '[SkyMP] Node.js not found. Trying automatic install...'\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  if command -v apt-get >/dev/null 2>&1; then\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    if command -v sudo >/dev/null 2>&1; then sudo apt-get update && sudo apt-get install -y nodejs npm; else apt-get update && apt-get install -y nodejs npm; fi\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  elif command -v dnf >/dev/null 2>&1; then\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    if command -v sudo >/dev/null 2>&1; then sudo dnf install -y nodejs npm; else dnf install -y nodejs npm; fi\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  elif command -v yum >/dev/null 2>&1; then\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    if command -v sudo >/dev/null 2>&1; then sudo yum install -y nodejs npm; else yum install -y nodejs npm; fi\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  elif command -v pacman >/dev/null 2>&1; then\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    if command -v sudo >/dev/null 2>&1; then sudo pacman -Sy --noconfirm nodejs npm; else pacman -Sy --noconfirm nodejs npm; fi\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  elif command -v zypper >/dev/null 2>&1; then\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    if command -v sudo >/dev/null 2>&1; then sudo zypper --non-interactive install nodejs20 npm20; else zypper --non-interactive install nodejs20 npm20; fi\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  fi\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  if ! command -v node >/dev/null 2>&1; then\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    echo '[SkyMP] Node.js is required. Please install Node.js LTS manually: https://nodejs.org' >&2\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    return 1\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  fi\n")
set(launch_server_sh_dist "${launch_server_sh_dist}}\n")
set(launch_server_sh_dist "${launch_server_sh_dist}ensure_node_modules() {\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  if [ -d node_modules ] || [ ! -f package.json ]; then\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    return 0\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  fi\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  if ! command -v npm >/dev/null 2>&1; then\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    echo '[SkyMP] npm is not available after Node.js setup.' >&2\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    return 1\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  fi\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  echo '[SkyMP] Installing runtime dependencies (first run)...'\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  if [ -f package-lock.json ]; then\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    npm ci --omit=dev || npm install --omit=dev\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  else\n")
set(launch_server_sh_dist "${launch_server_sh_dist}    npm install --omit=dev\n")
set(launch_server_sh_dist "${launch_server_sh_dist}  fi\n")
set(launch_server_sh_dist "${launch_server_sh_dist}}\n")
set(launch_server_sh_dist "${launch_server_sh_dist}ensure_node\n")
set(launch_server_sh_dist "${launch_server_sh_dist}ensure_node_modules\n")
set(launch_server_sh_dist "${launch_server_sh_dist}exec node dist_back/skymp5-server.js \"$@\"\n")
file(WRITE ${CMAKE_BINARY_DIR}/dist/server/launch_server.sh "${launch_server_sh_dist}")
file(CHMOD ${CMAKE_BINARY_DIR}/dist/server/launch_server.sh
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

configure_file(${CMAKE_SOURCE_DIR}/misc/update/update_server.sh ${CMAKE_BINARY_DIR}/dist/server/update_server.sh COPYONLY)
file(CHMOD ${CMAKE_BINARY_DIR}/dist/server/update_server.sh
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

configure_file(${CMAKE_SOURCE_DIR}/misc/update/update_server.ps1 ${CMAKE_BINARY_DIR}/dist/server/update_server.ps1 COPYONLY)

set(launch_server_sh_dev "#!/usr/bin/env sh\n")
set(launch_server_sh_dev "${launch_server_sh_dev}set -eu\n")
set(launch_server_sh_dev "${launch_server_sh_dev}SCRIPT_DIR=\"$(CDPATH= cd -- \"$(dirname -- \"$0\")\" && pwd)\"\n")
set(launch_server_sh_dev "${launch_server_sh_dev}cd \"$SCRIPT_DIR/dist/server\"\n")
set(launch_server_sh_dev "${launch_server_sh_dev}ensure_node() {\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  if command -v node >/dev/null 2>&1; then\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    return 0\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  fi\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  echo '[SkyMP] Node.js not found. Trying automatic install...'\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  if command -v apt-get >/dev/null 2>&1; then\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    if command -v sudo >/dev/null 2>&1; then sudo apt-get update && sudo apt-get install -y nodejs npm; else apt-get update && apt-get install -y nodejs npm; fi\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  elif command -v dnf >/dev/null 2>&1; then\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    if command -v sudo >/dev/null 2>&1; then sudo dnf install -y nodejs npm; else dnf install -y nodejs npm; fi\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  elif command -v yum >/dev/null 2>&1; then\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    if command -v sudo >/dev/null 2>&1; then sudo yum install -y nodejs npm; else yum install -y nodejs npm; fi\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  elif command -v pacman >/dev/null 2>&1; then\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    if command -v sudo >/dev/null 2>&1; then sudo pacman -Sy --noconfirm nodejs npm; else pacman -Sy --noconfirm nodejs npm; fi\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  elif command -v zypper >/dev/null 2>&1; then\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    if command -v sudo >/dev/null 2>&1; then sudo zypper --non-interactive install nodejs20 npm20; else zypper --non-interactive install nodejs20 npm20; fi\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  fi\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  if ! command -v node >/dev/null 2>&1; then\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    echo '[SkyMP] Node.js is required. Please install Node.js LTS manually: https://nodejs.org' >&2\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    return 1\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  fi\n")
set(launch_server_sh_dev "${launch_server_sh_dev}}\n")
set(launch_server_sh_dev "${launch_server_sh_dev}ensure_node_modules() {\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  if [ -d node_modules ] || [ ! -f package.json ]; then\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    return 0\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  fi\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  if ! command -v npm >/dev/null 2>&1; then\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    echo '[SkyMP] npm is not available after Node.js setup.' >&2\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    return 1\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  fi\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  echo '[SkyMP] Installing runtime dependencies (first run)...'\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  if [ -f package-lock.json ]; then\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    npm ci --omit=dev || npm install --omit=dev\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  else\n")
set(launch_server_sh_dev "${launch_server_sh_dev}    npm install --omit=dev\n")
set(launch_server_sh_dev "${launch_server_sh_dev}  fi\n")
set(launch_server_sh_dev "${launch_server_sh_dev}}\n")
set(launch_server_sh_dev "${launch_server_sh_dev}ensure_node\n")
set(launch_server_sh_dev "${launch_server_sh_dev}ensure_node_modules\n")
set(launch_server_sh_dev "${launch_server_sh_dev}exec node dist_back/skymp5-server.js \"$@\"\n")
file(WRITE ${CMAKE_BINARY_DIR}/launch_server.sh "${launch_server_sh_dev}")
file(CHMOD ${CMAKE_BINARY_DIR}/launch_server.sh
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)

if(WIN32)
  # On Windows create launch_server.bat in ./build/dist/server
  # This .bat file is for launching the server
  set(bat_dist "@echo off\n")
  set(bat_dist "${bat_dist}setlocal\n")
  set(bat_dist "${bat_dist}cd /d \"%~dp0\"\n")
  set(bat_dist "${bat_dist}call :ensure_node\n")
  set(bat_dist "${bat_dist}if errorlevel 1 goto finish\n")
  set(bat_dist "${bat_dist}call :ensure_node_modules\n")
  set(bat_dist "${bat_dist}if errorlevel 1 goto finish\n")
  set(bat_dist "${bat_dist}echo [SkyMP] Checking for existing server process...\n")
  set(bat_dist "${bat_dist}powershell -NoProfile -ExecutionPolicy Bypass -Command \"Get-CimInstance Win32_Process | Where-Object { $_.Name -eq 'node.exe' -and $_.CommandLine -match 'dist_back[\\\\/]skymp5-server\\.js' } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }\" >nul 2>nul\n")
  set(bat_dist "${bat_dist}echo [SkyMP] Starting server...\n")
  set(bat_dist "${bat_dist}node dist_back/skymp5-server.js\n")
  set(bat_dist "${bat_dist}set EXIT_CODE=%ERRORLEVEL%\n")
  set(bat_dist "${bat_dist}goto finish\n")
  set(bat_dist "${bat_dist}:ensure_node\n")
  set(bat_dist "${bat_dist}where node >nul 2>nul && exit /b 0\n")
  set(bat_dist "${bat_dist}echo [SkyMP] Node.js not found in PATH. Trying automatic install...\n")
  set(bat_dist "${bat_dist}where winget >nul 2>nul\n")
  set(bat_dist "${bat_dist}if not errorlevel 1 winget install --id OpenJS.NodeJS.LTS -e --silent --accept-package-agreements --accept-source-agreements\n")
  set(bat_dist "${bat_dist}set PATH=%ProgramFiles%\\nodejs;%LocalAppData%\\Programs\\nodejs;%PATH%\n")
  set(bat_dist "${bat_dist}where node >nul 2>nul && exit /b 0\n")
  set(bat_dist "${bat_dist}echo [SkyMP] Node.js is required. Install Node.js LTS from https://nodejs.org\n")
  set(bat_dist "${bat_dist}exit /b 1\n")
  set(bat_dist "${bat_dist}:ensure_node_modules\n")
  set(bat_dist "${bat_dist}if exist node_modules exit /b 0\n")
  set(bat_dist "${bat_dist}if not exist package.json exit /b 0\n")
  set(bat_dist "${bat_dist}where npm >nul 2>nul || (echo [SkyMP] npm not found after Node.js setup.& exit /b 1)\n")
  set(bat_dist "${bat_dist}echo [SkyMP] Installing runtime dependencies (first run)...\n")
  set(bat_dist "${bat_dist}if exist package-lock.json (call npm ci --omit=dev && exit /b 0)\n")
  set(bat_dist "${bat_dist}call npm install --omit=dev\n")
  set(bat_dist "${bat_dist}if errorlevel 1 exit /b 1\n")
  set(bat_dist "${bat_dist}exit /b 0\n")
  set(bat_dist "${bat_dist}:finish\n")
  set(bat_dist "${bat_dist}if not defined EXIT_CODE set EXIT_CODE=1\n")
  set(bat_dist "${bat_dist}pause\n")
  set(bat_dist "${bat_dist}endlocal & exit /B %EXIT_CODE%\n")
  file(WRITE ${CMAKE_BINARY_DIR}/dist/server/launch_server.bat "${bat_dist}")

  # Same but for dev (no pause, placed into ./build dir)
  set(bat "@echo off\n")
  set(bat "${bat}setlocal\n")
  set(bat "${bat}cd dist/server\n")
  set(bat "${bat}call :ensure_node\n")
  set(bat "${bat}if errorlevel 1 goto finish\n")
  set(bat "${bat}call :ensure_node_modules\n")
  set(bat "${bat}if errorlevel 1 goto finish\n")
  set(bat "${bat}echo [SkyMP] Checking for existing server process...\n")
  set(bat "${bat}powershell -NoProfile -ExecutionPolicy Bypass -Command \"Get-CimInstance Win32_Process | Where-Object { $_.Name -eq 'node.exe' -and $_.CommandLine -match 'dist_back[\\\\/]skymp5-server\\.js' } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }\" >nul 2>nul\n")
  set(bat "${bat}echo [SkyMP] Starting server...\n")
  set(bat "${bat}node dist_back/skymp5-server.js\n")
  set(bat "${bat}set EXIT_CODE=%ERRORLEVEL%\n")
  set(bat "${bat}goto finish\n")
  set(bat "${bat}:ensure_node\n")
  set(bat "${bat}where node >nul 2>nul && exit /b 0\n")
  set(bat "${bat}echo [SkyMP] Node.js not found in PATH. Trying automatic install...\n")
  set(bat "${bat}where winget >nul 2>nul\n")
  set(bat "${bat}if not errorlevel 1 winget install --id OpenJS.NodeJS.LTS -e --silent --accept-package-agreements --accept-source-agreements\n")
  set(bat "${bat}set PATH=%ProgramFiles%\\nodejs;%LocalAppData%\\Programs\\nodejs;%PATH%\n")
  set(bat "${bat}where node >nul 2>nul && exit /b 0\n")
  set(bat "${bat}echo [SkyMP] Node.js is required. Install Node.js LTS from https://nodejs.org\n")
  set(bat "${bat}exit /b 1\n")
  set(bat "${bat}:ensure_node_modules\n")
  set(bat "${bat}if exist node_modules exit /b 0\n")
  set(bat "${bat}if not exist package.json exit /b 0\n")
  set(bat "${bat}where npm >nul 2>nul || (echo [SkyMP] npm not found after Node.js setup.& exit /b 1)\n")
  set(bat "${bat}echo [SkyMP] Installing runtime dependencies (first run)...\n")
  set(bat "${bat}if exist package-lock.json (call npm ci --omit=dev && exit /b 0)\n")
  set(bat "${bat}call npm install --omit=dev\n")
  set(bat "${bat}if errorlevel 1 exit /b 1\n")
  set(bat "${bat}exit /b 0\n")
  set(bat "${bat}:finish\n")
  set(bat "${bat}if not defined EXIT_CODE set EXIT_CODE=1\n")
  set(bat "${bat}endlocal & exit /B %EXIT_CODE%\n")
  file(WRITE ${CMAKE_BINARY_DIR}/launch_server.bat "${bat}")
endif()

# Create server-settings.json in build/dist/server

if(SKYRIM_DIR)
  set(esm_prefix "${SKYRIM_DIR}/Data/")
elseif(UNIT_DATA_DIR)
  set(esm_prefix "${UNIT_DATA_DIR}/")
else()
  set(esm_prefix "")
endif()

message(STATUS "esm_prefix is '${esm_prefix}'")

# Defaults for custom launcher/master deployments.
# These can be overridden via -D on CMake configure.
set(SKYMP_MASTER_URL "https://api.skymp-worlds.net" CACHE STRING "Master API base URL for server heartbeats and session APIs")
set(SKYMP_MASTER_HEARTBEAT_INTERVAL_MS "15000" CACHE STRING "Heartbeat interval in milliseconds (clamped by runtime to 10-30s)")
set(SKYMP_DEFAULT_GAMEMODE "" CACHE STRING "Optional default gamemode sent in heartbeats")
set(SKYMP_DEFAULT_COUNTRY_CODE "" CACHE STRING "Optional default ISO country code sent in heartbeats (e.g. DE)")
set(SKYMP_DEFAULT_PUBLIC_HOST "" CACHE STRING "Optional public host/IP override for heartbeat announcements")

set(server_data_source_dir "")
if(SKYRIM_DIR)
  set(server_data_source_dir "${SKYRIM_DIR}/Data")
elseif(UNIT_DATA_DIR)
  set(server_data_source_dir "${UNIT_DATA_DIR}")
endif()

if(TARGET ${PROJECT_NAME})
  # Generate dist/server/server-settings.json with EMPTY ESM_PREFIX so that
  # loadOrder entries are always relative filenames (e.g. "Skyrim.esm").
  # The 'dataDir' field already tells the server where to find the files.
  # Using absolute CI build paths here would break the packaged release.
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
      -DESM_PREFIX=
      -DSERVER_SETTINGS_JSON_PATH=${CMAKE_BINARY_DIR}/dist/server/server-settings.json
      -DOFFLINE_MODE=${OFFLINE_MODE}
      -DMASTER_URL=${SKYMP_MASTER_URL}
      -DMASTER_HEARTBEAT_INTERVAL_MS=${SKYMP_MASTER_HEARTBEAT_INTERVAL_MS}
      -DDEFAULT_GAMEMODE=${SKYMP_DEFAULT_GAMEMODE}
      -DDEFAULT_COUNTRY_CODE=${SKYMP_DEFAULT_COUNTRY_CODE}
      -DDEFAULT_PUBLIC_HOST=${SKYMP_DEFAULT_PUBLIC_HOST}
      -P ${CMAKE_SOURCE_DIR}/cmake/scripts/generate_server_settings.cmake
    COMMENT "Generating dist/server/server-settings.json (relative loadOrder)"
  )

  # Generate server-settings-base.json for local dev/testing with the full
  # ESM_PREFIX so the server can resolve Skyrim data files directly.
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
      -DESM_PREFIX=${esm_prefix}
      -DSERVER_SETTINGS_JSON_PATH=${CMAKE_BINARY_DIR}/server-settings-base.json
      -DOFFLINE_MODE=${OFFLINE_MODE}
      -DMASTER_URL=${SKYMP_MASTER_URL}
      -DMASTER_HEARTBEAT_INTERVAL_MS=${SKYMP_MASTER_HEARTBEAT_INTERVAL_MS}
      -DDEFAULT_GAMEMODE=${SKYMP_DEFAULT_GAMEMODE}
      -DDEFAULT_COUNTRY_CODE=${SKYMP_DEFAULT_COUNTRY_CODE}
      -DDEFAULT_PUBLIC_HOST=${SKYMP_DEFAULT_PUBLIC_HOST}
      -P ${CMAKE_SOURCE_DIR}/cmake/scripts/generate_server_settings.cmake
    COMMENT "Generating server-settings-base.json (local dev with ESM_PREFIX)"
  )

  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SKYMP5_SERVER_SOURCE_DIR}/package.json ${CMAKE_BINARY_DIR}/dist/server/package.json
  )

  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SKYMP5_SERVER_SOURCE_DIR}/package-lock.json ${CMAKE_BINARY_DIR}/dist/server/package-lock.json
  )

  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
      -DDEST_DIR=${CMAKE_BINARY_DIR}/dist/server/data
      -DPEX_SOURCE_DIR=${CMAKE_SOURCE_DIR}/skymp5-scripts/pex
      -DSOURCE_DATA_DIR=${server_data_source_dir}
      -P ${CMAKE_SOURCE_DIR}/cmake/scripts/prepare_server_data.cmake
  )

  set(FRONTEND_BUILD_JS "${CMAKE_SOURCE_DIR}/skymp5-front/dist/build.js")
  set(FRONTEND_UI_DIR "${CMAKE_BINARY_DIR}/dist/server/ui")
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
      -DFRONTEND_BUILD_JS=${FRONTEND_BUILD_JS}
      -DFRONTEND_UI_DIR=${FRONTEND_UI_DIR}
      -P ${CMAKE_SOURCE_DIR}/cmake/scripts/copy_frontend_if_exists.cmake
    COMMENT "Copying frontend build.js to dist/server/ui/ (if available)"
  )
endif()

file(GLOB_RECURSE standard_scripts
  ${CMAKE_CURRENT_SOURCE_DIR}/standard_scripts/*
)
cmrc_add_resource_library(server_standard_scripts ${standard_scripts}
  NAMESPACE server_standard_scripts
)
apply_default_settings(TARGETS server_standard_scripts)
target_link_libraries(server_guest_lib PUBLIC server_standard_scripts)

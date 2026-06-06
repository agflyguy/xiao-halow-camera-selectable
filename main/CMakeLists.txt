set(_cfg_path "${CMAKE_CURRENT_LIST_DIR}/camera_build_config.h")
file(READ "${_cfg_path}" _camera_build_config_h)

if(_camera_build_config_h MATCHES "#define[ \t]+USE_WIFI[ \t]+([01])")
    set(USE_WIFI "${CMAKE_MATCH_1}")
else()
    set(USE_WIFI 0)
    message(WARNING "camera_build_config.h: could not parse USE_WIFI — defaulting to 0 (HaLow)")
endif()

if(_camera_build_config_h MATCHES "#define[ \t]+ENABLE_OV3660_SETTINGS[ \t]+([01])")
    set(ENABLE_OV3660_SETTINGS "${CMAKE_MATCH_1}")
else()
    set(ENABLE_OV3660_SETTINGS 0)
    message(WARNING "camera_build_config.h: could not parse ENABLE_OV3660_SETTINGS — defaulting to 0")
endif()

unset(USE_WIFI CACHE)
unset(ENABLE_OV3660_SETTINGS CACHE)

set(src
    "mm_app_loadconfig.c"
    "mm_app_common.c"
    "camera_httpd.cpp"
    "main.c")

if(USE_WIFI)
    list(APPEND src "app_wifi.c")
endif()

set(priv_requires driver morselib mm_shims mmipal lwip esp_timer esp_http_server esp_wifi esp_netif esp_event nvs_flash)

idf_component_register(SRCS ${src}
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES ${priv_requires})

# Recompile main + httpd when the header changes. Flags come from #include, not -D.
set_source_files_properties(
    main.c camera_httpd.cpp
    PROPERTIES OBJECT_DEPENDS "${_cfg_path}")

set_source_files_properties(camera_httpd.cpp PROPERTIES COMPILE_FLAGS "-Wno-format")

if(OPENMANET_MESH)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE OPENMANET_MESH=1)
    message(STATUS "HaLow preset: OPENMANET mesh (haven / havenmesh)")
endif()
if(DEFINED HALOW_SSID)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE "APP_HALOW_SSID=\"${HALOW_SSID}\"")
    message(STATUS "HaLow SSID override: ${HALOW_SSID}")
endif()
if(DEFINED HALOW_PASSPHRASE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE "APP_HALOW_PASSPHRASE=\"${HALOW_PASSPHRASE}\"")
    message(STATUS "HaLow passphrase override: (set)")
endif()
if(NOT DEFINED HALOW_SSID AND NOT OPENMANET_MESH)
    message(STATUS "HaLow defaults: grey-AP / heltec.org")
endif()

file(TIMESTAMP _cfg_mtime "${_cfg_path}" UTC)
message(STATUS "camera_build_config.h: ${_cfg_path} (mtime ${_cfg_mtime})")
message(STATUS "XIAO camera USE_WIFI=${USE_WIFI} ENABLE_OV3660_SETTINGS=${ENABLE_OV3660_SETTINGS}")

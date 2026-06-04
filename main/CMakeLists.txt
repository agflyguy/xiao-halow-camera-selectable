set(src
    "mm_app_loadconfig.c"
    "mm_app_common.c"
    "camera_http.c"
    "main.c")

if(NOT DEFINED USE_WIFI)
    set(USE_WIFI 0)
endif()

if(USE_WIFI)
    list(APPEND src "app_wifi.c")
endif()

set(priv_requires driver morselib mm_shims mmipal lwip esp_timer esp_http_server esp_wifi esp_netif esp_event nvs_flash)

idf_component_register(SRCS ${src}
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES ${priv_requires})

target_compile_definitions(${COMPONENT_LIB} PRIVATE USE_WIFI=${USE_WIFI})
# camera_http.c reads USE_WIFI for frame pacing and resolution hints

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

message(STATUS "XIAO camera USE_WIFI=${USE_WIFI}")

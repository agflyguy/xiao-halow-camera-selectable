cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED ENV{MMIOT_ROOT})
    if(EXISTS "$ENV{HOME}/mm-iot-esp32/framework/morselib")
        set(ENV{MMIOT_ROOT} "$ENV{HOME}/mm-iot-esp32")
        message(STATUS "MMIOT_ROOT auto-detected: $ENV{HOME}/mm-iot-esp32")
    else()
        message(FATAL_ERROR
            "MMIOT_ROOT is not set and ~/mm-iot-esp32 was not found.\n"
            "  cd xiao-halow-camera\n"
            "  git clone https://github.com/Seeed-Studio/mm-iot-esp32.git ~/mm-iot-esp32\n"
            "  source ./setenv.sh\n"
            "  idf.py build")
    endif()
endif()

message(STATUS "MMIOT_ROOT: $ENV{MMIOT_ROOT}")

if(NOT DEFINED USE_WIFI)
    set(USE_WIFI 0 CACHE STRING "0=HaLow grey-AP, 1=2.4GHz blue-2g (HT-HC33 style)")
endif()
message(STATUS "USE_WIFI: ${USE_WIFI}")

option(OPENMANET_MESH "HaLow STA: haven / havenmesh (OPENMANET mesh ID)" OFF)

add_compile_definitions(LWIP_NETIF_LINK_CALLBACK=1)

if(DEFINED COUNTRY_CODE)
    add_compile_definitions(COUNTRY_CODE="${COUNTRY_CODE}")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiao_halow_camera)

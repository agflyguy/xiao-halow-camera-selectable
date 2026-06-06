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

option(OPENMANET_MESH "HaLow STA: haven / havenmesh (OPENMANET mesh ID)" OFF)

add_compile_definitions(LWIP_NETIF_LINK_CALLBACK=1)

if(DEFINED COUNTRY_CODE)
    add_compile_definitions(COUNTRY_CODE="${COUNTRY_CODE}")
endif()

# iCloud / interrupted downloads can leave managed_components half-written.
# Remove broken trees so the component manager re-fetches on configure.
set(_mc "${CMAKE_SOURCE_DIR}/managed_components")
if(EXISTS "${_mc}")
    foreach(_broken
        "espressif__esp_jpeg"
        "espressif__esp32-camera"
        "espressif__esp_jpeg 2"
        "espressif__esp32-camera 2")
        set(_path "${_mc}/${_broken}")
        if(EXISTS "${_path}")
            if(_broken STREQUAL "espressif__esp_jpeg" AND NOT IS_DIRECTORY "${_path}/include")
                message(WARNING "Removing incomplete managed component: ${_broken}")
                file(REMOVE_RECURSE "${_path}")
            elseif(_broken STREQUAL "espressif__esp32-camera" AND NOT IS_DIRECTORY "${_path}/driver")
                message(WARNING "Removing incomplete managed component: ${_broken}")
                file(REMOVE_RECURSE "${_path}")
            elseif(_broken MATCHES " 2$")
                message(WARNING "Removing stray managed_components copy: ${_broken}")
                file(REMOVE_RECURSE "${_path}")
            endif()
        endif()
    endforeach()
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(xiao_halow_camera)

# Re-run CMake when main/camera_build_config.h changes (parsed in main/CMakeLists.txt).
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/main/camera_build_config.h")

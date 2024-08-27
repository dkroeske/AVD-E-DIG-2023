#
# Web server example
#

#
cmake_minimum_required(VERSION 3.13)

#
find_package(Perl)
if(NOT PERL_FOUND)
    message(FATAL_ERROR "Perl needed to generate the fsdata.c file")
endif()

execute_process(COMMAND
    perl ${CMAKE_CURRENT_LIST_DIR}/web/makefsdata/makefsdata
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/web/pages
    ECHO_OUTPUT_VARIABLE
    ECHO_ERROR_VARIABLE
)

# Include tools van de PICO sdk
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

# Projectnaam
project(web_ex C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Wifi credentials
set(WIFI_SSID "${WIFI_SSID}" CACHE INTERNAL "WiFi SSID")
set(WIFI_PASSWORD "${WIFI_PASSWORD}" CACHE INTERNAL "WiFi PASSWORD")
if("${WIFI_SSID}" STREQUAL "")
    message("WiFi SSID not defined!")
endif()

if("${WIFI_PASSWORD}" STREQUAL "")
    message("WiFi PASSWORD not defined!")
endif()

#
set(PICO_BOARD pico_w)

# 
pico_sdk_init()
#
add_compile_options(
    -Wall
    -Werror
)

add_executable(${PROJECT_NAME}
    main.c
)

#
pico_add_extra_outputs(${PROJECT_NAME})

#
target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)

# Nodig ivm extra cmake voor lwip stack
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
)

#
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    hardware_gpio
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_http
)

#
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)



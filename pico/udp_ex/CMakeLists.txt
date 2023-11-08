#
# PICO PIO example. 
#
#
cmake_minimum_required(VERSION 3.13)

# Include tools van de PICO sdk
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

# Projectnaam
project(udp_ex C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(WIFI_SSID "${WIFI_SSID}" CACHE INTERNAL "WiFi SSID")
set(WIFI_PASSWORD "${WIFI_PASSWORD}" CACHE INTERNAL "WiFi PASSWORD")

if("${WIFI_SSID}" STREQUAL "")
    message("WiFi SSID niet gedefinieerd!")
endif()

if("${WIFI_PASSWORD}" STREQUAL "")
    message("WiFi PASSWORD niet gedefinieerd!")
endif()


# Blijkbaar moet onderstaande boven pico_sdk_init(). Zie ook output cmake
set(PICO_BOARD pico_w)

# 
pico_sdk_init()

#
add_compile_options(
    -Wall
#    -Werror
)

add_executable(${PROJECT_NAME}
    main.c
    ws2812.c
    artnet.c
)

#
pico_generate_pio_header(${PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio
)

#
pico_add_extra_outputs(${PROJECT_NAME})

# Nodig ivm extra cmake voor lwip stack
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_LIST_DIR}
)
    
#
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    pico_util
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_spi
)

#
target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)

#
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)



#
# Hardware SPI example
# Communicatie PICO met de MCP2515 CAN controller
#

#
cmake_minimum_required(VERSION 3.13)

# Include tools van de PICO sdk
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

# Projectnaam
project(spi_ex C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 
pico_sdk_init()

#
#set(PICO_BOARD pico_w)

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
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    hardware_gpio
    hardware_spi
)

#
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)



#
# SD card example. Base on the Cytron hardware. 
#
#
cmake_minimum_required(VERSION 3.13)

# Include tools van de PICO sdk
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

# Projectnaam
project(sdcard_ex C CXX ASM)
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

add_subdirectory(no-OS-FatFS/src)

# Flex and Bison

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

flex_target(CONFIG_LEXER
    ./ly/src/config_lexer.l
    "${CMAKE_CURRENT_BINARY_DIR}/config_lexer.c"
    COMPILE_FLAGS "--header-file=lex.yy.h"
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/config_lexer.h)


bison_target(CONFIG_PARSER
    ./ly/src/config_parser.y
    "${CMAKE_CURRENT_BINARY_DIR}/config_parser.c"
    COMPILE_FLAGS "--warnings=all --debug --verbose -d"
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/config_parser.h)
 
add_flex_bison_dependency(CONFIG_LEXER CONFIG_PARSER)

# PIO
#pico_generate_pio_header(${PROJECT_NAME}
#    ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio
#)

#
include_directories(${CMAKE_CURRENT_BINARY_DIR})

#
add_executable(${PROJECT_NAME}
    main.c
    hw_config.c         # for SD card library
    ${CMAKE_CURRENT_BINARY_DIR}/config_lexer.c
    ${CMAKE_CURRENT_BINARY_DIR}/config_parser.c
    config.c
)

#
pico_add_extra_outputs(${PROJECT_NAME})

#
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_BINARY_DIR}
)

#
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    FatFs_SPI
    pico_util
    hardware_gpio
    hardware_spi
    hardware_clocks
)

#
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)



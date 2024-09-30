#
# Helloworld example
#

#
cmake_minimum_required(VERSION 3.13)

# Include tools van de PICO sdk
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

# Projectnaam
project(helloworld_pico2 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


if (PICO_SDK_VERSION_STRING VERSION_LESS "2.0.0")
    message(FATAL_ERROR "Raspberry PI Pico SDK version 2.0.0 (or later) required. Your version is $(PICO_SDK_VERSION_STRING)")
endif()

# If you want debug output from USB (pass -DPICO_STDIO_USB=1) this ensures you don't lose any debug output while USB is set up
if (NOT DEFINED PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS)
    set(PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS 3000)
endif()

# Not working
set(PICO_BOARD pico2)

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
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    pico_stdio_usb
)

#
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)



#
# PICO Knikkerbaan
#
# 2025/04/17    dkroeske@gmail.com  Initial
#
#
cmake_minimum_required(VERSION 3.13)

message(STATUS "cmsis-dsp experiment for pico2: adc->fir->pwm_audio")

#set(CMSISDSP ${CMAKE_CURRENT_LIST_DIR}/CMSIS-DSP)
#set(CMSISCORE ${CMAKE_CURRENT_LIST_DIR}/CMSIS_6/CMSIS/Core)

# Include tools van de PICO sdk
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

# Projectnaam
project(knikkerbaan C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.0.0")
    message(FATAL_ERROR "Raspberry PI Pico SDK version 2.0.0 (or later) required. Your version is $(PICO_SDK_VERSION_STRING)")
endif()

# If you want debug output from USB (pass -DPICO_STDIO_USB=1) this ensures you don't lose any debug output while USB is set up
if (NOT DEFINED PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS)
    set(PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS 3000)
endif()

# 
pico_sdk_init()

#
#add_subdirectory(${CMSISDSP}/Source bin_dsp)

#target_compile_options(CMSISDSP PUBLIC 
#    -Wsign-compare
#    -Wdouble-promotion
#    -Ofast -ffast-math
#    -DNDEBUG
#    -Wall -Wextra  -Werror
#    -fshort-enums 
#    #-fshort-wchar
#)


#
add_compile_options(
    -Wall
    -Werror
    -O0
    -g
)

add_executable(${PROJECT_NAME}
    main.c
    servo.c
)

#
#pico_generate_pio_header(${PROJECT_NAME}
#    ${CMAKE_CURRENT_LIST_DIR}/nco.pio
#)

#
pico_add_extra_outputs(${PROJECT_NAME})

#
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/CMSIS-DSP/Include
  ${CMAKE_CURRENT_LIST_DIR}/CMSIS_6/CMSIS/Core/Include
)

#
#    ${CMAKE_CURRENT_LIST_DIR}/CMSISDSP/build/bin_dsp/libCMSISDSP.a
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
#    ${CMAKE_CURRENT_LIST_DIR}/build/bin_dsp/libCMSISDSP.a
    pico_stdio_usb
    hardware_gpio
    hardware_pio
    hardware_dma
    hardware_sync
    hardware_clocks
    hardware_irq
    hardware_pwm
    hardware_adc
)

#
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)



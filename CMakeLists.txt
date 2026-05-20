cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH /usr/share/pico-sdk)
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(polsat-ir-receiver C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

option(POLSAT_ENABLE_SERIAL "Enable USB CDC serial logs" ON)

add_executable(polsat-ir-receiver
    main.c
    ir.c
    sejin.c
    hid_bridge.c
    usb_descriptors.c
)

target_include_directories(polsat-ir-receiver PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(polsat-ir-receiver PRIVATE
    PICO_TUSB_CONFIG_HEADER="tusb_config.h"
    POLSAT_ENABLE_SERIAL=$<BOOL:${POLSAT_ENABLE_SERIAL}>
)

target_link_libraries(polsat-ir-receiver
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

if(POLSAT_ENABLE_SERIAL)
    pico_enable_stdio_usb(polsat-ir-receiver 1)
else()
    pico_enable_stdio_usb(polsat-ir-receiver 0)
endif()

pico_enable_stdio_uart(polsat-ir-receiver 0)
pico_add_extra_outputs(polsat-ir-receiver)

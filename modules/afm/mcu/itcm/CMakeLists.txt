set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/itcm/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/itcm/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/itcm/lib)

# Linker script for the itcm (assuming it uses the same one for now)
set(LINKER_SCRIPT
    ${CMAKE_SOURCE_DIR}/bsp/startup/linker/GOWIN_M1_flash_burn.ld
)

# Bootloader source files
file(GLOB_RECURSE BOOTLOADER_SOURCES
    boot.c
    ${CMAKE_SOURCE_DIR}/src/debug.c
    ${CMAKE_SOURCE_DIR}/src/delay.c
    ${CMAKE_SOURCE_DIR}/src/gpio.c
    ${CMAKE_SOURCE_DIR}/src/hw.c
    ${CMAKE_SOURCE_DIR}/bsp/system/*.c
    ${CMAKE_SOURCE_DIR}/bsp/drivers/src/*.c
    ${CMAKE_SOURCE_DIR}/kernel/*.c
    ${CMAKE_SOURCE_DIR}/kernel/pendsv.S
)

file(GLOB STARTUP_SOURCES
    ${CMAKE_SOURCE_DIR}/bsp/startup/*.S
)

# Define the itcm executable
add_executable(itcm.elf
    ${STARTUP_SOURCES}
    ${BOOTLOADER_SOURCES}
)

# Include directories for the itcm
target_include_directories(itcm.elf PRIVATE
    ${CMAKE_SOURCE_DIR}/kernel
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/bsp/core
    ${CMAKE_SOURCE_DIR}/bsp/system
    ${CMAKE_SOURCE_DIR}/bsp/drivers/inc
    ${CMAKE_SOURCE_DIR}/cheby
)

# CPU + Compile options (same as main project)
target_compile_options(itcm.elf PRIVATE
    -mcpu=cortex-m1
    -mthumb

    -O0
    -g3

    -fmessage-length=0
    -fsigned-char
    -ffunction-sections
    -fdata-sections

    -ffixed-r8
    -ffixed-r9
    -ffixed-r10
    -ffixed-r11

    -std=gnu11
)

# Linker options (same as main project)
target_link_options(itcm.elf PRIVATE
    -mcpu=cortex-m1
    -mthumb

    -T ${LINKER_SCRIPT}

    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/itcm.map
)

# Post-build: generate .bin
add_custom_command(TARGET itcm.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
            -O binary
            $<TARGET_FILE:itcm.elf>
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/itcm.bin
)

# Post-build: print size
add_custom_command(TARGET itcm.elf POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/size_summary.sh
            $<TARGET_FILE:itcm.elf>
)

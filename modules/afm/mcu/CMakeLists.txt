cmake_minimum_required(VERSION 3.16)

# Set the project name
project(softcore_fw_example C ASM)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# --------------------------------------------------
# Output directories
# --------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# --------------------------------------------------
# Linker script
# --------------------------------------------------
set(LINKER_SCRIPT
    ${CMAKE_SOURCE_DIR}/bsp/startup/linker/GOWIN_M1_flash_burn.ld
)

# --------------------------------------------------
# Source files
# --------------------------------------------------
file(GLOB_RECURSE SOURCES
    src/*.c
    bsp/system/*.c
    bsp/drivers/src/*.c
    kernel/*.c
    kernel/pendsv.S
)

file(GLOB STARTUP_SOURCES
    bsp/startup/*.S
)

# --------------------------------------------------
# Bootloader subdirectory
# --------------------------------------------------
add_subdirectory(itcm)

# --------------------------------------------------
# Executable
# --------------------------------------------------
add_executable(${PROJECT_NAME}.elf
    ${STARTUP_SOURCES}
    ${SOURCES}
)

# --------------------------------------------------
# Include directories
# --------------------------------------------------
target_include_directories(${PROJECT_NAME}.elf PRIVATE
    src
    bsp/core
    bsp/system
    bsp/drivers/inc
    kernel
    cheby
)


# --------------------------------------------------
# CPU + Compile options
# --------------------------------------------------
target_compile_options(${PROJECT_NAME}.elf PRIVATE
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

# --------------------------------------------------
# Linker options
# --------------------------------------------------
target_link_options(${PROJECT_NAME}.elf PRIVATE
    -mcpu=cortex-m1
    -mthumb

    -T ${LINKER_SCRIPT}

    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.map
)

# --------------------------------------------------
# Post-build: generate .bin
# --------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND arm-none-eabi-objcopy
            -O binary
            $<TARGET_FILE:${PROJECT_NAME}.elf>
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.bin
)

# --------------------------------------------------
# Post-build: print size
# --------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/size_summary.sh
            $<TARGET_FILE:${PROJECT_NAME}.elf>
)

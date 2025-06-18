cmake_minimum_required(VERSION 3.20)
project(signalmesh C CXX ASM)

# Toolchain (assuming arm-none-eabi is installed)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m7)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

# Toolchain-wide flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 -O2 -ggdb")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${CMAKE_SOURCE_DIR}/modules/aam/cfg/STM32H755xI_M7.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=${PROJECT_NAME}.map")

# Include ChibiOS components
set(CHIBIOS_PATH ${CMAKE_SOURCE_DIR}/import/ChibiOS)
include_directories(
    ${CHIBIOS_PATH}/os/license
    ${CHIBIOS_PATH}/os/common/oslib/include
    ${CHIBIOS_PATH}/os/hal/include
    ${CHIBIOS_PATH}/os/hal/ports/common/ARMCMx
    ${CHIBIOS_PATH}/os/hal/ports/STM32/STM32H7xx
    ${CHIBIOS_PATH}/os/hal/boards/ST_NUCLEO144_H755ZI
    ${CHIBIOS_PATH}/os/rt/include
    ${CHIBIOS_PATH}/os/hal/osal/rt-nil
)

# build the module
add_subdirectory(modules/aam)


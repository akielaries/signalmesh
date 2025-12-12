cmake_minimum_required(VERSION 3.15)
project(audio_peripheral_module C CXX ASM)

set(CMAKE_C_STANDARD 99)
set(COMMON_FLAGS "-mcpu=cortex-m7 -mthumb -O1 -ggdb -fomit-frame-pointer \
                  -falign-functions=16 -ffunction-sections -fdata-sections \
                  -fno-common -flto=auto"
)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -I${CMAKE_SOURCE_DIR}/bsp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${COMMON_FLAGS}")

set(STM32_CHIP STM32H755)
message(STATUS "Building Audio Peripheral Module (APM) on ${STM32_CHIP}")

set(SIGNALMESH_ROOT "../..")
set(TOOLCHAIN "${SIGNALMESH_ROOT}/toolchain")
set(CMAKE_TOOLCHAIN_FILE ${TOOLCHAIN}/gcc_stm32.cmake)
set(CMAKE_MODULE_PATH ${TOOLCHAIN})
#list(APPEND CMAKE_MODULE_PATH "${TOOLCHAIN}")
set(CHIBIOS_ROOT "${SIGNALMESH_ROOT}/import/ChibiOS")

include(${TOOLCHAIN}/gcc_stm32.cmake)
#include(${TOOLCHAIN}/chibiOS_common.cmake)
include(${TOOLCHAIN}/stm32h7xx.cmake)

message(STATUS "")

# create static libraries for some chibios components
#add_chibios_common_library(chibios_common TYPE STATIC)
add_chibios_stm32h7_library(chibios_stm32h7 TYPE STATIC)
include_directories(
    .
    ./bsp
    ./app
    ./tests
    ./bsp/configs
    ./bsp/utils
)

add_definitions(
    -DCHPRINTF_USE_FLOAT=1
    -DCORE_CM7
    -DCORTEX_USE_FPU=FALSE
)

set(SOURCES
    ./bsp/bsp.c
    ./bsp/configs/bsp_uart_config.c
    ./bsp/configs/bsp_gpt_config.c
    ./bsp/configs/bsp_adc_config.c
    ./bsp/configs/bsp_pwm_config.c
    ./bsp/configs/bsp_i2c_config.c
    ./bsp/utils/bsp_gpt.c
    ./bsp/utils/bsp_io.c
    ./drivers/adc.c
    ./drivers/servo.c
    ./drivers/ina219.c
    ./drivers/bme280.c
    #./app/main.c
    #./tests/test_servo.c
    #./tests/test_i2c_lb.c
    #./tests/test_ina219.c
    #./tests/test_i2c_scan.c
    ./tests/test_bme280.c
)

add_executable(${PROJECT_NAME}.elf ${SOURCES})

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -nostartfiles
    -Wl,-Map=${CMAKE_BINARY_DIR}/stm32h755_aam.map,--cref,--no-warn-mismatch
    -Wl,--gc-sections
    -Wl,--library-path=../../../import/ChibiOS/os/common/startup/ARMCMx/compilers/GCC/ld
    -Wl,--script=../../../import/ChibiOS/os/common/startup/ARMCMx/compilers/GCC/ld/STM32H755xI_M7.ld
    -Wl,--defsym=__process_stack_size__=0x400
    -Wl,--defsym=__main_stack_size__=0x400
)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE
  m
  #chibios_common
  chibios_stm32h7
)

target_include_directories(chibios_stm32h7 PUBLIC ${CMAKE_SOURCE_DIR}/bsp)

STM32_ADD_HEX_BIN_TARGETS(${CMAKE_PROJECT_NAME}.elf)
STM32_PRINT_SIZE_OF_TARGETS(${CMAKE_PROJECT_NAME}.elf)
STM32_PRINT_SIZE_DIFF(${CMAKE_PROJECT_NAME}.elf)

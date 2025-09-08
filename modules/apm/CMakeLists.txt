cmake_minimum_required(VERSION 3.15)
project(audio_peripheral_module C ASM)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

set(CMAKE_C_STANDARD 11)

# Compiler flags
set(COMMON_FLAGS "-mcpu=cortex-m7 -mthumb -O2 -ggdb -fomit-frame-pointer -falign-functions=16 -ffunction-sections -fdata-sections -fno-common -flto=auto")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${COMMON_FLAGS}")

# Include directories
include_directories(
    .
    ../../import/ChibiOS/os/common/ext/ARM/CMSIS/Core/Include
    ../../import/ChibiOS/os/common/ext/ST/STM32H7xx
    ../../import/ChibiOS/os/common/oop/include
    ../../import/ChibiOS/os/common/portability/GCC
    ../../import/ChibiOS/os/common/ports/ARM-common/include
    ../../import/ChibiOS/os/common/ports/ARMv7-M
    ../../import/ChibiOS/os/common/startup/ARMCMx/compilers/GCC
    ../../import/ChibiOS/os/common/startup/ARMCMx/devices/STM32H7xx
    ../../import/ChibiOS/os/hal/boards/ST_NUCLEO144_H755ZI
    ../../import/ChibiOS/os/hal/include
    ../../import/ChibiOS/os/hal/lib/streams
    ../../import/ChibiOS/os/hal/osal/rt-nil
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/ADCv4
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/BDMAv1
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/CRYPv1
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/DACv1
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/DMAv2
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/EXTIv1
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/FDCANv2
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/GPIOv2
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/I2Cv3
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/MACv2
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/MDMAv1
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/OTGv1
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/QUADSPIv2
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/RNGv1
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/RTCv2
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/SDMMCv2
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/SPIv3
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/SYSTICKv1
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/TIMv1
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/USART
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/USARTv3
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/xWDGv1
    ../../import/ChibiOS/os/hal/ports/STM32/STM32H7xx
    ../../import/ChibiOS/os/hal/ports/common/ARMCMx
    ../../import/ChibiOS/os/license
    ../../import/ChibiOS/os/oslib/include
    ../../import/ChibiOS/os/rt/include
    ../../import/ChibiOS/os/test/include
    ../../import/ChibiOS/test/oslib/source/test
    ../../import/ChibiOS/test/rt/source/test
    ./cfg
)

# Preprocessor definitions
add_definitions(
    -DCHPRINTF_USE_FLOAT=1
    -DCORE_CM7
    -DCORTEX_USE_FPU=FALSE
)

# Sources
set(SOURCES
    ../../import/ChibiOS/os/common/ports/ARMv7-M/compilers/GCC/chcoreasm.S
    ../../import/ChibiOS/os/common/startup/ARMCMx/compilers/GCC/crt0_v7m.S
    ../../import/ChibiOS/os/common/startup/ARMCMx/compilers/GCC/vectors.S
    ../../import/ChibiOS/os/common/oop/src/oop_base_object.c
    ../../import/ChibiOS/os/common/ports/ARMv7-M/chcore.c
    ../../import/ChibiOS/os/common/startup/ARMCMx/compilers/GCC/crt1.c
    ../../import/ChibiOS/os/hal/boards/ST_NUCLEO144_H755ZI/board.c
    ../../import/ChibiOS/os/hal/lib/streams/bufstreams.c
    ../../import/ChibiOS/os/hal/lib/streams/chprintf.c
    ../../import/ChibiOS/os/hal/lib/streams/chscanf.c
    ../../import/ChibiOS/os/hal/lib/streams/memstreams.c
    ../../import/ChibiOS/os/hal/lib/streams/nullstreams.c
    ../../import/ChibiOS/os/hal/osal/rt-nil/osal.c
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/ADCv4/hal_adc_lld.c
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/BDMAv1/stm32_bdma.c
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/DACv1/hal_dac_lld.c
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/DMAv2/stm32_dma.c
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/EXTIv1/stm32_exti.c
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/GPIOv2/hal_pal_lld.c
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/MDMAv1/stm32_mdma.c
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/SPIv3/hal_spi_v2_lld.c
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/SYSTICKv1/hal_st_lld.c
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/TIMv1/hal_gpt_lld.c
    ../../import/ChibiOS/os/hal/ports/STM32/LLD/USARTv3/hal_serial_lld.c
    ../../import/ChibiOS/os/hal/ports/STM32/STM32H7xx/hal_lld.c
    ../../import/ChibiOS/os/hal/ports/STM32/STM32H7xx/stm32_isr.c
    ../../import/ChibiOS/os/hal/ports/common/ARMCMx/nvic.c
    ../../import/ChibiOS/os/hal/src/hal.c
    ../../import/ChibiOS/os/hal/src/hal_adc.c
    ../../import/ChibiOS/os/hal/src/hal_buffered_serial.c
    ../../import/ChibiOS/os/hal/src/hal_buffers.c
    ../../import/ChibiOS/os/hal/src/hal_dac.c
    ../../import/ChibiOS/os/hal/src/hal_flash.c
    ../../import/ChibiOS/os/hal/src/hal_gpt.c
    ../../import/ChibiOS/os/hal/src/hal_mmcsd.c
    ../../import/ChibiOS/os/hal/src/hal_pal.c
    ../../import/ChibiOS/os/hal/src/hal_queues.c
    ../../import/ChibiOS/os/hal/src/hal_serial.c
    ../../import/ChibiOS/os/hal/src/hal_spi.c
    ../../import/ChibiOS/os/hal/src/hal_st.c
    ../../import/ChibiOS/os/oslib/src/chdelegates.c
    ../../import/ChibiOS/os/oslib/src/chfactory.c
    ../../import/ChibiOS/os/oslib/src/chmboxes.c
    ../../import/ChibiOS/os/oslib/src/chmemchecks.c
    ../../import/ChibiOS/os/oslib/src/chmemcore.c
    ../../import/ChibiOS/os/oslib/src/chmemheaps.c
    ../../import/ChibiOS/os/oslib/src/chmempools.c
    ../../import/ChibiOS/os/oslib/src/chobjcaches.c
    ../../import/ChibiOS/os/oslib/src/chpipes.c
    ../../import/ChibiOS/os/rt/src/chcond.c
    ../../import/ChibiOS/os/rt/src/chdebug.c
    ../../import/ChibiOS/os/rt/src/chdynamic.c
    ../../import/ChibiOS/os/rt/src/chevents.c
    ../../import/ChibiOS/os/rt/src/chinstances.c
    ../../import/ChibiOS/os/rt/src/chmsg.c
    ../../import/ChibiOS/os/rt/src/chmtx.c
    ../../import/ChibiOS/os/rt/src/chregistry.c
    ../../import/ChibiOS/os/rt/src/chrfcu.c
    ../../import/ChibiOS/os/rt/src/chschd.c
    ../../import/ChibiOS/os/rt/src/chsem.c
    ../../import/ChibiOS/os/rt/src/chsys.c
    ../../import/ChibiOS/os/rt/src/chthreads.c
    ../../import/ChibiOS/os/rt/src/chtm.c
    ../../import/ChibiOS/os/rt/src/chtrace.c
    ../../import/ChibiOS/os/rt/src/chvt.c
    ./cfg/portab.c
    src/adc.c
)

add_executable(${PROJECT_NAME}.elf ${SOURCES})

# Linker flags
target_link_options(${PROJECT_NAME}.elf PRIVATE
    -nostartfiles
    -Wl,-Map=${CMAKE_BINARY_DIR}/stm32h755_aam.map,--cref,--no-warn-mismatch
    -Wl,--gc-sections
    -Wl,--library-path=../../../import/ChibiOS/os/common/startup/ARMCMx/compilers/GCC/ld
    -Wl,--script=../../../import/ChibiOS/os/common/startup/ARMCMx/compilers/GCC/ld/STM32H755xI_M7.ld
    -Wl,--defsym=__process_stack_size__=0x400
    -Wl,--defsym=__main_stack_size__=0x400
)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE m)


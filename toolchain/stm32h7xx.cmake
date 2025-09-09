if(NOT DEFINED CHIBIOS_ROOT)
  set(CHIBIOS_ROOT "${CMAKE_SOURCE_DIR}/../../import/ChibiOS" CACHE PATH "Path to ChibiOS sources")
endif()

function(add_chibios_stm32h7_library target)
  cmake_parse_arguments(ARG "" "CHIBIOS_ROOT;TYPE" "" ${ARGN})

  if(ARG_CHIBIOS_ROOT)
    set(_chibios_root "${ARG_CHIBIOS_ROOT}")
  else()
    set(_chibios_root "${CHIBIOS_ROOT}")
  endif()

  if(NOT ARG_TYPE)
    set(ARG_TYPE "STATIC")
  endif()

  FILE(GLOB _stm32h7_sources
    "${_chibios_root}/os/rt/src/*.c"
    "${_chibios_root}/os/hal/src/*.c"
    "${_chibios_root}/os/oslib/src/*.c"
    "${_chibios_root}/os/hal/lib/streams/*.c"
    "${_chibios_root}/os/common/oop/src/oop_base_object.c"

    "${_chibios_root}/os/common/ports/ARMv7-M/compilers/GCC/chcoreasm.S"
    "${_chibios_root}/os/common/startup/ARMCMx/compilers/GCC/crt0_v7m.S"
    "${_chibios_root}/os/common/startup/ARMCMx/compilers/GCC/vectors.S"
    "${_chibios_root}/os/common/oop/src/oop_base_object.c"
    "${_chibios_root}/os/common/ports/ARMv7-M/chcore.c"
    "${_chibios_root}/os/common/startup/ARMCMx/compilers/GCC/crt1.c"
    "${_chibios_root}/os/hal/boards/ST_NUCLEO144_H755ZI/board.c"
    "${_chibios_root}/os/hal/ports/STM32/LLD/ADCv4/hal_adc_lld.c"
    "${_chibios_root}/os/hal/ports/STM32/LLD/BDMAv1/stm32_bdma.c"
    "${_chibios_root}/os/hal/ports/STM32/LLD/DACv1/hal_dac_lld.c"
    "${_chibios_root}/os/hal/ports/STM32/LLD/DMAv2/stm32_dma.c"
    "${_chibios_root}/os/hal/ports/STM32/LLD/EXTIv1/stm32_exti.c"
    "${_chibios_root}/os/hal/ports/STM32/LLD/GPIOv2/hal_pal_lld.c"
    "${_chibios_root}/os/hal/ports/STM32/LLD/MDMAv1/stm32_mdma.c"
    "${_chibios_root}/os/hal/ports/STM32/LLD/SPIv3/hal_spi_v2_lld.c"
    "${_chibios_root}/os/hal/ports/STM32/LLD/SYSTICKv1/hal_st_lld.c"
    "${_chibios_root}/os/hal/ports/STM32/LLD/TIMv1/hal_gpt_lld.c"
    "${_chibios_root}/os/hal/ports/STM32/LLD/USARTv3/hal_serial_lld.c"
    "${_chibios_root}/os/hal/ports/STM32/STM32H7xx/hal_lld.c"
    "${_chibios_root}/os/hal/ports/STM32/STM32H7xx/stm32_isr.c"
    "${_chibios_root}/os/hal/ports/common/ARMCMx/nvic.c"
  )

  if(ARG_TYPE STREQUAL "OBJECT")
    add_library(${target} OBJECT ${_stm32h7_sources})
  else()
    add_library(${target} STATIC ${_stm32h7_sources})
  endif()

  target_include_directories(${target} PUBLIC
    "${_chibios_root}/os/common/oop/include"
    "${_chibios_root}/os/hal/lib/streams"
    "${_chibios_root}/os/hal/include"
    "${_chibios_root}/os/hal/lib/include"
    "${_chibios_root}/os/hal/osal/rt-nil"
    "${_chibios_root}/os/hal/osal/include"
    "${_chibios_root}/os/rt/include"
    "${_chibios_root}/os/oslib/include"
    "${_chibios_root}/os/license"

    "${_chibios_root}/os/common/ext/ARM/CMSIS/Core/Include"
    "${_chibios_root}/os/common/ext/ST/STM32H7xx"
    "${_chibios_root}/os/common/portability/GCC"
    "${_chibios_root}/os/common/ports/ARM-common/include"
    "${_chibios_root}/os/common/ports/ARMv7-M"
    "${_chibios_root}/os/common/startup/ARMCMx/compilers/GCC"
    "${_chibios_root}/os/common/startup/ARMCMx/devices/STM32H7xx"
    "${_chibios_root}/os/hal/boards/ST_NUCLEO144_H755ZI"
    "${_chibios_root}/os/hal/ports/STM32/LLD/ADCv4"
    "${_chibios_root}/os/hal/ports/STM32/LLD/BDMAv1"
    "${_chibios_root}/os/hal/ports/STM32/LLD/CRYPv1"
    "${_chibios_root}/os/hal/ports/STM32/LLD/DACv1"
    "${_chibios_root}/os/hal/ports/STM32/LLD/DMAv2"
    "${_chibios_root}/os/hal/ports/STM32/LLD/EXTIv1"
    "${_chibios_root}/os/hal/ports/STM32/LLD/FDCANv2"
    "${_chibios_root}/os/hal/ports/STM32/LLD/GPIOv2"
    "${_chibios_root}/os/hal/ports/STM32/LLD/I2Cv3"
    "${_chibios_root}/os/hal/ports/STM32/LLD/MACv2"
    "${_chibios_root}/os/hal/ports/STM32/LLD/MDMAv1"
    "${_chibios_root}/os/hal/ports/STM32/LLD/OTGv1"
    "${_chibios_root}/os/hal/ports/STM32/LLD/QUADSPIv2"
    "${_chibios_root}/os/hal/ports/STM32/LLD/RNGv1"
    "${_chibios_root}/os/hal/ports/STM32/LLD/RTCv2"
    "${_chibios_root}/os/hal/ports/STM32/LLD/SDMMCv2"
    "${_chibios_root}/os/hal/ports/STM32/LLD/SPIv3"
    "${_chibios_root}/os/hal/ports/STM32/LLD/SYSTICKv1"
    "${_chibios_root}/os/hal/ports/STM32/LLD/TIMv1"
    "${_chibios_root}/os/hal/ports/STM32/LLD/USART"
    "${_chibios_root}/os/hal/ports/STM32/LLD/USARTv3"
    "${_chibios_root}/os/hal/ports/STM32/LLD/xWDGv1"
    "${_chibios_root}/os/hal/ports/STM32/STM32H7xx"
    "${_chibios_root}/os/hal/ports/common/ARMCMx"
  )
  #target_link_libraries(${target} PUBLIC chibios_common)

  set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endfunction()


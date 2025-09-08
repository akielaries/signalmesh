# Default stack sizes if not provided
IF(NOT CHIBIOS_PROCESS_STACK_SIZE)
  SET(CHIBIOS_PROCESS_STACK_SIZE 0x400)
  MESSAGE(STATUS "No CHIBIOS_PROCESS_STACK_SIZE specified, using default: ${CHIBIOS_PROCESS_STACK_SIZE}")
ENDIF()

IF(NOT CHIBIOS_MAIN_STACK_SIZE)
  SET(CHIBIOS_MAIN_STACK_SIZE 0x400)
  MESSAGE(STATUS "No CHIBIOS_MAIN_STACK_SIZE specified, using default: ${CHIBIOS_MAIN_STACK_SIZE}")
ENDIF()

# Path to the top-level STM32H7 linker script
SET(ChibiOS_LINKER_SCRIPT "${CHIBIOS_ROOT}/os/common/startup/ARMCMx/compilers/GCC/ld/STM32H755xI_M7.ld")

# Make sure linker finds the ChibiOS rules_*.ld files
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L\"../${CHIBIOS_ROOT}/os/common/startup/ARMCMx/compilers/GCC/ld\"")

# Pass stack sizes as linker definitions
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--defsym=__process_stack_size__=${CHIBIOS_PROCESS_STACK_SIZE}")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--defsym=__main_stack_size__=${CHIBIOS_MAIN_STACK_SIZE}")

# Copy the top-level linker script to the build directory for this target
# so CMake can refer to it with STM32_SET_FLASH_PARAMS
IF(NOT ChibiOS_LINKER_SCRIPT_COPY)
  CONFIGURE_FILE(${ChibiOS_LINKER_SCRIPT} ${CMAKE_BINARY_DIR}/chibios_link.ld COPYONLY)
  SET(ChibiOS_LINKER_SCRIPT_COPY ${CMAKE_BINARY_DIR}/chibios_link.ld)
ENDIF()

# Expose the linker script path to STM32 build functions
SET(STM32_LINKER_SCRIPT ${ChibiOS_LINKER_SCRIPT_COPY} CACHE INTERNAL "ChibiOS linker script for STM32H7")


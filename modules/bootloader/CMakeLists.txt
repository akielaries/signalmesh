cmake_minimum_required(VERSION 3.15)
project(signalmesh_bootloader C CXX ASM)

# standalone STM32/APM bootloader. mirrors modules/apm/CMakeLists.txt and reuses
# a MINIMAL slice of the APM BSP (debug console + configs) - NOT bsp.c, which
# runs the full device registry. TODO: give the bootloader its own linker script
# bounding it to 0x08000000..0x08020000 (128K) so the app can start at
# 0x08020000; for now it reuses the full-flash APM linker (fine as a standalone
# image while bringing things up).

set(CMAKE_C_STANDARD 99)
set(COMMON_FLAGS
    "-mcpu=cortex-m7 -mthumb -O1 -ggdb -fomit-frame-pointer \
                  -falign-functions=16 -ffunction-sections -fdata-sections \
                  -fno-common -flto=auto")

set(SIGNALMESH_ROOT "../..")
set(APM_DIR "../apm")
# search the bootloader dir FIRST (before apm/bsp) so our halconf_community.h
# (USB host stack fully off) overrides APM's - for these sources AND the ChibiOS
# HAL library. APM enables USBH for the MIDI keyboard; the bootloader must not,
# or it drags in usbh_midi.c / the hub driver.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR} -I${APM_DIR}/bsp")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${COMMON_FLAGS}")

set(STM32_CHIP STM32H755)
message(STATUS "Building signalmesh bootloader on ${STM32_CHIP}")

set(TOOLCHAIN "${SIGNALMESH_ROOT}/toolchain")
set(CMAKE_TOOLCHAIN_FILE ${TOOLCHAIN}/gcc_stm32.cmake)
set(CMAKE_MODULE_PATH ${TOOLCHAIN})
set(CHIBIOS_ROOT "${SIGNALMESH_ROOT}/import/ChibiOS")
set(CHIBIOS_CONTRIB_ROOT "${SIGNALMESH_ROOT}/import/ChibiOS-Contrib")

include(${TOOLCHAIN}/gcc_stm32.cmake)
include(${TOOLCHAIN}/stm32h7xx.cmake)

add_chibios_stm32h7_library(chibios_stm32h7 TYPE STATIC)

set(DRIVERS_SRC_DIR ${SIGNALMESH_ROOT}/lib/drivers/src)
set(DRIVERS_INCLUDE_DIR ${SIGNALMESH_ROOT}/lib/drivers/include)
set(COMMON_INCLUDE_DIR ${SIGNALMESH_ROOT}/lib/common/include)
set(BOOTLOADER_SRC_DIR ${SIGNALMESH_ROOT}/lib/bootloader/src)
set(BOOTLOADER_INCLUDE_DIR ${SIGNALMESH_ROOT}/lib/bootloader/include)

include_directories(
  .
  ${APM_DIR}
  ${APM_DIR}/bsp
  ${APM_DIR}/bsp/configs
  ${APM_DIR}/bsp/utils
  ${DRIVERS_INCLUDE_DIR}
  ${COMMON_INCLUDE_DIR}
  ${BOOTLOADER_INCLUDE_DIR}
  ${CHIBIOS_ROOT}/os/hal/include
  ${CHIBIOS_ROOT}/os/rt/include
  ${CHIBIOS_CONTRIB_ROOT}/os/hal/include
)

add_definitions(-DCHPRINTF_USE_FLOAT=1 -DCORE_CM7 -DCORTEX_USE_FPU=FALSE)

# --- build-time version header (repo VERSION file + git state) ---
get_filename_component(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
set(VERSION_GEN_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${VERSION_GEN_DIR})
add_custom_target(version_gen ALL
  BYPRODUCTS ${VERSION_GEN_DIR}/version_gen.h
  COMMAND ${CMAKE_COMMAND}
    -DVERSION_FILE=${CMAKE_CURRENT_SOURCE_DIR}/version.yml
    -DTEMPLATE=${REPO_ROOT}/cmake/version.h.in
    -DOUTPUT=${VERSION_GEN_DIR}/version_gen.h
    -DCOMPONENT=bootloader
    -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -P ${REPO_ROOT}/cmake/version.cmake
  COMMENT "Generating version_gen.h (bootloader)")
include_directories(${VERSION_GEN_DIR})

set(SOURCES
    ./bl_main.c
    ./bl_update.c
    ./bl_stage.c
    ./memmap.c
    ${BOOTLOADER_SRC_DIR}/crc32.c
    ${BOOTLOADER_SRC_DIR}/frame.c
    ${BOOTLOADER_SRC_DIR}/image.c

    # minimal init only - NO bsp.c (it runs the full device registry). just the
    # debug console; UART4 + QSPI are brought up in bl_main. TODO: a small
    # bl_bsp.c for the QSPI/UART4 pin config (lift the QUADSPI AF9 pins from
    # apm/bsp/configs/bsp_spi_config.c).
    ${APM_DIR}/bsp/utils/bsp_io.c
    ${APM_DIR}/bsp/configs/bsp_uart_config.c

    # QSPI: the app slots + fpga bitstream (existing driver, used directly)
    ${DRIVERS_SRC_DIR}/qspi_memmap.c
    ${DRIVERS_SRC_DIR}/w25qxx.c

    # internal-flash erase/program: stage the chosen app into bank 2 exec region
    ${DRIVERS_SRC_DIR}/stm32h7_flash.c
)

add_executable(${PROJECT_NAME}.elf ${SOURCES})
add_dependencies(${PROJECT_NAME}.elf version_gen)

target_link_options(
  ${PROJECT_NAME}.elf
  PRIVATE
  -nostartfiles
  -Wl,-Map=${CMAKE_BINARY_DIR}/bootloader.map,--cref,--no-warn-mismatch
  -Wl,--gc-sections
  -Wl,--library-path=../${TOOLCHAIN}/ld
  -T../${TOOLCHAIN}/ld/STM32H755xI_M7.ld
  -Wl,--defsym=__process_stack_size__=0x400
  -Wl,--defsym=__main_stack_size__=0x400)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE m chibios_stm32h7)
# PRIVATE, not PUBLIC: the library needs apm/bsp for its own config, but must NOT
# propagate it to bl_main - that would shadow our halconf_community.h override
# and re-enable USBH in the executable's translation units.
target_include_directories(chibios_stm32h7 PRIVATE ${APM_DIR}/bsp)

stm32_add_hex_bin_targets(${CMAKE_PROJECT_NAME}.elf)
stm32_print_size_of_targets(${CMAKE_PROJECT_NAME}.elf)

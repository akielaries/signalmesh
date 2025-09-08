# -----------------------------------------------------------------------------
#  gcc_stm32h7.cmake
#
#  Toolchain settings for STM32H7 family (e.g., STM32H755 CM7 core)
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Compiler / Assembler / Linker Flags
# -----------------------------------------------------------------------------
set(CMAKE_C_FLAGS "-ggdb -mthumb -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard \
                   -Wall -std=gnu99 -ffunction-sections -fdata-sections \
                   -fomit-frame-pointer -mabi=aapcs -fno-unroll-loops \
                   -ffast-math -ftree-vectorize"
    CACHE INTERNAL "c compiler flags")

set(CMAKE_CXX_FLAGS "-mthumb -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard \
                     -Wall -std=c++17 -ffunction-sections -fdata-sections \
                     -fomit-frame-pointer -mabi=aapcs -fno-unroll-loops \
                     -ffast-math -ftree-vectorize"
    CACHE INTERNAL "cxx compiler flags")

set(CMAKE_ASM_FLAGS "-mthumb -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard \
                     -x assembler-with-cpp"
    CACHE INTERNAL "asm compiler flags")

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -mthumb -mcpu=cortex-m7 \
                            -mfpu=fpv5-d16 -mfloat-abi=hard -mabi=aapcs"
    CACHE INTERNAL "executable linker flags")

set(CMAKE_MODULE_LINKER_FLAGS "-mthumb -mcpu=cortex-m7 -mfpu=fpv5-d16 \
                               -mfloat-abi=hard -mabi=aapcs"
    CACHE INTERNAL "module linker flags")

set(CMAKE_SHARED_LINKER_FLAGS "-mthumb -mcpu=cortex-m7 -mfpu=fpv5-d16 \
                               -mfloat-abi=hard -mabi=aapcs"
    CACHE INTERNAL "shared linker flags")

# -----------------------------------------------------------------------------
# Chip Type Handling
# -----------------------------------------------------------------------------
set(STM32_CHIP_TYPES
    750xx 753xx 755xx 765xx 767xx 769xx 7A3xx 7B3xx 7B0xx 7C3xx
    CACHE INTERNAL "stm32h7 chip types")

set(STM32_CODES
    "750.." "753.." "755.." "765.." "767.." "769.." "7A3.." "7B3.." "7B0.." "7C3..")

# Get chip type (e.g., STM32H755xx -> 755xx)
macro(STM32_GET_CHIP_TYPE CHIP CHIP_TYPE)
    string(REGEX REPLACE "^[sS][tT][mM]32[hH](7[0-9A-C][0-9][0-9][xX][xX]).*$" "\\1" STM32_CODE ${CHIP})
    set(INDEX 0)
    foreach(C_TYPE ${STM32_CHIP_TYPES})
        list(GET STM32_CODES ${INDEX} CHIP_TYPE_REGEXP)
        if(STM32_CODE MATCHES ${CHIP_TYPE_REGEXP})
            set(RESULT_TYPE ${C_TYPE})
        endif()
        math(EXPR INDEX "${INDEX}+1")
    endforeach()
    set(${CHIP_TYPE} ${RESULT_TYPE})
endmacro()

# -----------------------------------------------------------------------------
# Flash / RAM sizes (extend this table as needed)
# -----------------------------------------------------------------------------
macro(STM32_GET_CHIP_PARAMETERS CHIP FLASH_SIZE RAM_SIZE)
    if(${CHIP} STREQUAL "STM32H755xx")
        set(${FLASH_SIZE} "2048K")
        set(${RAM_SIZE}   "1024K")
    else()
        message(FATAL_ERROR "Unknown or unsupported STM32H7 chip: ${CHIP}")
    endif()
endmacro()

# -----------------------------------------------------------------------------
# Add preprocessor defines
# -----------------------------------------------------------------------------
function(STM32_SET_CHIP_DEFINITIONS TARGET CHIP_TYPE)
    get_target_property(TARGET_DEFS ${TARGET} COMPILE_DEFINITIONS)
    if(TARGET_DEFS)
        set(TARGET_DEFS "STM32H7;STM32H${CHIP_TYPE};${TARGET_DEFS}")
    else()
        set(TARGET_DEFS "STM32H7;STM32H${CHIP_TYPE}")
    endif()
    set_target_properties(${TARGET} PROPERTIES COMPILE_DEFINITIONS "${TARGET_DEFS}")
endfunction()


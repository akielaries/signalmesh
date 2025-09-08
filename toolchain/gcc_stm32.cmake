set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

# Function to generate .hex and .bin from ELF
function(STM32_ADD_HEX_BIN_TARGETS TARGET)
    if(EXECUTABLE_OUTPUT_PATH)
        set(FILENAME "${EXECUTABLE_OUTPUT_PATH}/${TARGET}")
    else()
        set(FILENAME "${TARGET}")
    endif()

    add_custom_target(${TARGET}.hex
        DEPENDS ${TARGET}
        COMMAND ${CMAKE_OBJCOPY} -Oihex ${FILENAME} ${FILENAME}.hex
        COMMENT "Building ${FILENAME}.hex"
    )

    add_custom_target(${TARGET}.bin
        DEPENDS ${TARGET}
        COMMAND ${CMAKE_OBJCOPY} -Obinary ${FILENAME} ${FILENAME}.bin
        COMMENT "Building ${FILENAME}.bin"
    )
endfunction()

# Function to print ELF size after build
function(STM32_PRINT_SIZE_OF_TARGETS TARGET)
    if(EXECUTABLE_OUTPUT_PATH)
        set(FILENAME "${EXECUTABLE_OUTPUT_PATH}/${TARGET}")
    else()
        set(FILENAME "${TARGET}")
    endif()

    add_custom_command(TARGET ${TARGET}
        POST_BUILD
        COMMAND ${CMAKE_SIZE} ${FILENAME}
        COMMENT "Size of ${FILENAME}"
    )
endfunction()


# cmake/chibios_rt.cmake
# Define a function to create a ChibiOS RT library from os/rt/src

if(NOT DEFINED CHIBIOS_ROOT)
  set(CHIBIOS_ROOT "${CMAKE_SOURCE_DIR}/../../import/ChibiOS" CACHE PATH "Path to ChibiOS sources")
endif()

function(add_chibios_rt_library target)
  cmake_parse_arguments(ARG "" "CHIBIOS_ROOT;TYPE" "" ${ARGN})

  if(ARG_CHIBIOS_ROOT)
    set(_chibios_root "${ARG_CHIBIOS_ROOT}")
  else()
    set(_chibios_root "${CHIBIOS_ROOT}")
  endif()

  if(NOT ARG_TYPE)
    set(ARG_TYPE "STATIC")
  endif()

  # Automatically gather all .c sources in os/rt/src
  file(GLOB _rt_sources "${_chibios_root}/os/rt/src/*.c")

  if(ARG_TYPE STREQUAL "OBJECT")
    add_library(${target} OBJECT ${_rt_sources})
  else()
    add_library(${target} STATIC ${_rt_sources})
  endif()

  target_include_directories(${target} PUBLIC
    "${_chibios_root}/os/rt/include"
    "${_chibios_root}/os/oslib/include"
    "${_chibios_root}/os/common/oop/include"
    "${_chibios_root}/os/common/ports/ARMv7-M"
    "${_chibios_root}/os/common/startup/ARMCMx/compilers/GCC"
  )

  set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endfunction()


if(NOT DEFINED CHIBIOS_ROOT)
  set(CHIBIOS_ROOT "${CMAKE_SOURCE_DIR}/../../import/ChibiOS" CACHE PATH "Path to ChibiOS sources")
endif()

function(add_chibios_common_library target)
  cmake_parse_arguments(ARG "" "CHIBIOS_ROOT;TYPE" "" ${ARGN})

  if(ARG_CHIBIOS_ROOT)
    set(_chibios_root "${ARG_CHIBIOS_ROOT}")
  else()
    set(_chibios_root "${CHIBIOS_ROOT}")
  endif()

  if(NOT ARG_TYPE)
    set(ARG_TYPE "STATIC")
  endif()

  file(GLOB _common_sources
    "${_chibios_root}/os/rt/src/*.c"
    "${_chibios_root}/os/hal/src/*.c"
    "${_chibios_root}/os/oslib/src/*.c"
    "${_chibios_root}/os/hal/lib/streams/*.c"
    "${_chibios_root}/os/common/oop/src/oop_base_object.c"
  )

  if(ARG_TYPE STREQUAL "OBJECT")
    add_library(${target} OBJECT ${_common_sources})
  else()
    add_library(${target} STATIC ${_common_sources})
  endif()

  target_include_directories(${target} PUBLIC
    "${_chibios_root}/os/common/portability/GCC"
    "${_chibios_root}/os/common/ports/ARM-common/include"
    #"${_chibios_root}/os/hal/include/ports/ARM"
    "${_chibios_root}/os/common/ports/ARMv7-M"
    #"${_chibios_root}/os/hal/ports/common/ARMCMx"
    "${_chibios_root}/os/common/oop/include"
    "${_chibios_root}/os/hal/lib/streams"
    "${_chibios_root}/os/hal/include"
    "${_chibios_root}/os/hal/lib/include"
    "${_chibios_root}/os/hal/osal/rt-nil"
    #"${_chibios_root}/os/hal/osal/include"
    "${_chibios_root}/os/rt/include"
    "${_chibios_root}/os/oslib/include"
    "${_chibios_root}/os/license"
  )
  set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE OFF)
endfunction()


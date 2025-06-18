# sources
set(AAM_SRC
    src/sine.c
    src/voice.c
    src/dma_ex.c
    cfg/portab.c
)

# public headers for other modules
set(AAM_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/cfg
)

# create a library
add_library(aam STATIC ${AAM_SRC})
target_include_directories(aam PUBLIC ${AAM_INCLUDE_DIRS})

# target options
target_compile_definitions(aam PRIVATE -DCORE_CM7)
target_compile_options(aam PRIVATE -O2 -ggdb -fomit-frame-pointer)


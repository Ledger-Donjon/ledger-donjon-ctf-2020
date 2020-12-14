set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_SYSTEM_NAME Generic)

# Avoid known bug in linux giving:
# arm-none-eabi-gcc: error: unrecognized command line option '-rdynamic'
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(CMAKE_C_FLAGS "-nostdlib ${CMAKE_C_FLAGS}")

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-as)

# Common flags for Cortex-M3
set(CMAKE_C_FLAGS "-mthumb -mcpu=cortex-m3 -Os ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-fno-exceptions ${CMAKE_C_FLAGS}")
set(CMAKE_ASM_FLAGS "-mthumb -mcpu=cortex-m3")
SET(CMAKE_EXE_LINKER_FLAGS "-mthumb -mcpu=cortex-m3" CACHE INTERNAL "executable linker flags")

# Debug and release flags. Enable lto by default on release builds.
set(CMAKE_C_FLAGS_DEBUG "-Og -g" CACHE INTERNAL "c compiler flags debug")
set(CMAKE_ASM_FLAGS_DEBUG "-g" CACHE INTERNAL "asm compiler flags debug")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "" CACHE INTERNAL "linker flags debug")

set(CMAKE_C_FLAGS_RELEASE "-O3 -flto" CACHE INTERNAL "c compiler flags release")
set(CMAKE_ASM_FLAGS_RELEASE "" CACHE INTERNAL "asm compiler flags release")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-flto" CACHE INTERNAL "linker flags release")

set(CMAKE_OBJCOPY "arm-none-eabi-objcopy" CACHE INTERNAL "objcopy tool")

function(stm32_add_bin_target TARGET)
    add_custom_target(${TARGET}.bin ALL DEPENDS ${TARGET} COMMAND ${CMAKE_OBJCOPY} -Obinary ${TARGET} ${TARGET}.bin)
endfunction()

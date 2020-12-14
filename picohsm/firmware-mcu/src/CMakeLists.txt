cmake_minimum_required(VERSION 2.8)

project("picohsm-firmware-mcu")

include("gcc-stm32.cmake")

enable_language(ASM)

set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/linker_script ${CMAKE_EXE_LINKER_FLAGS}")

add_executable(firmware-mcu main.cxx usart.cxx w5500.cxx delay.cxx panic.cxx
    util.cxx boot.s)
stm32_add_bin_target(firmware-mcu)

add_executable(firmware-mcu-ctf main.cxx usart.cxx w5500.cxx delay.cxx panic.cxx
    util.cxx boot.s)
target_compile_options(firmware-mcu-ctf PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:-DHIDE_SECRETS>")

# reset connected to DTR
# boot0 connected to RTS
# DTR and RTS logic is complemented
add_custom_target(flash
    COMMAND stm32flash -R -i -rts,dtr,-dtr:rts,dtr,-dtr -w firmware-mcu.bin -v -g 0x0 /dev/ttyUSB0
    DEPENDS firmware-mcu.bin
)

function(patch_ipmac NUM)
    add_custom_target(firmware-mcu-ip${NUM}.bin
        ALL DEPENDS firmware-mcu.bin
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/patch.py ipmac 192.168.60.${NUM} 0.8.220.1.2.${NUM} firmware-mcu firmware-mcu.bin firmware-mcu-ip${NUM}.bin
        BYPRODUCTS firmware-mcu-ip${BUM}.bin
    )
endfunction()

function(flashn NUM)
    add_custom_target(flash${NUM}
        DEPENDS firmware-mcu-ip${NUM}.bin
        COMMAND stm32flash -R -i -rts,dtr,-dtr:rts,dtr,-dtr -w firmware-mcu-ip${NUM}.bin -v -g 0x0 /dev/ttyUSB0
    )
endfunction()

foreach(num RANGE 11 20 1)
    patch_ipmac(${num})
    flashn(${num})
endforeach(num)


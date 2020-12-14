cmake_minimum_required(VERSION 2.8)

project("picohsm-firmware-sec")

set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER "avr-gcc")
set(CMAKE_CXX_COMPILER "avr-g++")
set(CMAKE_OBJCOPY "avr-objcopy")
set(DFP "~/bin/Atmel.ATmega_DFP.1.2.272.atpack_FILES")
set(CMAKE_C_FLAGS "-mmcu=atmega1284p -B ${DFP}/gcc/dev/atmega1284p -I ${DFP}/include/ -D F_CPU=10000000")
set(CMAKE_CXX_FLAGS "-mmcu=atmega1284p -B ${DFP}/gcc/dev/atmega1284p -I ${DFP}/include/ -D F_CPU=10000000")
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
set(AVRDUDE_FLAGS -c atmelice_isp -p ATMega1284p -B 15)

add_executable(firmware main.cxx uart.cxx aes.c)
add_custom_target(firmware.hex ALL DEPENDS firmware COMMAND ${CMAKE_OBJCOPY} -Oihex firmware firmware.hex)
add_custom_target(flash
    COMMAND avrdude ${AVRDUDE_FLAGS} -v -e -U flash:w:firmware.hex
    DEPENDS firmware.hex
)
add_custom_target(fuses
    COMMAND avrdude ${AVRDUDE_FLAGS} -v -U lfuse:w:0xe0:m
)

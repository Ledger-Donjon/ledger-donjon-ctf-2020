CC=arm-none-eabi-gcc
AS=arm-none-eabi-as
OBJCOPY=arm-none-eabi-objcopy
LD=arm-none-eabi-ld
CFLAGS=-mthumb -mcpu=cortex-m3
LDFLAGS=-nostdlib -T script.ld

all:
	python3 help.py
	$(CC) $(CFLAGS) -c shell-getflag.c -o shell-getflag.o
	$(CC) $(CFLAGS) -c shell-pin.c -o shell-pin.o
	$(CC) $(CFLAGS) -c shell-fault.c -o shell-fault.o
	$(AS) $(CFLAGS) -c shell.s -o shell.o
	$(LD) shell.o shell-getflag.o $(LDFLAGS) -o shell-getflag.elf
	$(LD) shell.o shell-pin.o $(LDFLAGS) -o shell-pin.elf
	$(LD) shell.o shell-fault.o $(LDFLAGS) -o shell-fault.elf
	$(OBJCOPY) -Obinary shell-getflag.elf shell-getflag.bin
	$(OBJCOPY) -Obinary shell-pin.elf shell-pin.bin
	$(OBJCOPY) -Obinary shell-fault.elf shell-fault.bin


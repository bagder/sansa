
#
# A trivial makefile for building e200tool on Linux/Unix
# This makefile assumes that arm-elf package is installed
# Original file by MrH
# Extended by Virtuoso015

CROSS= arm-elf-


e200tool: e200tool.c e200_protocol.h e200_code.c
	$(CC) -o e200tool e200tool.c -lusb

e200_code.c : bin2c arm_code.bin
	./bin2c arm_code.bin e200_code
    
bin2c : bin2c.c
	$(CC) -o bin2c bin2c.c

arm_code.bin: arm_code.out
	$(CROSS)objcopy -Obinary arm_code.out arm_code.bin

arm_code.out : arm_code.o 
	$(CROSS)ld  -L/usr/local/arm-elf/lib/gcc/arm-elf/4.0.3/arm7tdmi/ -Ttext 0x40004000 -N arm_code.o -lgcc -o arm_code.out 


arm_code.o : arm_code.c
	$(CROSS)gcc -c -Os -fno-unit-at-a-time -mcpu=arm7tdmi -mtune=arm7tdmi arm_code.c 
	
clean :
	rm -f arm_code.o arm_code.out arm_code.bin bin2c e200_code.c e200_code.h e200tool

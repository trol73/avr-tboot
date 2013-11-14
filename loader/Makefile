AS=avr-as
CPP=avr-cpp
CC=avr-gcc
LD=avr-ld
CFLAGS=-mmcu=$(MCU) $(DEFS)
ASFLAGS=-mmcu=$(MCU) # -Wa,-al

MCU=atmega32
# DEFS+=-DCHECK_PORTD_4

all: tboot.hex 

clean:
	rm -f tboot.hex 
	rm -f tboot.o 
	rm -r tboot.hex.map

tboot.o : tboot.S
	$(CC) $(CFLAGS) -c -o $@ $^


# Gnu ld works in terms of bytes/octets, instead of the 16-bit words
# which the AVR datasheet specifies, so these values are double the
# values on page 220 of Atmel's doc2486 (2486P-AVR-02/06).
# tboot.hex:  BOOTSTART=0x1F00
tboot.hex:  BOOTSTART=0x7E00

%.hex: %.o
#	$(LD) -Map $@.map --defsym BOOTSTART="$(BOOTSTART)" -T avr4-bootloader-8k.x $^ -o tboot.elf
	$(LD) -Map $@.map --defsym BOOTSTART="$(BOOTSTART)" -T avr4-bootloader-32k.x $^ -o tboot.elf
#	avr-objcopy tboot.elf -O ihex tboot.hex.tmp
	avr-objcopy tboot.elf -O ihex tboot.hex
#	egrep -v ':10....00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF..' tboot.hex.tmp 
#	rm tboot.hex.tmp


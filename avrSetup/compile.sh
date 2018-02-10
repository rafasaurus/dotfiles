avr-gcc -g -Os -mmcu=atmega2560 -c led.c
avr-gcc -g -mmcu=atmega2560 -o led.elf led.o
avr-objcopy -j .text -j .data -O ihex led.elf led.hex
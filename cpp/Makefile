all:
	avr-g++ -Os -DF_CPU=16000000 -ffreestanding -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wextra -mmcu=atmega328p main.cpp -o main.elf
	avr-size --mcu=atmega328p -C main.elf


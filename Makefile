DEVICE     = atmega328p
SERIAL     = /dev/ttyACM0
NAME       = serial_keyboard
OBJECTS    = $(NAME).o

COMPILE = avr-gcc -Wall -Os -mmcu=$(DEVICE)
 
all: $(NAME).hex
 
.c.o:
	$(COMPILE) -c $< -o $@
 
.S.o:
	$(COMPILE) -c $< -o $@

.c.s:
	$(COMPILE) -S $< -o $@

flash:  all
	avrdude -V -F -c arduino -p ATMEGA328P -b 115200 -P $(SERIAL) -U flash:w:$(NAME).hex

clean:
	rm -f $(NAME).hex $(NAME).elf $(OBJECTS)
 
$(NAME).elf: $(OBJECTS)
	$(COMPILE) -o $(NAME).elf $(OBJECTS)
 
$(NAME).hex: $(NAME).elf
	rm -f $(NAME).hex
	avr-objcopy -O ihex -R .eeprom $(NAME).elf $(NAME).hex
 

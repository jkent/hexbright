TARGET=hexbright
TARGET_MCU=atmega168p
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%.o,$(SRC))

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump

CFLAGS = -g -Wall -O2 -mmcu=$(TARGET_MCU) -DF_CPU=8000000L
LDFLAGS = -g -mmcu=$(TARGET_MCU) -Wl,-Map,$(TARGET).map

ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

.PHONY: all flash
all: $(TARGET).elf $(TARGET).hex #$(TARGET)_eeprom.hex

flash: $(TARGET).hex #$(TARGET)_eeprom.hex
	$(Q) avrdude -p $(TARGET_MCU) -c arduino -P /dev/ttyUSB0 -b 19200 -U flash:w:$(TARGET).hex 
	$(Q) #avrdude -p $(TARGET_MCU) -c arduino -P /dev/ttyUSB0 -b 19200 -U eeprom:w:$(TARGET)_eeprom.hex

%.o: %.c
	$(vecho) "CC $<"
	$(Q) $(CC) $(CFLAGS) -c -o $@ $<

$(TARGET).elf: $(OBJ)
	$(vecho) "LD $@"
	$(Q) $(CC) $(LDFLAGS) -o $@ $^
	$(vecho) "OBJDUMP $(patsubst %.elf,%.lst,$@)"
	$(Q) $(OBJDUMP) -h -S $< > $(patsubst %.elf,%.lst,$@)

clean:
	$(Q) rm -f *.o $(TARGET).elf
	$(Q) rm -rf *.lst *.hex *.map

%.hex: %.elf
	$(vecho) "OBJCOPY $@"
	$(Q) $(OBJCOPY) -j .text -j .data -O ihex $< $@

%_eeprom.hex: %.elf
	$(vecho) "OBJCOPY $@"
	$(Q) $(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@


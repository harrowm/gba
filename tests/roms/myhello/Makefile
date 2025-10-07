# Simple GBA test ROM build
# Requires: arm-none-eabi-gcc (from devkitARM or ARM toolchain)

AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

TARGET = myhello
SOURCES = $(TARGET).s
LDSCRIPT = $(TARGET).ld

all: $(TARGET).gba

$(TARGET).o: $(SOURCES)
	$(AS) -mthumb-interwork -mcpu=arm7tdmi $(SOURCES) -o $(TARGET).o

$(TARGET).elf: $(TARGET).o $(LDSCRIPT)
	$(LD) -T $(LDSCRIPT) $(TARGET).o -o $(TARGET).elf

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $(TARGET).elf $(TARGET).gba
	python3 fix_header.py $(TARGET).gba
	@echo "Built $(TARGET).gba"

clean:
	rm -f $(TARGET).o $(TARGET).elf $(TARGET).gba

.PHONY: all clean

# Minimal GBA Test ROM Makefile

# Tools
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
FIXHEADER = python3 ../assets/fix_header.py

# Flags
ASFLAGS = -mthumb-interwork -mcpu=arm7tdmi
LDFLAGS = -T linker.ld

# Targets
TARGETS = minimal_display_test simple_red_test
ROMS = $(TARGETS:=.gba)

.PHONY: all clean

all: $(ROMS)

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.elf: %.o
	$(LD) $(LDFLAGS) -o $@ $<

%.gba: %.elf
	$(OBJCOPY) -O binary $< $@
	$(FIXHEADER) $@

clean:
	rm -f $(TARGETS:=.o) $(TARGETS:=.elf) $(ROMS)

run: $(ROM)
	@echo "Running $(ROM) with 3 second timeout..."
	timeout 3 ../gba_emulator $(ROM) || true

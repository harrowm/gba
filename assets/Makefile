# Makefile for simple GBA test ROM

# Toolchain
PREFIX = arm-none-eabi-
AS = $(PREFIX)as
LD = $(PREFIX)ld
OBJCOPY = $(PREFIX)objcopy

# Flags
ASFLAGS = -mthumb-interwork
LDFLAGS = -T test_pixels.ld

# Files
SRC = test_pixels.s
OBJ = test_pixels.o
ELF = test_pixels.elf
ROM = test_pixels.gba

.PHONY: all clean

all: $(ROM)

$(OBJ): $(SRC)
	$(AS) $(ASFLAGS) $(SRC) -o $(OBJ)

$(ELF): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $(ELF)

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $(ELF) $(ROM)
	@echo "ROM built: $(ROM)"
	@python3 fix_header.py $(ROM)
	@ls -lh $(ROM)

clean:
	rm -f $(OBJ) $(ELF) $(ROM)

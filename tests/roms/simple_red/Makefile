# Makefile for simple_red_test ROM

AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
PYTHON = python3

ASFLAGS = -mthumb-interwork -mcpu=arm7tdmi
LDFLAGS = -T simple_red_test.ld

TARGET = simple_red_test
SRC = $(TARGET).s
OBJ = $(TARGET).o
ELF = $(TARGET).elf
ROM = $(TARGET).gba

.PHONY: all clean

all: $(ROM)
	@echo "Built $(ROM)"

$(OBJ): $(SRC)
	$(AS) $(ASFLAGS) $(SRC) -o $(OBJ)

$(ELF): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $(ELF)

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $(ELF) $(ROM)
	$(PYTHON) fix_header.py $(ROM)

clean:
	rm -f $(OBJ) $(ELF) $(ROM)

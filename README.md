# gba
My attempt at a GameBoy Advance emulator - wip

# Game Boy Advance Emulator

This project is a high-performance, cycle-accurate emulator for the Game Boy Advance.

## Features
- ARM7TDMI CPU emulation
- GPU rendering
- Memory management
- Interrupt handling

## Directory Structure
- `src/`: Source code files
- `include/`: Header files
- `assets/`: Emulator assets
- `tests/`: Unit and integration tests
- `docs/`: Documentation

## Getting Started
To build and run the emulator, compile the source files and execute `main.cpp`.


Useful references:

https://github.com/matanlurey/armv4t.dart?tab=readme-ov-file


Some tests here:
https://github.com/daniel5151/armv4t_emu/tree/master/tests


The tests use Google Test, installed with:
brew install googletest
brew install gperftools


SETTING THE C FLAG:
Yes, you can use the same logic (R[rd] > (UINT32_MAX - imm)) to test whether the C (carry) flag should be set for all Thumb instructions that involve unsigned addition. However, the applicability depends on the specific instruction and its behavior. Here's a breakdown:

Instructions That Set the C Flag
ADD (Immediate):

The C flag is set if the addition results in an unsigned overflow.
The logic R[rd] > (UINT32_MAX - imm) works perfectly here.
ADC (Add with Carry):

The C flag is set if the addition (including the carry-in) results in an unsigned overflow.
You would need to include the carry-in value in the calculation:
SUB (Immediate):

The C flag is set if no borrow occurs (i.e., the subtraction does not go below zero).
For subtraction, the logic would be:
SBC (Subtract with Carry):

The C flag is set if no borrow occurs, considering the carry-in.
The logic would be:
CMP (Compare):

This is essentially a subtraction that updates the flags but does not store the result.
The same logic as SUB applies:
Generalization
For all instructions that involve addition or subtraction and need to set the C flag:

Use R[rd] > (UINT32_MAX - imm) for unsigned addition.
Use R[rd] >= imm for unsigned subtraction.
Caveats
Ensure that the instruction actually updates the C flag. Some instructions in Thumb mode do not update flags.
For instructions like ADC and SBC, include the carry-in value in the calculation.



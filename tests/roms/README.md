# GBA Test ROMs

This directory contains organized test ROMs for the GBA emulator.

## Directory Structure

```
tests/
├── roms/                          # Organized test ROMs (each in own directory)
│   ├── myhello/                   # Mode 0 test - 4 colored tiles (white, red, green, blue)
│   │   ├── Makefile
│   │   ├── myhello.s              # Assembly source
│   │   ├── myhello.ld             # Linker script
│   │   ├── fix_header.py          # GBA header checksum fixer
│   │   └── myhello.gba            # Built ROM
│   │
│   └── simple_red/                # Simple red screen test
│       ├── Makefile
│       ├── simple_red_test.s      # Assembly source
│       ├── simple_red_test.ld     # Linker script
│       ├── fix_header.py          # GBA header checksum fixer
│       └── simple_red_test.gba    # Built ROM
│
└── minimal_display_test.s         # (Legacy - still in main tests dir)

```

## Building ROMs

Each ROM directory has its own Makefile:

```bash
# Build myhello
cd roms/myhello
make

# Build simple_red
cd roms/simple_red
make

# Clean build artifacts
make clean
```

## Running ROMs

From the main GBA directory:

```bash
# Run myhello test
./gba_emulator --skip-bios tests/roms/myhello/myhello.gba

# Run simple_red test
./gba_emulator --skip-bios tests/roms/simple_red/simple_red_test.gba
```

## Test Descriptions

### myhello
- **Purpose**: Tests Mode 0 tiled background rendering
- **Display**: 4 colored 8x8 tiles at row 10, column 8
- **Colors**: White, Red, Green, Blue (using 8bpp palette mode)
- **Features**: Palette RAM, VRAM tile data, screen map entries

### simple_red
- **Purpose**: Basic display test with solid red screen
- **Display**: Full screen red color
- **Features**: Basic DISPCNT and palette setup

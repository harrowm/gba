#!/bin/bash

# Script to add palette/tile logging to mGBA

MGBA_DIR="/Users/malcolm/mgba-instrumented"
SOURCE_FILE="$MGBA_DIR/src/gba/renderers/video-software.c"
BACKUP_FILE="$SOURCE_FILE.backup_palette"

echo "Adding palette/tile logging to mGBA..."

# Backup original file if not already backed up
if [ ! -f "$BACKUP_FILE" ]; then
    echo "Creating backup at $BACKUP_FILE"
    cp "$SOURCE_FILE" "$BACKUP_FILE"
else
    echo "Backup already exists, restoring from backup first"
    cp "$BACKUP_FILE" "$SOURCE_FILE"
fi

# Find the line number where we need to insert code (right after the "softwareRenderer->nextY = y + 1;" line)
LINE_NUM=$(grep -n "softwareRenderer->nextY = y + 1" "$SOURCE_FILE" | head -1 | sed 's/:.*//')

if [ -z "$LINE_NUM" ]; then
    echo "ERROR: Could not find insertion point in video-software.c"
    exit 1
fi

echo "Found insertion point at line $LINE_NUM"

# Create the new file with logging code inserted
{
    # Print lines before insertion point
    head -n "$LINE_NUM" "$SOURCE_FILE"
    
    # Insert logging code
    cat << 'LOGGING_CODE'

	// === DEBUG: Log OBJ Palette 0 and Tile data at specific frames ===
	static int frameCount = 0;
	static int lastLoggedFrame = -1;
	if (y == 0 && frameCount != lastLoggedFrame) {
		lastLoggedFrame = frameCount;
		if (frameCount >= 100 && frameCount <= 150 && frameCount % 10 == 0) {
			printf("[MGBA PALETTE/TILE FRAME %d]\n", frameCount);
			
			// Log OBJ Palette 0 (first 16 colors)
			printf("  OBJ Palette 0: ");
			uint16_t* objPalette = (uint16_t*) &softwareRenderer->d.palette[256]; // OBJ palette starts at color 256
			for (int i = 0; i < 16; i++) {
				printf("%04X ", objPalette[i]);
			}
			printf("\n");
			
			// Log Tile 0 checksum (first 32 bytes = 8x8 4bpp tile)
			uint32_t tile0Checksum = 0;
			uint8_t* vram = (uint8_t*)softwareRenderer->d.vram;
			uint32_t tile0Offset = 0x10000; // OBJ tiles start at 0x06010000 = offset 0x10000 in VRAM
			for (int i = 0; i < 32; i++) {
				tile0Checksum += vram[tile0Offset + i];
			}
			printf("  Tile 0 checksum: 0x%08X\n", tile0Checksum);
			
			// Log first 8 bytes of Tile 0 for visual inspection
			printf("  Tile 0 first 8 bytes: ");
			for (int i = 0; i < 8; i++) {
				printf("%02X ", vram[tile0Offset + i]);
			}
			printf("\n");
		}
		frameCount++;
	}
LOGGING_CODE
    
    # Print remaining lines after insertion point
    tail -n +$((LINE_NUM + 1)) "$SOURCE_FILE"
} > "$SOURCE_FILE.new"

# Replace original with modified version
mv "$SOURCE_FILE.new" "$SOURCE_FILE"

echo "Logging code added successfully!"
echo ""
echo "Now rebuild mGBA with:"
echo "  cd $MGBA_DIR/build"
echo "  make -j8"
echo ""
echo "Then run mGBA and your emulator with the BIOS:"
echo "  $MGBA_DIR/build/mips/mgba -b assets/bios.bin > /tmp/mgba_output.txt 2>&1"
echo "  ./gba_emulator > /tmp/gba_output.txt 2>&1"
echo ""
echo "Compare outputs with:"
echo "  grep 'PALETTE/TILE' /tmp/mgba_output.txt"
echo "  grep 'PALETTE/TILE' /tmp/gba_output.txt"

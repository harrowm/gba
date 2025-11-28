#!/usr/bin/env python3
"""
Compare palette and tile data between mGBA and our emulator
"""

import sys
import re

def parse_log_file(filename, prefix):
    """Parse a log file and extract palette/tile data by frame"""
    frames = {}
    
    with open(filename, 'r') as f:
        current_frame = None
        for line in f:
            # Match frame header
            frame_match = re.match(r'\[.*PALETTE/TILE FRAME (\d+)\]', line)
            if frame_match:
                current_frame = int(frame_match.group(1))
                frames[current_frame] = {'palette': [], 'checksum': None, 'bytes': []}
                continue
            
            if current_frame is None:
                continue
            
            # Match palette data
            if 'OBJ Palette 0:' in line:
                palette_hex = line.split('OBJ Palette 0:')[1].strip().split()
                frames[current_frame]['palette'] = palette_hex
            
            # Match checksum
            if 'Tile 0 checksum:' in line:
                checksum = line.split('Tile 0 checksum:')[1].strip()
                frames[current_frame]['checksum'] = checksum
            
            # Match first 8 bytes
            if 'Tile 0 first 8 bytes:' in line:
                bytes_hex = line.split('Tile 0 first 8 bytes:')[1].strip().split()
                frames[current_frame]['bytes'] = bytes_hex
    
    return frames

def compare_data(mgba_data, gba_data):
    """Compare data between two emulators"""
    all_frames = sorted(set(mgba_data.keys()) | set(gba_data.keys()))
    
    differences_found = False
    
    for frame in all_frames:
        if frame not in mgba_data:
            print(f"❌ Frame {frame}: Missing in mGBA data")
            differences_found = True
            continue
        
        if frame not in gba_data:
            print(f"❌ Frame {frame}: Missing in our emulator data")
            differences_found = True
            continue
        
        mgba = mgba_data[frame]
        gba = gba_data[frame]
        
        # Compare palette
        if mgba['palette'] != gba['palette']:
            print(f"\n❌ Frame {frame}: PALETTE MISMATCH")
            print(f"   mGBA:  {' '.join(mgba['palette'])}")
            print(f"   Ours:  {' '.join(gba['palette'])}")
            
            # Find which colors differ
            for i in range(min(len(mgba['palette']), len(gba['palette']))):
                if mgba['palette'][i] != gba['palette'][i]:
                    print(f"   Color {i}: mGBA={mgba['palette'][i]} Ours={gba['palette'][i]}")
            differences_found = True
        else:
            print(f"✅ Frame {frame}: Palette matches")
        
        # Compare checksum
        if mgba['checksum'] != gba['checksum']:
            print(f"❌ Frame {frame}: TILE 0 CHECKSUM MISMATCH")
            print(f"   mGBA:  {mgba['checksum']}")
            print(f"   Ours:  {gba['checksum']}")
            differences_found = True
        else:
            print(f"✅ Frame {frame}: Tile 0 checksum matches ({mgba['checksum']})")
        
        # Compare first 8 bytes
        if mgba['bytes'] != gba['bytes']:
            print(f"❌ Frame {frame}: TILE 0 BYTES MISMATCH")
            print(f"   mGBA:  {' '.join(mgba['bytes'])}")
            print(f"   Ours:  {' '.join(gba['bytes'])}")
            differences_found = True
    
    return not differences_found

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 compare_palette_tiles.py <mgba_log> <gba_log>")
        sys.exit(1)
    
    mgba_file = sys.argv[1]
    gba_file = sys.argv[2]
    
    print("Parsing mGBA log...")
    mgba_data = parse_log_file(mgba_file, "MGBA")
    
    print("Parsing our emulator log...")
    gba_data = parse_log_file(gba_file, "")
    
    print(f"\nFound {len(mgba_data)} frames in mGBA log")
    print(f"Found {len(gba_data)} frames in our emulator log\n")
    
    print("=" * 80)
    print("COMPARISON RESULTS")
    print("=" * 80)
    
    all_match = compare_data(mgba_data, gba_data)
    
    print("\n" + "=" * 80)
    if all_match:
        print("✅ ALL DATA MATCHES - palette and tiles are identical!")
        print("   The purple artifacts are NOT a data issue.")
        print("   Check timing/DMA/rendering order next.")
    else:
        print("❌ DIFFERENCES FOUND - see above for details")
        print("   The purple artifacts are likely due to wrong palette/tile data.")
    print("=" * 80)

if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Analyze DMA timing failures by cross-referencing test output with expected values."""

import re
import sys

# The test output uses %5li (decimal long int), but there appear to be characters
# like :;<=>? in the output. This is because the GBA's font renders digits 0-9
# starting at ASCII '0' (0x30), and the vsnprintf on GBA treats values up to
# certain sizes correctly. But the debug console output may use a compact font
# where 10-15 get rendered as :;<=>?.
#
# Actually, let's just observe: the GBA's vsnprintf outputs decimal. But the
# mGBA debug console captures raw bytes. The issue is that values > 127 or
# negative values get display correctly. The ':' = 0x3A = '0'+10 = 10.
# Wait... %li with decimal would never produce ':'. 
#
# RE-ANALYSIS: Looking at the output more carefully:
# "Got    0< vs    0=: FAIL"  — the '0<' and '0=' are TWO characters each
# In %5li format, "   0<" would be 5 chars. '0' is char, '<' is char.
# But %li produces DECIMAL digits 0-9 only.
#
# HYPOTHESIS: The output IS decimal but the debug print goes through the GBA's
# tile-based text renderer which maps each digit character. If the number has
# more digits than expected by the format, characters shift into the adjacent
# character code range. E.g., "13" might get displayed as "0=" if there's a
# base-offset issue.
#
# SIMPLER HYPOTHESIS: The output contains non-ASCII because the mGBA debug
# console encodes via mgba_printf which writes to debug registers. The
# characters :;<=>? ARE the correct ASCII representations of decimal digits
# when the value is >= 10 in a single digit position... but that's not how
# %li works.
#
# ACTUALLY: Let me look at the actual byte values:
# '0' = 48, '1' = 49, ..., '9' = 57
# ':' = 58 = 48+10
# ';' = 59 = 48+11
# '<' = 60 = 48+12
# '=' = 61 = 48+13
# '>' = 62 = 48+14
# '?' = 63 = 48+15
#
# This confirms: the GBA's printf is encoding values in some non-standard way
# where each "digit" can go beyond 9. This is HEXADECIMAL with digits represented
# as '0'+nibble, NOT the standard 0-9a-f. So '0;' = 0x0B = 11 decimal.
#
# BUT WAIT: %5li is supposed to be decimal. Unless the GBA's libc implementation
# is broken or uses a custom format. Let me just decode both ways and see which
# matches the expected values from the source.

def decode_shifted_hex(s):
    """Decode a number where each character is '0' + nibble_value (0-15)."""
    s = s.strip()
    result = 0
    for c in s:
        result = result * 16 + (ord(c) - ord('0'))
    return result

def is_pure_decimal(s):
    """Check if string contains only standard decimal digits."""
    return all(c in '0123456789' for c in s.strip())

# Expected values from mgba-emu/suite source timing.c
# Format: 10 values per test: arm_text_{0000,4000,0004,4004,0010,4010,0014,4014}, arm_ewram, arm_iwram
# Then 10 more for thumb equivalents
# Map: ... = 0x0000, P.. = 0x4000, .N. = 0x0004, PN. = 0x4004,
#       ..S = 0x0010, P.S = 0x4010, .NS = 0x0014, PNS = 0x4014

expected = {
    "Trivial DMA (16)": {
        "ARM": [13, 10, 12, 10, 12, 8, 11, 8, 11, 2],
        "Thumb": [10, 7, 9, 7, 10, 2, 9, 2, 8, 2],
    },
    "Trivial DMA (16/ROM)": {
        "ARM": [17, 14, 15, 13, 16, 13, 14, 12, 15, 2],
        "Thumb": [14, 11, 12, 10, 14, 2, 12, 2, 12, 2],
    },
    "Trivial DMA (16/to ROM)": {
        "ARM": [17, 15, 15, 14, 16, 12, 14, 11, 15, 2],
        "Thumb": [14, 12, 12, 11, 14, 2, 12, 2, 12, 2],
    },
    "Trivial DMA (16/ROM to ROM)": {
        "ARM": [19, 16, 17, 15, 17, 14, 15, 13, 17, 2],
        "Thumb": [16, 13, 14, 12, 15, 2, 13, 2, 14, 2],
    },
    "Trivial DMA (32)": {
        "ARM": [13, 10, 12, 10, 12, 8, 11, 8, 11, 2],
        "Thumb": [10, 7, 9, 7, 10, 2, 9, 2, 8, 2],
    },
    "Trivial DMA (32/from ROM)": {
        "ARM": [20, 17, 18, 16, 18, 15, 16, 14, 18, 2],
        "Thumb": [17, 14, 15, 13, 16, 2, 14, 2, 15, 2],
    },
    "Trivial DMA (32/to ROM)": {
        "ARM": [20, 18, 18, 17, 18, 14, 16, 13, 18, 2],
        "Thumb": [17, 15, 15, 14, 16, 2, 14, 2, 15, 2],
    },
    "Trivial DMA (32/ROM to ROM)": {
        "ARM": [25, 22, 23, 21, 21, 18, 19, 17, 23, 2],
        "Thumb": [22, 19, 20, 18, 19, 2, 17, 2, 20, 2],
    },
    "Short DMA (16)": {
        "ARM": [43, 40, 42, 40, 42, 38, 41, 38, 41, 2],
        "Thumb": [40, 37, 39, 37, 40, 2, 39, 2, 38, 2],
    },
    "Short DMA (16/from ROM)": {
        "ARM": [77, 74, 75, 73, 61, 58, 59, 57, 75, 2],
        "Thumb": [74, 71, 72, 70, 59, 2, 57, 2, 72, 2],
    },
    "Short DMA (16/to ROM)": {
        "ARM": [77, 75, 75, 74, 61, 57, 59, 56, 75, 2],
        "Thumb": [74, 72, 72, 71, 59, 2, 57, 2, 72, 2],
    },
    "Short DMA (16/ROM to ROM)": {
        "ARM": [109, 106, 107, 105, 77, 74, 75, 73, 107, 2],
        "Thumb": [106, 103, 104, 102, 75, 2, 73, 2, 104, 2],
    },
    "Short DMA (32)": {
        "ARM": [43, 40, 42, 40, 42, 38, 41, 38, 41, 2],
        "Thumb": [40, 37, 39, 37, 40, 2, 39, 2, 38, 2],
    },
    "Short DMA (32/from ROM)": {
        "ARM": [125, 122, 123, 121, 93, 90, 91, 89, 123, 2],
        "Thumb": [122, 119, 120, 118, 91, 2, 89, 2, 120, 2],
    },
    "Short DMA (32/to ROM)": {
        "ARM": [125, 123, 123, 122, 93, 89, 91, 88, 123, 2],
        "Thumb": [122, 120, 120, 119, 91, 2, 89, 2, 120, 2],
    },
    "Short DMA (32/ROM to ROM)": {
        "ARM": [205, 202, 203, 201, 141, 138, 139, 137, 203, 2],
        "Thumb": [202, 199, 200, 198, 139, 2, 137, 2, 200, 2],
    },
}

# Config index map
config_names = ["...", "P..", ".N.", "PN.", "..S", "P.S", ".NS", "PNS"]
config_to_idx = {name: i for i, name in enumerate(config_names)}

# Parse failures from the log
# Lines look like: [mGBA INFO] ARM/ROM P.S: Got    0< vs    0=: FAIL
# with preceding [mGBA INFO] Timing test: Trivial DMA (16/ROM) to identify which test

failures = []
current_test = None

with open("/tmp/timing_dma.txt") as f:
    for line in f:
        m = re.search(r'Timing test: (.+)', line)
        if m:
            current_test = m.group(1).strip()
            continue
        
        m = re.search(r'(ARM|Thumb)/ROM\s+([.A-Z]{3}):\s+Got\s+(\S+)\s+vs\s+(\S+):\s+FAIL', line)
        if m and current_test:
            mode = m.group(1)  # ARM or Thumb
            config = m.group(2)  # e.g. P.S
            got_str = m.group(3)
            exp_str = m.group(4)
            
            # Try to decode the values
            if is_pure_decimal(got_str):
                got_dec = int(got_str)
            else:
                got_dec = decode_shifted_hex(got_str)
            
            if is_pure_decimal(exp_str):
                exp_dec = int(exp_str)
            else:
                exp_dec = decode_shifted_hex(exp_str)
            
            # Cross-reference with expected source values
            if current_test in expected and mode in expected[current_test]:
                cfg_idx = config_to_idx.get(config, -1)
                if cfg_idx >= 0:
                    source_expected = expected[current_test][mode][cfg_idx]
                else:
                    source_expected = "?"
            else:
                source_expected = "?"
            
            delta = got_dec - exp_dec
            
            failures.append({
                'test': current_test,
                'mode': mode,
                'config': config,
                'got_raw': got_str,
                'exp_raw': exp_str,
                'got': got_dec,
                'exp': exp_dec,
                'source_exp': source_expected,
                'delta': delta,
            })

print(f"Total failures: {len(failures)}")
print()
print(f"{'Test':<35} {'Mode':<6} {'Cfg':<4} {'Got':>5} {'Exp':>5} {'Src':>5} {'Delta':>6} {'Match?'}")
print("-" * 100)

for f in failures:
    match = "YES" if f['exp'] == f['source_exp'] else f"NO({f['source_exp']})"
    print(f"{f['test']:<35} {f['mode']:<6} {f['config']:<4} {f['got']:5d} {f['exp']:5d} {str(f['source_exp']):>5} {f['delta']:+6d} {match}")

# Summary
print()
print("=== Delta Summary ===")
delta_counts = {}
for f in failures:
    d = f['delta']
    delta_counts[d] = delta_counts.get(d, 0) + 1
for d in sorted(delta_counts.keys()):
    print(f"  delta={d:+d}: {delta_counts[d]} failures")

# Check if output decoding matches source expected values
print()
print("=== Output Encoding Validation ===")
mismatches = [f for f in failures if f['exp'] != f['source_exp']]
if mismatches:
    print(f"WARNING: {len(mismatches)} cases where decoded 'expected' doesn't match source!")
    for m in mismatches:
        print(f"  {m['test']} {m['mode']}/{m['config']}: output says {m['exp']}, source says {m['source_exp']}")
else:
    print("All decoded expected values match source - encoding is correct!")

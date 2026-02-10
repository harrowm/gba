#!/usr/bin/env python3
"""Decode shifted-hex test output from mgba-emu/suite timing tests."""
import re
import sys

def decode_shifted_hex(s):
    """Decode hex where 0-9 are '0'-'9' and A-F are ':'-'?'"""
    result = 0
    for c in s:
        result *= 16
        if '0' <= c <= '9':
            result += ord(c) - ord('0')
        elif ':' <= c <= '?':
            result += ord(c) - ord(':') + 10
        else:
            return None
    return result

with open('/tmp/timing_dma.txt') as f:
    lines = f.readlines()

dma_failures = []
for i, line in enumerate(lines):
    if 'FAIL' in line and 'DMA' in line and 'DEBUG' in line:
        m = re.search(r'FAIL: (.+)', line.strip())
        if m:
            test_name = m.group(1)
            for j in range(i+1, min(i+3, len(lines))):
                if 'INFO' in lines[j] and 'FAIL' in lines[j]:
                    m2 = re.search(r'Got\s+(\S+)\s+vs\s+(\S+):', lines[j].strip())
                    if m2:
                        got_raw = m2.group(1)
                        exp_raw = m2.group(2)
                        got = decode_shifted_hex(got_raw)
                        exp = decode_shifted_hex(exp_raw)
                        if got is not None and exp is not None:
                            delta = got - exp
                            dma_failures.append((test_name, got, exp, delta))
                    break

print(f'Total DMA failures: {len(dma_failures)}')
print()
print(f'{"Test":<55} {"Got":>5} {"Exp":>5} {"D":>4}')
print('-' * 72)
for name, got, exp, delta in dma_failures:
    print(f'{name:<55} {got:>5} {exp:>5} {delta:>+4d}')

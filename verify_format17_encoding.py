#!/usr/bin/env python3
"""
Verify ARM Thumb Format 17 (Software Interrupt) instruction encoding.

Format 17: Software interrupt
Encoding: 11011111 Value8
Instructions: SWI

Bit pattern: 1101 1111 VVVV VVVV
Where VVVVVVVV is an 8-bit comment value (0x00-0xFF)
"""

def encode_swi(comment):
    """
    Encode a SWI instruction with the given comment value.
    
    Args:
        comment: 8-bit comment value (0-255)
    
    Returns:
        16-bit instruction encoding
    """
    if not (0 <= comment <= 255):
        raise ValueError(f"Comment must be 0-255, got {comment}")
    
    # Format 17: 11011111 Value8
    instruction = 0b1101111100000000 | (comment & 0xFF)
    return instruction

def decode_swi(instruction):
    """
    Decode a SWI instruction to extract the comment value.
    
    Args:
        instruction: 16-bit instruction
    
    Returns:
        comment value (0-255) or None if not a valid SWI instruction
    """
    # Check if this is a Format 17 SWI instruction
    if (instruction & 0xFF00) != 0xDF00:
        return None
    
    comment = instruction & 0xFF
    return comment

def verify_encoding():
    """Verify SWI instruction encoding for various comment values."""
    
    print("=== ARM Thumb Format 17 (SWI) Encoding Verification ===\n")
    
    # Test cases: comment values and expected encodings
    test_cases = [
        (0x00, 0xDF00),  # SWI #0
        (0x01, 0xDF01),  # SWI #1
        (0x0F, 0xDF0F),  # SWI #15
        (0x10, 0xDF10),  # SWI #16
        (0x20, 0xDF20),  # SWI #32
        (0x40, 0xDF40),  # SWI #64
        (0x7F, 0xDF7F),  # SWI #127
        (0x80, 0xDF80),  # SWI #128
        (0xAA, 0xDFAA),  # SWI #170
        (0xFF, 0xDFFF),  # SWI #255
    ]
    
    print("Testing SWI instruction encoding:")
    print("Comment | Expected | Encoded  | Decoded | Status")
    print("--------|----------|----------|---------|--------")
    
    all_passed = True
    
    for comment, expected in test_cases:
        try:
            # Test encoding
            encoded = encode_swi(comment)
            
            # Test decoding
            decoded_comment = decode_swi(encoded)
            
            # Verify results
            encoding_ok = (encoded == expected)
            decoding_ok = (decoded_comment == comment)
            status = "PASS" if (encoding_ok and decoding_ok) else "FAIL"
            
            if not (encoding_ok and decoding_ok):
                all_passed = False
            
            print(f"  0x{comment:02X}  |  0x{expected:04X}  |  0x{encoded:04X}  |   0x{decoded_comment:02X}   | {status}")
            
            if not encoding_ok:
                print(f"        ERROR: Expected 0x{expected:04X}, got 0x{encoded:04X}")
            if not decoding_ok:
                print(f"        ERROR: Decoded comment 0x{decoded_comment:02X}, expected 0x{comment:02X}")
                
        except Exception as e:
            print(f"  0x{comment:02X}  |  0x{expected:04X}  |   ERROR  |  ERROR  | FAIL")
            print(f"        Exception: {e}")
            all_passed = False
    
    print()
    
    # Test boundary conditions
    print("Testing boundary conditions:")
    
    # Test minimum value
    try:
        min_encoded = encode_swi(0)
        min_decoded = decode_swi(min_encoded)
        min_ok = (min_encoded == 0xDF00 and min_decoded == 0)
        print(f"Minimum (0):    0x{min_encoded:04X}, decoded: 0x{min_decoded:02X} - {'PASS' if min_ok else 'FAIL'}")
        if not min_ok:
            all_passed = False
    except Exception as e:
        print(f"Minimum (0):    ERROR - {e}")
        all_passed = False
    
    # Test maximum value
    try:
        max_encoded = encode_swi(255)
        max_decoded = decode_swi(max_encoded)
        max_ok = (max_encoded == 0xDFFF and max_decoded == 255)
        print(f"Maximum (255):  0x{max_encoded:04X}, decoded: 0x{max_decoded:02X} - {'PASS' if max_ok else 'FAIL'}")
        if not max_ok:
            all_passed = False
    except Exception as e:
        print(f"Maximum (255):  ERROR - {e}")
        all_passed = False
    
    # Test invalid values
    print("\nTesting invalid comment values:")
    invalid_values = [-1, 256, 300, 1000]
    
    for invalid in invalid_values:
        try:
            encode_swi(invalid)
            print(f"Invalid ({invalid}): ERROR - Should have raised exception")
            all_passed = False
        except ValueError:
            print(f"Invalid ({invalid}): PASS - Correctly rejected")
        except Exception as e:
            print(f"Invalid ({invalid}): ERROR - Wrong exception type: {e}")
            all_passed = False
    
    # Test instruction format recognition
    print("\nTesting instruction format recognition:")
    
    # Test non-SWI instructions that should not be decoded as SWI
    non_swi_instructions = [
        0xDE00,  # Format 16 (conditional branch)
        0xE000,  # Format 18 (unconditional branch)
        0xF000,  # Format 19 (long branch with link)
        0xC000,  # Format 15 (multiple load/store)
        0x0000,  # Format 1 (move shifted register)
    ]
    
    for instruction in non_swi_instructions:
        decoded = decode_swi(instruction)
        is_correct = (decoded is None)
        print(f"0x{instruction:04X}: {'PASS' if is_correct else 'FAIL'} - {'Not SWI' if is_correct else f'Incorrectly decoded as SWI #{decoded}'}")
        if not is_correct:
            all_passed = False
    
    print(f"\n=== Overall Result: {'PASS' if all_passed else 'FAIL'} ===")
    
    if all_passed:
        print("All Format 17 (SWI) encoding tests passed!")
    else:
        print("Some Format 17 (SWI) encoding tests failed!")
    
    return all_passed

if __name__ == "__main__":
    verify_encoding()

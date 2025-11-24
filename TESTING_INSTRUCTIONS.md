# Testing Instructions

## Running the Emulator

**CRITICAL**: Always run the emulator with a **30-second timeout** when testing. This ensures:
- The Nintendo logo animation completes fully
- Semi-transparent sprite effects have time to appear
- Sufficient frames are rendered for debugging

### Standard Test Command

```bash
timeout 30 ./gba_emulator assets/test_pixels.gba
```

### Running with Log Capture

```bash
timeout 30 ./gba_emulator assets/test_pixels.gba > /tmp/gba_debug.log 2>&1
```

Then search the logs:
```bash
grep "pattern" /tmp/gba_debug.log
```

## Why 30 Seconds?

- The BIOS Nintendo logo animation takes ~5-10 seconds
- Semi-transparent sprite effects appear during the logo animation
- Running for less than 30 seconds may miss critical rendering behavior
- 30 seconds = ~1800 frames at 60 FPS, providing ample debug data

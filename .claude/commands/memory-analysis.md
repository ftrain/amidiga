---
description: Analyze memory usage for Teensy 4.1 deployment
---

Perform a memory analysis for GRUVBOK on Teensy 4.1:

1. **Data Structure Calculation**
   - Calculate Song memory: 15 modes × 32 patterns × 8 tracks × 16 events
   - Show Event size (bit-packed vs. struct)
   - Estimate total RAM usage

2. **Code Size**
   - If compiled, run: `arm-none-eabi-size --format=SysV gruvbok.elf`
   - Show flash and RAM usage
   - Compare against Teensy 4.1 limits (1MB RAM, 8MB Flash)

3. **Optimization Opportunities**
   - Identify potential memory waste
   - Suggest bit-packing improvements
   - Check for dynamic allocations in hot paths

4. **Real-Time Analysis**
   - Estimate stack usage in playback loop
   - Check Lua memory footprint
   - Verify no heap allocations in critical path

Provide recommendations for staying within Teensy constraints.

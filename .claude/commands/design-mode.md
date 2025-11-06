---
description: Design a new musical mode concept for GRUVBOK
---

Design a new mode concept for GRUVBOK:

- Mode concept: [ask if not specified - e.g., "euclidean rhythm generator", "arpeggiator", "chord strum"]

The design should include:

1. **Musical Purpose**: What does this mode do? Why is it useful?

2. **Track Usage**: How do the 8 tracks function?
   - Are they individual voices?
   - Different rhythmic patterns?
   - Chord degrees?

3. **Slider Mappings** (S1-S4): What parameters do they control?
   - Choose musically meaningful parameters
   - Consider real-time playability
   - Examples: filter, resonance, octave, swing, density, etc.

4. **Button Interaction** (B1-B16): What do the 16 switches do?
   - Note triggers?
   - Step enables?
   - Chord voicings?

5. **MIDI Output Strategy**:
   - What notes are generated?
   - How are velocities determined?
   - Any CC messages needed?

6. **Implementation Hints**:
   - Any Lua tables needed for mapping?
   - Calculations required (scales, euclidean rhythm, etc.)
   - State to track between events

Don't implement yet - just design the concept thoroughly.

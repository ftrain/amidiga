---
description: Scaffold a new Lua mode for GRUVBOK
---

Create a new Lua mode script with the following details:

- Mode number: [ask if not specified]
- Mode name: [ask if not specified]
- Description: [ask if not specified]

The new mode should:
1. Be created in the `modes/` directory as `[number]_[name].lua`
2. Include the standard init() and process_event() functions
3. Include helpful comments explaining the mode's purpose
4. Follow the Lua Mode Contract from CLAUDE.md
5. Include example usage of the Lua API (note, off, cc, stopall)

After creating the mode:
- Show the file contents
- Explain how to test it
- Suggest appropriate S1-S4 slider mappings for musical parameters

# Claude Commands for GRUVBOK

This directory contains slash commands to help with common GRUVBOK development tasks.

## Available Commands

### `/add-mode`
**Scaffold a new Lua mode**

Creates a new mode script with proper structure and template code.
```
Usage: /add-mode
```
Claude will ask for mode number, name, and description, then create the file in `modes/`.

---

### `/architecture-check`
**Review architecture and implementation status**

Analyzes the current state of the codebase:
- What components exist
- What's still needed
- Current development phase (1-5)
- Next priorities

```
Usage: /architecture-check
```

---

### `/design-mode`
**Design a new musical mode concept**

Helps brainstorm and design mode concepts before implementing:
- Musical purpose
- Track usage
- Slider mappings (S1-S4)
- Button interaction (B1-B16)
- MIDI output strategy

```
Usage: /design-mode
Example: /design-mode
Then specify: "euclidean rhythm generator"
```

---

### `/explain-mode`
**Explain how a mode works**

Deep dive into an existing Lua mode:
- Code walkthrough
- Parameter mappings
- MIDI output behavior
- Musical examples

```
Usage: /explain-mode
Example: /explain-mode
Then specify: "01_drums"
```

---

### `/memory-analysis`
**Analyze Teensy memory usage**

Calculates memory requirements:
- Data structure sizes
- Flash and RAM usage
- Optimization opportunities
- Real-time constraints

```
Usage: /memory-analysis
```

---

## Tips

1. **Starting a new session?** Run `/architecture-check` first to see current status
2. **Adding features?** Design with `/design-mode` before implementing
3. **Debugging modes?** Use `/explain-mode` to understand behavior
4. **Preparing for Teensy?** Run `/memory-analysis` to check constraints

## File Structure

Each command is a markdown file with:
- YAML frontmatter with description
- Detailed prompt for Claude
- Context about what to analyze or create

## Creating New Commands

To add a new command:

1. Create `commands/your-command.md`
2. Add YAML frontmatter:
   ```yaml
   ---
   description: Brief description of command
   ---
   ```
3. Write the prompt/instructions for Claude
4. Update this README

## Related Documentation

- **README.md** - Project overview and concept
- **CLAUDE.md** - Comprehensive development guide
- **PROJECT_STRUCTURE.md** - Directory layout and organization

---

*These commands are designed to accelerate GRUVBOK development with Claude.*

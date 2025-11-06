# GRUVBOK Song Format Specification

## Overview

GRUVBOK songs are stored in JSON format for human readability and ease of editing. The format is designed to be:
- **Hand-editable** with any text editor
- **Compact** enough for SD card storage on Teensy 4.1
- **Version-controlled** for future compatibility
- **Lossless** - preserves all event data exactly

## File Structure

### File Extension: `.grv`
Song files use the `.grv` extension (GRUVBOK format).

### Location on SD Card
```
SD:/songs/
├── demo.grv
├── mysong.grv
└── experiments/
    ├── acid_test.grv
    └── drums.grv
```

## JSON Schema

### Basic Structure
```json
{
  "format_version": "1.0",
  "metadata": {
    "title": "My Song",
    "author": "Artist Name",
    "created": "2025-11-06T14:00:00Z",
    "modified": "2025-11-06T15:30:00Z",
    "tempo": 120,
    "description": "A groovy acid bassline with drums"
  },
  "modes": {
    "0": {
      "name": "Song Sequencer",
      "patterns": {
        "0": {
          "tracks": {
            "0": {
              "events": [
                {"step": 0, "switch": true, "pots": [0, 0, 0, 0]},
                {"step": 4, "switch": true, "pots": [4, 0, 0, 0]}
              ]
            }
          }
        }
      }
    },
    "1": {
      "name": "Drums",
      "patterns": {
        "0": {
          "tracks": {
            "0": {
              "events": [
                {"step": 0, "switch": true, "pots": [100, 50, 0, 0]},
                {"step": 4, "switch": true, "pots": [90, 50, 0, 0]}
              ]
            },
            "1": {
              "events": [
                {"step": 4, "switch": true, "pots": [90, 30, 0, 0]}
              ]
            }
          }
        }
      }
    }
  }
}
```

### Field Descriptions

#### Top-Level Fields

**`format_version`** (string, required)
- Version of the file format
- Current version: `"1.0"`
- Used for forward/backward compatibility

**`metadata`** (object, required)
- Song-level information
- Fields:
  - `title` (string): Song name
  - `author` (string): Creator name
  - `created` (ISO 8601 timestamp): Creation date
  - `modified` (ISO 8601 timestamp): Last modified date
  - `tempo` (integer, 1-1000): Default tempo in BPM
  - `description` (string, optional): Song notes

**`modes`** (object, required)
- Dictionary of modes (keys are mode numbers 0-14 as strings)
- Each mode contains patterns

#### Mode Fields

**`name`** (string, optional)
- Human-readable mode name (e.g., "Drums", "Acid", "Chords")
- Defaults to "Mode N" if not specified

**`patterns`** (object, required)
- Dictionary of patterns (keys are pattern numbers 0-31 as strings)
- Only non-empty patterns need to be saved

#### Pattern Fields

**`tracks`** (object, required)
- Dictionary of tracks (keys are track numbers 0-7 as strings)
- Only non-empty tracks need to be saved

#### Track Fields

**`events`** (array, required)
- Array of events (only events with `switch = true` are saved)
- Sparse array - empty steps are omitted

#### Event Fields

**`step`** (integer, 0-15, required)
- Step number within the pattern

**`switch`** (boolean, required)
- Event on/off state
- If `false`, event can be omitted entirely

**`pots`** (array[4], required)
- Four slider values (S1-S4)
- Each value is 0-127 (MIDI range)
- Format: `[s1, s2, s3, s4]`

## Compact Representation

### Minimal Example
A minimal song with just one drum hit:
```json
{
  "format_version": "1.0",
  "metadata": {"title": "Kick", "tempo": 120},
  "modes": {
    "1": {
      "patterns": {
        "0": {
          "tracks": {
            "0": {
              "events": [
                {"step": 0, "switch": true, "pots": [100, 50, 0, 0]}
              ]
            }
          }
        }
      }
    }
  }
}
```

### Omitting Empty Data
- Empty modes can be omitted
- Empty patterns can be omitted
- Empty tracks can be omitted
- Events with `switch = false` can be omitted
- This significantly reduces file size

## File Size Estimates

### Worst Case (Full Song)
```
Metadata:           ~200 bytes
15 modes:
  32 patterns each:
    8 tracks each:
      16 events each:
        ~50 bytes per event

Total events: 15 × 32 × 8 × 16 = 61,440 events
Worst case: 61,440 × 50 = ~3 MB (if all events active)
```

### Typical Case (Sparse Song)
```
Metadata:           ~200 bytes
4 active modes:
  4 patterns each:
    4 tracks each:
      8 events each (50% sparse):

Total events: 4 × 4 × 4 × 8 = 512 events
Typical: 512 × 50 = ~25 KB
```

**Conclusion:** Even large songs fit easily on SD card (typical songs 10-100KB).

## Binary Format (Alternative)

For extremely space-constrained scenarios, a binary format can be used:

### File Extension: `.grvb`

### Structure
```
Header (32 bytes):
  - Magic: "GRVB" (4 bytes)
  - Version: uint16 (2 bytes)
  - Tempo: uint16 (2 bytes)
  - Title length: uint8 (1 byte)
  - Title: variable length (max 255 bytes)
  - Padding to 32-byte boundary

Mode Index (60 bytes):
  - 15 uint32 offsets to mode data (15 × 4 = 60 bytes)

Mode Data (variable):
  For each mode:
    - Pattern index: 32 uint32 offsets (128 bytes)
    - Pattern data: variable

Pattern Data (variable):
  For each pattern:
    - Track index: 8 uint32 offsets (32 bytes)
    - Track data: variable

Track Data (variable):
  - Number of events: uint8 (1 byte)
  - Events: uint32 each (4 bytes × N events)
    - Uses same 29-bit packing as in-memory Event structure
```

### Size Comparison
```
JSON:   ~50 bytes per event
Binary: ~4 bytes per event (12.5× smaller!)
```

For a typical 512-event song:
- JSON: ~25 KB
- Binary: ~2 KB

**Recommendation:** Use JSON for ease of editing. Binary is available if needed.

## Loading/Saving API (C++)

### Load from JSON
```cpp
#include <ArduinoJson.h>  // Or RapidJSON for desktop

bool Song::loadFromFile(const char* filename) {
    // Open file
    File file = SD.open(filename, FILE_READ);
    if (!file) return false;

    // Parse JSON
    DynamicJsonDocument doc(65536);  // 64KB buffer
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) return false;

    // Extract metadata
    int tempo = doc["metadata"]["tempo"] | 120;

    // Load modes
    JsonObject modes = doc["modes"];
    for (JsonPair mode_pair : modes) {
        int mode_num = atoi(mode_pair.key().c_str());
        JsonObject mode_obj = mode_pair.value();

        Mode& mode = getMode(mode_num);

        // Load patterns
        JsonObject patterns = mode_obj["patterns"];
        for (JsonPair pattern_pair : patterns) {
            int pattern_num = atoi(pattern_pair.key().c_str());
            JsonObject pattern_obj = pattern_pair.value();

            Pattern& pattern = mode.getPattern(pattern_num);

            // Load tracks
            JsonObject tracks = pattern_obj["tracks"];
            for (JsonPair track_pair : tracks) {
                int track_num = atoi(track_pair.key().c_str());
                JsonArray events = track_pair.value()["events"];

                // Load events
                for (JsonObject event_obj : events) {
                    int step = event_obj["step"];
                    bool sw = event_obj["switch"];
                    JsonArray pots = event_obj["pots"];

                    Event& evt = pattern.getEvent(track_num, step);
                    evt.setSwitch(sw);
                    evt.setPot(0, pots[0]);
                    evt.setPot(1, pots[1]);
                    evt.setPot(2, pots[2]);
                    evt.setPot(3, pots[3]);
                }
            }
        }
    }

    return true;
}
```

### Save to JSON
```cpp
bool Song::saveToFile(const char* filename) {
    // Create JSON document
    DynamicJsonDocument doc(65536);

    // Metadata
    doc["format_version"] = "1.0";
    JsonObject metadata = doc.createNestedObject("metadata");
    metadata["title"] = "My Song";
    metadata["tempo"] = 120;
    // ... other metadata

    // Modes
    JsonObject modes_obj = doc.createNestedObject("modes");

    for (int m = 0; m < NUM_MODES; m++) {
        Mode& mode = getMode(m);
        bool mode_has_data = false;

        JsonObject mode_obj = modes_obj.createNestedObject(String(m));
        JsonObject patterns_obj = mode_obj.createNestedObject("patterns");

        for (int p = 0; p < Mode::NUM_PATTERNS; p++) {
            Pattern& pattern = mode.getPattern(p);
            bool pattern_has_data = false;

            JsonObject pattern_obj = patterns_obj.createNestedObject(String(p));
            JsonObject tracks_obj = pattern_obj.createNestedObject("tracks");

            for (int t = 0; t < Pattern::NUM_TRACKS; t++) {
                Track& track = pattern.getTrack(t);
                bool track_has_data = false;

                JsonObject track_obj = tracks_obj.createNestedObject(String(t));
                JsonArray events_arr = track_obj.createNestedArray("events");

                for (int s = 0; s < 16; s++) {
                    const Event& evt = track.getEvent(s);

                    if (evt.getSwitch()) {
                        track_has_data = true;
                        pattern_has_data = true;
                        mode_has_data = true;

                        JsonObject event_obj = events_arr.createNestedObject();
                        event_obj["step"] = s;
                        event_obj["switch"] = true;
                        JsonArray pots = event_obj.createNestedArray("pots");
                        pots.add(evt.getPot(0));
                        pots.add(evt.getPot(1));
                        pots.add(evt.getPot(2));
                        pots.add(evt.getPot(3));
                    }
                }

                if (!track_has_data) {
                    tracks_obj.remove(String(t));  // Remove empty track
                }
            }

            if (!pattern_has_data) {
                patterns_obj.remove(String(p));  // Remove empty pattern
            }
        }

        if (!mode_has_data) {
            modes_obj.remove(String(m));  // Remove empty mode
        }
    }

    // Write to file
    File file = SD.open(filename, FILE_WRITE);
    if (!file) return false;

    serializeJsonPretty(doc, file);  // Pretty-print for readability
    file.close();

    return true;
}
```

## Desktop Integration

On desktop, songs can be saved to:
- `~/Documents/GRUVBOK/songs/`
- Or any user-specified directory

Use the same JSON format for cross-platform compatibility.

## Versioning

Future versions may add fields:
- `format_version: "1.1"` - Adds new features
- `format_version: "2.0"` - Breaking changes

Parsers should:
1. Check `format_version`
2. Support older versions (backward compatibility)
3. Warn user if file is newer than supported version

## Examples

### Complete Example Song

See `examples/demo_song.grv` for a full example with:
- All 4 current modes (drums, acid, chords, song sequencer)
- Multiple patterns
- Complex arrangements
- Metadata

### Minimal Templates

**Empty Song:**
```json
{
  "format_version": "1.0",
  "metadata": {"title": "Untitled", "tempo": 120},
  "modes": {}
}
```

**Single Kick Drum:**
```json
{
  "format_version": "1.0",
  "metadata": {"title": "Kick", "tempo": 120},
  "modes": {
    "1": {
      "patterns": {
        "0": {
          "tracks": {
            "0": {
              "events": [{"step": 0, "switch": true, "pots": [100, 50, 0, 0]}]
            }
          }
        }
      }
    }
  }
}
```

## Tools

### Song Converter (Future)
```bash
# Convert JSON to binary
gruvbok-convert demo.grv demo.grvb

# Convert binary to JSON
gruvbok-convert demo.grvb demo.grv

# Validate song format
gruvbok-validate demo.grv
```

### Song Editor (Future)
Web-based song editor for creating/editing .grv files without hardware.

## Summary

The GRUVBOK song format is:
✅ Human-readable (JSON)
✅ Hand-editable (any text editor)
✅ Compact (typical songs 10-100KB)
✅ Lossless (preserves all data)
✅ Version-controlled (future-proof)
✅ Cross-platform (desktop and Teensy)
✅ SD card friendly (FAT32 compatible)

Binary format available if space is critical, but JSON is recommended for ease of use.

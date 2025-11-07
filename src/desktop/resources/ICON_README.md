# GRUVBOK Icon Generation

## Quick Start

The GRUVBOK icon has been designed with a circuit board + waveform aesthetic in neon cyan/green on a dark background.

### Automated Generation (Recommended)

```bash
cd src/desktop/resources
./generate_icns.sh
```

This script will:
1. Convert `gruvbok_icon.svg` to all required PNG sizes
2. Create `gruvbok.iconset/` directory with proper naming
3. Generate `gruvbok.icns` (on macOS with iconutil)

### Requirements

**Option 1: librsvg (Recommended)**
```bash
# macOS
brew install librsvg

# Linux
apt-get install librsvg2-bin
```

**Option 2: ImageMagick**
```bash
# macOS
brew install imagemagick

# Linux
apt-get install imagemagick
```

---

## Manual Generation

If the automated script doesn't work, follow these steps:

### 1. Install Dependencies (macOS)

```bash
brew install librsvg  # For SVG to PNG conversion
```

### 2. Convert SVG to PNG at All Required Sizes

```bash
cd src/desktop/resources

# Create iconset directory
mkdir -p gruvbok.iconset

# Generate all sizes
rsvg-convert -w 16 -h 16 gruvbok_icon.svg -o gruvbok.iconset/icon_16x16.png
rsvg-convert -w 32 -h 32 gruvbok_icon.svg -o gruvbok.iconset/icon_16x16@2x.png
rsvg-convert -w 32 -h 32 gruvbok_icon.svg -o gruvbok.iconset/icon_32x32.png
rsvg-convert -w 64 -h 64 gruvbok_icon.svg -o gruvbok.iconset/icon_32x32@2x.png
rsvg-convert -w 128 -h 128 gruvbok_icon.svg -o gruvbok.iconset/icon_128x128.png
rsvg-convert -w 256 -h 256 gruvbok_icon.svg -o gruvbok.iconset/icon_128x128@2x.png
rsvg-convert -w 256 -h 256 gruvbok_icon.svg -o gruvbok.iconset/icon_256x256.png
rsvg-convert -w 512 -h 512 gruvbok_icon.svg -o gruvbok.iconset/icon_256x256@2x.png
rsvg-convert -w 512 -h 512 gruvbok_icon.svg -o gruvbok.iconset/icon_512x512.png
rsvg-convert -w 1024 -h 1024 gruvbok_icon.svg -o gruvbok.iconset/icon_512x512@2x.png
```

### 3. Create .icns File (macOS Only)

```bash
iconutil -c icns gruvbok.iconset
# Output: gruvbok.icns
```

### 4. Rebuild GRUVBOK

```bash
cd /path/to/amidiga
cmake --build build
```

The icon will be automatically included in `GRUVBOK.app`.

---

## Alternative: Online Conversion

If you don't have the tools installed, you can use online converters:

1. **PNG to ICNS Converter**: https://cloudconvert.com/png-to-icns
2. Upload the largest PNG from the iconset: `icon_512x512@2x.png` (1024x1024)
3. Download the converted `gruvbok.icns`
4. Place it in `src/desktop/resources/gruvbok.icns`

---

## Design Concept

The GRUVBOK icon represents the fusion of electronic circuits and musical sequencing:

### Visual Elements

- **Dark Background** (`#1a1a2e` → `#0f0f1e` gradient)
  - Modern, professional look
  - High contrast for visibility

- **Circuit Board Traces** (Cyan `#00d9ff`)
  - Horizontal and vertical lines
  - Connection points (small circles)
  - Represents the hardware/electronic nature

- **16-Step Grid** (Neon Green `#0aff9d`)
  - 4×4 grid representing the 16 buttons (B1-B16)
  - Active steps shown as glowing dots
  - Core visual metaphor for the sequencer

- **Waveforms** (Cyan with glow effect)
  - Stylized audio waveforms on sides
  - Musical/audio representation

- **"GRUVBOK" Text** (Neon Green with glow)
  - Bold, geometric font
  - Readable at small sizes

- **Corner Accents** (Cyan circles)
  - Circuit board solder pads
  - Technical aesthetic

### Color Palette

- **Background**: `#1a1a2e` (dark blue-gray)
- **Cyan**: `#00d9ff` (circuit traces, waveforms)
- **Neon Green**: `#0aff9d` (active elements, text, grid)
- **Glow Effect**: Gaussian blur for neon aesthetic

---

## Icon Sizes Reference

Apple's iconset requires these exact filenames and sizes:

| Filename | Actual Size | Purpose |
|----------|-------------|---------|
| `icon_16x16.png` | 16×16 | Standard small icon |
| `icon_16x16@2x.png` | 32×32 | Retina small icon |
| `icon_32x32.png` | 32×32 | Standard medium icon |
| `icon_32x32@2x.png` | 64×64 | Retina medium icon |
| `icon_128x128.png` | 128×128 | Standard large icon |
| `icon_128x128@2x.png` | 256×256 | Retina large icon |
| `icon_256x256.png` | 256×256 | Standard huge icon |
| `icon_256x256@2x.png` | 512×512 | Retina huge icon |
| `icon_512x512.png` | 512×512 | Standard max icon |
| `icon_512x512@2x.png` | 1024×1024 | Retina max icon |

---

## Troubleshooting

### "rsvg-convert: command not found"

Install librsvg:
```bash
brew install librsvg  # macOS
apt-get install librsvg2-bin  # Linux
```

### "iconutil: command not found"

`iconutil` is macOS-only. You're probably on Linux. Options:
1. Run the script on a Mac
2. Use online converter (see "Alternative: Online Conversion" above)
3. Use `png2icns` tool on Linux

### Icon doesn't appear in GRUVBOK.app

1. Verify `gruvbok.icns` exists in `src/desktop/resources/`
2. Rebuild the project: `cmake --build build`
3. Clear icon cache (macOS): `sudo rm -rf /Library/Caches/com.apple.iconservices.store`
4. Restart Finder: `killall Finder`

### Icon looks blurry at small sizes

The SVG design is optimized for 1024×1024. At very small sizes (16×16, 32×32), some details may appear blurry. This is normal for complex icons. The glow effects and grid should still be visible.

---

## Customizing the Icon

To modify the design, edit `gruvbok_icon.svg` in any vector graphics editor:

- **Inkscape** (Free, open-source)
- **Adobe Illustrator**
- **Affinity Designer**
- **Figma** (Web-based)

Or edit the SVG XML directly - it's well-commented!

Key elements to customize:
- Colors: Search for `#00d9ff` (cyan) and `#0aff9d` (green)
- Grid pattern: Lines starting around line 50
- Active steps: Circle elements around line 65
- Text: Change "GRUVBOK" near the end

After editing, re-run `./generate_icns.sh`.

---

## Files in This Directory

- **`gruvbok_icon.svg`** - Source vector graphic (1024×1024)
- **`generate_icns.sh`** - Automated conversion script
- **`gruvbok.iconset/`** - Generated PNG files (after running script)
- **`gruvbok.icns`** - Final macOS icon (after running script)
- **`ICON_README.md`** - This file

---

## Preview

The icon features:
- Circuit board aesthetic (cyan traces and connection points)
- 16-step grid with active steps lit up (neon green)
- Stylized waveforms on sides
- Bold "GRUVBOK" text at bottom
- Dark background with subtle gradient
- Neon glow effects for modern look

Perfect for a hardware groovebox/sequencer aesthetic! 🎹⚡️


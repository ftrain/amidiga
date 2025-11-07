# GRUVBOK Icon Generation

## Creating the .icns file

To generate `gruvbok.icns` for the macOS app bundle:

### 1. Create PNG icons at multiple sizes:
- 16x16, 32x32, 64x64, 128x128, 256x256, 512x512, 1024x1024

### 2. Name them according to Apple's iconset format:
```
icon_16x16.png
icon_16x16@2x.png (32x32)
icon_32x32.png
icon_32x32@2x.png (64x64)
icon_128x128.png
icon_128x128@2x.png (256x256)
icon_256x256.png
icon_256x256@2x.png (512x512)
icon_512x512.png
icon_512x512@2x.png (1024x1024)
```

### 3. Create iconset directory:
```bash
mkdir gruvbok.iconset
# Move all PNG files into gruvbok.iconset/
```

### 4. Generate .icns using iconutil (macOS only):
```bash
iconutil -c icns gruvbok.iconset
# Output: gruvbok.icns
```

### 5. Copy to resources:
```bash
cp gruvbok.icns src/desktop/resources/
```

## Design Concept

**GRUVBOK icon aesthetic:**
- Circuit board traces + waveform
- Colors: Neon green/cyan on dark background
- Grid pattern representing 16 steps
- Musical + electronic fusion

## Quick Generation (Placeholder)

For now, you can use any 1024x1024 PNG and convert it:
```bash
# On macOS
sips -s format icns your_image.png --out gruvbok.icns
```

## TODO
- [ ] Design actual icon artwork
- [ ] Generate multi-resolution iconset
- [ ] Convert to .icns format
- [ ] Test in app bundle

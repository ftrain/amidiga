#!/bin/bash

# Create GRUVBOK app icon using macOS built-in tools

set -e

echo "Creating GRUVBOK icon..."

# Create a temporary directory for the iconset
ICONSET="AppIcon.iconset"
rm -rf "$ICONSET"
mkdir "$ICONSET"

# Create a base 1024x1024 image using sips
# Start with a colored square
BASE_IMG="base_icon.png"

# Create a gradient cyan-to-blue image using ImageMagick-style approach
# Since we don't have PIL, we'll create it with Swift
cat > create_base_icon.swift << 'EOF'
import AppKit
import CoreGraphics

let size: CGFloat = 1024
let image = NSImage(size: NSSize(width: size, height: size))

image.lockFocus()

// Background gradient
let gradient = NSGradient(colors: [
    NSColor(red: 0.1, green: 0.1, blue: 0.15, alpha: 1.0),
    NSColor(red: 0.05, green: 0.05, blue: 0.1, alpha: 1.0)
])
gradient?.draw(in: NSRect(x: 0, y: 0, width: size, height: size), angle: 90)

// Draw 4x4 grid of pads (sequencer style)
let margin: CGFloat = 120
let padSize: CGFloat = 160
let spacing: CGFloat = 40

for row in 0..<4 {
    for col in 0..<4 {
        let x = margin + CGFloat(col) * (padSize + spacing)
        let y = margin + CGFloat(row) * (padSize + spacing)

        // Determine if this pad is "active"
        let active = (row + col) % 3 == 0

        // Shadow
        let shadowPath = NSBezierPath(roundedRect: NSRect(x: x+3, y: y-3, width: padSize, height: padSize), xRadius: 12, yRadius: 12)
        NSColor.black.withAlphaComponent(0.5).setFill()
        shadowPath.fill()

        // Pad
        let padPath = NSBezierPath(roundedRect: NSRect(x: x, y: y, width: padSize, height: padSize), xRadius: 12, yRadius: 12)

        if active {
            NSColor(red: 0, green: 1, blue: 1, alpha: 0.9).setFill()
        } else {
            NSColor(red: 0, green: 0.3, blue: 0.35, alpha: 1.0).setFill()
        }
        padPath.fill()

        // Border
        if active {
            NSColor.white.withAlphaComponent(0.8).setStroke()
        } else {
            NSColor.white.withAlphaComponent(0.2).setStroke()
        }
        padPath.lineWidth = 2
        padPath.stroke()

        // Step number
        let stepNum = "\(row * 4 + col + 1)"
        let attrs: [NSAttributedString.Key: Any] = [
            .font: NSFont.boldSystemFont(ofSize: 48),
            .foregroundColor: active ? NSColor.white : NSColor.gray
        ]
        let textSize = (stepNum as NSString).size(withAttributes: attrs)
        let textX = x + (padSize - textSize.width) / 2
        let textY = y + (padSize - textSize.height) / 2
        (stepNum as NSString).draw(at: NSPoint(x: textX, y: textY), withAttributes: attrs)
    }
}

image.unlockFocus()

// Save as PNG
if let tiffData = image.tiffRepresentation,
   let bitmapImage = NSBitmapImageRep(data: tiffData),
   let pngData = bitmapImage.representation(using: .png, properties: [:]) {
    try? pngData.write(to: URL(fileURLWithPath: "base_icon.png"))
    print("Created base_icon.png")
}
EOF

# Compile and run Swift script
swift create_base_icon.swift

if [ ! -f "$BASE_IMG" ]; then
    echo "Failed to create base icon"
    exit 1
fi

# Generate all required icon sizes
sips -z 16 16     "$BASE_IMG" --out "$ICONSET/icon_16x16.png"
sips -z 32 32     "$BASE_IMG" --out "$ICONSET/icon_16x16@2x.png"
sips -z 32 32     "$BASE_IMG" --out "$ICONSET/icon_32x32.png"
sips -z 64 64     "$BASE_IMG" --out "$ICONSET/icon_32x32@2x.png"
sips -z 128 128   "$BASE_IMG" --out "$ICONSET/icon_128x128.png"
sips -z 256 256   "$BASE_IMG" --out "$ICONSET/icon_128x128@2x.png"
sips -z 256 256   "$BASE_IMG" --out "$ICONSET/icon_256x256.png"
sips -z 512 512   "$BASE_IMG" --out "$ICONSET/icon_256x256@2x.png"
sips -z 512 512   "$BASE_IMG" --out "$ICONSET/icon_512x512.png"
sips -z 1024 1024 "$BASE_IMG" --out "$ICONSET/icon_512x512@2x.png"

# Convert to .icns
iconutil -c icns "$ICONSET" -o AppIcon.icns

echo "✅ Created AppIcon.icns"

# Clean up
rm -rf "$ICONSET"
rm "$BASE_IMG"
rm create_base_icon.swift

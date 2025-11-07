#!/bin/bash
# Generate macOS .icns icon from SVG source
# Requires: ImageMagick (convert/magick command) or rsvg-convert

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SVG_FILE="$SCRIPT_DIR/gruvbok_icon.svg"
ICONSET_DIR="$SCRIPT_DIR/gruvbok.iconset"

# Check if SVG exists
if [ ! -f "$SVG_FILE" ]; then
    echo "Error: gruvbok_icon.svg not found!"
    exit 1
fi

# Check for conversion tools
if command -v rsvg-convert &> /dev/null; then
    CONVERTER="rsvg"
    echo "Using rsvg-convert for SVG to PNG conversion"
elif command -v magick &> /dev/null; then
    CONVERTER="magick"
    echo "Using ImageMagick (magick) for SVG to PNG conversion"
elif command -v convert &> /dev/null; then
    CONVERTER="convert"
    echo "Using ImageMagick (convert) for SVG to PNG conversion"
else
    echo "Error: No SVG conversion tool found!"
    echo "Please install one of the following:"
    echo "  - librsvg (rsvg-convert): brew install librsvg OR apt-get install librsvg2-bin"
    echo "  - ImageMagick: brew install imagemagick OR apt-get install imagemagick"
    exit 1
fi

# Create iconset directory
echo "Creating iconset directory..."
rm -rf "$ICONSET_DIR"
mkdir -p "$ICONSET_DIR"

# Function to convert SVG to PNG
convert_svg() {
    local size=$1
    local output=$2

    case $CONVERTER in
        rsvg)
            rsvg-convert -w $size -h $size "$SVG_FILE" -o "$output"
            ;;
        magick)
            magick "$SVG_FILE" -resize ${size}x${size} "$output"
            ;;
        convert)
            convert "$SVG_FILE" -resize ${size}x${size} "$output"
            ;;
    esac

    echo "  Generated: $(basename $output) (${size}x${size})"
}

# Generate all required sizes for macOS iconset
echo "Generating PNG files at multiple resolutions..."

convert_svg 16 "$ICONSET_DIR/icon_16x16.png"
convert_svg 32 "$ICONSET_DIR/icon_16x16@2x.png"
convert_svg 32 "$ICONSET_DIR/icon_32x32.png"
convert_svg 64 "$ICONSET_DIR/icon_32x32@2x.png"
convert_svg 128 "$ICONSET_DIR/icon_128x128.png"
convert_svg 256 "$ICONSET_DIR/icon_128x128@2x.png"
convert_svg 256 "$ICONSET_DIR/icon_256x256.png"
convert_svg 512 "$ICONSET_DIR/icon_256x256@2x.png"
convert_svg 512 "$ICONSET_DIR/icon_512x512.png"
convert_svg 1024 "$ICONSET_DIR/icon_512x512@2x.png"

echo ""
echo "✅ All PNG files generated successfully!"
echo ""

# Check if we're on macOS and can create .icns
if [[ "$OSTYPE" == "darwin"* ]]; then
    if command -v iconutil &> /dev/null; then
        echo "Creating .icns file..."
        iconutil -c icns "$ICONSET_DIR" -o "$SCRIPT_DIR/gruvbok.icns"
        echo ""
        echo "✅ gruvbok.icns created successfully!"
        echo ""
        echo "Icon location: $SCRIPT_DIR/gruvbok.icns"
        echo ""
        echo "Next steps:"
        echo "  1. Rebuild GRUVBOK: cmake --build build"
        echo "  2. The .icns will be automatically included in GRUVBOK.app"
        echo ""
    else
        echo "⚠️  iconutil not found (required for .icns creation)"
        echo ""
        echo "Iconset created at: $ICONSET_DIR"
        echo ""
        echo "To create .icns manually:"
        echo "  iconutil -c icns $ICONSET_DIR"
    fi
else
    echo "ℹ️  Not running on macOS - .icns creation skipped"
    echo ""
    echo "Iconset created at: $ICONSET_DIR"
    echo ""
    echo "To create .icns on macOS:"
    echo "  iconutil -c icns $ICONSET_DIR"
    echo ""
    echo "Or use online converter:"
    echo "  https://cloudconvert.com/png-to-icns"
    echo "  (Upload the largest PNG: icon_512x512@2x.png)"
fi

echo ""
echo "🎨 Icon generation complete!"

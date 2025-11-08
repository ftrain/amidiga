#!/usr/bin/env python3
"""
Generate GRUVBOK app icon - a retro groovebox style icon
"""

from PIL import Image, ImageDraw, ImageFont
import os

# Create 1024x1024 icon
size = 1024
img = Image.new('RGB', (size, size), color='#1a1a1a')
draw = ImageDraw.Draw(img)

# Draw retro groovebox style design
# Background gradient effect
for i in range(0, size, 20):
    color = int(26 + (i / size) * 20)
    draw.rectangle([(0, i), (size, i+20)], fill=f'#{color:02x}{color:02x}{color:02x}')

# Draw a grid pattern (like a sequencer)
grid_color = '#00ffff'  # Cyan
grid_dim = '#004455'    # Dim cyan

# Draw 4x4 grid of pads
margin = 150
pad_size = 140
spacing = 40

for row in range(4):
    for col in range(4):
        x = margin + col * (pad_size + spacing)
        y = margin + row * (pad_size + spacing)

        # Active pads (some lit, some not)
        active = (row + col) % 3 == 0
        color = grid_color if active else grid_dim

        # Draw pad with 3D effect
        # Shadow
        draw.rounded_rectangle(
            [(x+4, y+4), (x+pad_size+4, y+pad_size+4)],
            radius=15,
            fill='#000000'
        )
        # Pad
        draw.rounded_rectangle(
            [(x, y), (x+pad_size, y+pad_size)],
            radius=15,
            fill=color,
            outline='#ffffff' if active else '#333333',
            width=3
        )

        # Draw step number
        try:
            # Try to use a system font
            font = ImageFont.truetype('/System/Library/Fonts/Helvetica.ttc', 40)
        except:
            font = ImageFont.load_default()

        step_num = str(row * 4 + col + 1)
        # Get text bounding box
        bbox = draw.textbbox((0, 0), step_num, font=font)
        text_width = bbox[2] - bbox[0]
        text_height = bbox[3] - bbox[1]

        text_x = x + (pad_size - text_width) // 2
        text_y = y + (pad_size - text_height) // 2 - 5

        draw.text((text_x, text_y), step_num, fill='#ffffff' if active else '#666666', font=font)

# Save as PNG
output_path = 'icon.png'
img.save(output_path)
print(f"Created {output_path}")

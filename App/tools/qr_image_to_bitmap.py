#!/usr/bin/env python3
"""
QR Code Image to C Bitmap Converter
Converts QR code image to framebuffer column-major format for LCD display
"""

import sys
from PIL import Image
import numpy as np

def image_to_qr_bitmap(image_path, output_path=None, size=33):
    """
    Convert QR code image to C bitmap array in framebuffer column-major format
    
    Args:
        image_path: Path to input QR code image (PNG/JPG)
        output_path: Path for output .c file (optional)
        size: Target QR code size in modules (default 33 for version 4)
    
    Returns:
        Tuple of (bitmap_array, c_code_string)
    """
    try:
        img = Image.open(image_path)
        
        # Convert to grayscale and resize to target size
        img = img.convert('L')
        img = img.resize((size, size), Image.Resampling.LANCZOS)
        
        # Convert to binary (black=1, white=0)
        threshold = 128
        pixels = np.array(img)
        binary = (pixels < threshold).astype(np.uint8)
        
        # Convert to framebuffer column-major format
        # Format: 5 rows of 33 bytes each (for 33x33 QR code)
        # Each byte packs 8 vertical pixels (bit 0 = top)
        # Last row uses only bit 0
        
        bitmap = []
        for row in range(5):  # 5 framebuffer lines
            row_data = []
            for col in range(size):  # 33 columns
                byte_val = 0
                for bit in range(8):
                    y = row * 8 + bit
                    if y < size and binary[y][col]:
                        byte_val |= (1 << bit)
                row_data.append(byte_val)
            bitmap.extend(row_data)
        
        # Generate C code
        var_name = "BITMAP_QR_Custom_Compressed"
        c_lines = [f"static const uint8_t {var_name}[{len(bitmap)}] = {{"]
        
        # Format as comma-separated values, 16 per line
        for i in range(0, len(bitmap), 16):
            chunk = bitmap[i:i+16]
            line_str = "    " + ", ".join(f"0x{b:02X}" for b in chunk)
            if i + 16 < len(bitmap):
                line_str += ","
            c_lines.append(line_str)
        
        c_lines.append("};")
        c_code = "\n".join(c_lines)
        
        if output_path:
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write(f"// Auto-generated QR code bitmap from: {image_path}\n")
                f.write(f"// Size: {size}x{size} modules\n\n")
                f.write(c_code)
                f.write("\n")
            print(f"✓ Bitmap saved to: {output_path}")
        
        return bitmap, c_code
        
    except Exception as e:
        print(f"✗ Error processing image: {e}")
        return None, None

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python qr_image_to_bitmap.py <input_image> [output_file] [size]")
        print("\nExample:")
        print("  python qr_image_to_bitmap.py qrcode.png qr_bitmap.c 33")
        sys.exit(1)
    
    input_img = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else "qr_bitmap.c"
    qr_size = int(sys.argv[3]) if len(sys.argv) > 3 else 33
    
    print(f"\nConverting QR code image: {input_img}")
    print(f"Target size: {qr_size}x{qr_size} modules")
    print(f"Output file: {output_file}")
    print("-" * 50)
    
    bitmap, code = image_to_qr_bitmap(input_img, output_file, qr_size)
    
    if bitmap:
        print(f"\n✓ Success! Generated {len(bitmap)} bytes ({qr_size}x{ceil({qr_size}/8)} format)")
        print(f"\nPreview of generated code:\n{code[:200]}...")

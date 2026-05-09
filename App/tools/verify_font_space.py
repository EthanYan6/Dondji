#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Verify font library space usage and overlap with legacy region
"""

# Font library
font_base = 0x010200
font_size = 40150
font_end = font_base + font_size

print(f'Font library start: 0x{font_base:06X} ({font_base} bytes)')
print(f'Font library size: {font_size} bytes ({font_size/1024:.1f} KB)')
print(f'Font library end: 0x{font_end:06X} ({font_end} bytes)')

# Legacy CN name region
legacy_base = 0x020000
legacy_size = 0x010000  # 64KB
legacy_end = legacy_base + legacy_size

print(f'\nLegacy CN name region start: 0x{legacy_base:06X}')
print(f'Legacy CN name region end: 0x{legacy_end:06X}')

# Check overlap
if font_end > legacy_base:
    print(f'\n[WARNING] Font library overlaps with legacy CN name region!')
    print(f'Overlap region: 0x{legacy_base:06X} - 0x{min(font_end, legacy_end):06X}')
    print(f'\nThis is OK because:')
    print(f'1. Legacy CN name region is deprecated and no longer used')
    print(f'2. Migration code erases it after migrating data')
    print(f'3. Font library can safely use this space')
else:
    print(f'\n[OK] Font library does not overlap with legacy CN name region')

# SPI Flash total size
spi_total = 2 * 1024 * 1024  # 2MB
print(f'\nSPI Flash total size: {spi_total/1024/1024:.1f} MB')
print(f'Font library usage: {font_size/spi_total*100:.1f}%')
print(f'Remaining space: {(spi_total - font_end)/1024:.1f} KB')

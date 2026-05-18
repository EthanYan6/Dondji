#!/usr/bin/env python3
import re

print("=" * 50)
print("Checking minified files...")
print("=" * 50)

# Check CSS
css = open('docs/css/style.min.css', 'r', encoding='utf-8').read()
print(f"\nCSS file size: {len(css):,} bytes")

key_selectors = [
    '.subtitle-chips',
    '.subtitle-chip',
    '.tab',
    '.tabs',
    '.top-right-buttons',
    '.theme-toggle',
    '.help-btn',
    '.flash-steps-bar',
    '.timeline-sidebar',
    '.loading-overlay',
    '.app-toast',
    '.writefreq-table',
    '.toolbox-grid'
]

print("\nCSS Key Selectors:")
all_css_ok = True
for sel in key_selectors:
    count = len(re.findall(re.escape(sel), css))
    status = "OK" if count > 0 else "MISSING"
    if count == 0:
        all_css_ok = False
    print(f"  {sel}: {count} ({status})")

# Check JS
js = open('docs/js/flash.min.js', 'r', encoding='utf-8').read()
print(f"\nJS file size: {len(js):,} bytes")

key_funcs = [
    'connect',
    'disconnect',
    'flashFirmware',
    'dumpCalibration',
    'restoreCalibration',
    'writefreqReadFromDevice',
    'writefreqWriteToDevice',
    'showToast',
    'updateProgress'
]

print("\nJS Key Functions:")
all_js_ok = True
for func in key_funcs:
    pattern = r'\b' + func + r'\s*\('
    count = len(re.findall(pattern, js))
    status = "OK" if count > 0 else "MISSING"
    if count == 0:
        all_js_ok = False
    print(f"  {func}(): {count} ({status})")

# Check SVG strings
svg_count = len(re.findall(r'<svg[^>]+>', js))
print(f"\nSVG elements found: {svg_count}")

# Check for potential issues
print("\n" + "=" * 50)
print("Potential Issues Check:")
print("=" * 50)

# Check for broken strings
broken_strings = re.findall(r"'[^']*$", js, re.MULTILINE)
if broken_strings:
    print(f"  WARNING: {len(broken_strings)} potentially unclosed single-quoted strings")
else:
    print("  OK: No unclosed single-quoted strings found")

broken_strings2 = re.findall(r'"[^"]*$', js, re.MULTILINE)
if broken_strings2:
    print(f"  WARNING: {len(broken_strings2)} potentially unclosed double-quoted strings")
else:
    print("  OK: No unclosed double-quoted strings found")

# Check for balanced braces
open_braces = js.count('{')
close_braces = js.count('}')
if open_braces == close_braces:
    print(f"  OK: Braces balanced ({open_braces} pairs)")
else:
    print(f"  WARNING: Braces not balanced! Open: {open_braces}, Close: {close_braces}")

# Check for balanced parentheses
open_parens = js.count('(')
close_parens = js.count(')')
if open_parens == close_parens:
    print(f"  OK: Parentheses balanced ({open_parens} pairs)")
else:
    print(f"  WARNING: Parentheses not balanced! Open: {open_parens}, Close: {close_parens}")

# Check for balanced brackets
open_brackets = js.count('[')
close_brackets = js.count(']')
if open_brackets == close_brackets:
    print(f"  OK: Brackets balanced ({open_brackets} pairs)")
else:
    print(f"  WARNING: Brackets not balanced! Open: {open_brackets}, Close: {close_brackets}")

print("\n" + "=" * 50)
if all_css_ok and all_js_ok:
    print("All checks passed!")
else:
    print("Some checks failed - please review above")
print("=" * 50)

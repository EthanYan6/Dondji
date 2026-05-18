#!/usr/bin/env python3
import re
import os

def minify_css(css_content):
    css_content = re.sub(r'/\*.*?\*/', '', css_content, flags=re.DOTALL)
    css_content = re.sub(r'\s+', ' ', css_content)
    css_content = re.sub(r'\s*([{};:,>+~])\s*', r'\1', css_content)
    css_content = re.sub(r';}', '}', css_content)
    css_content = re.sub(r'\s*!\s*important', '!important', css_content)
    return css_content.strip()

def remove_line_comment(line):
    result = []
    i = 0
    in_string = False
    string_char = None
    length = len(line)
    
    while i < length:
        ch = line[i]
        
        if in_string:
            result.append(ch)
            if ch == '\\' and i + 1 < length:
                result.append(line[i + 1])
                i += 2
                continue
            if ch == string_char:
                in_string = False
                string_char = None
            i += 1
        else:
            if ch in '"\'`':
                in_string = True
                string_char = ch
                result.append(ch)
                i += 1
            elif ch == '/' and i + 1 < length and line[i + 1] == '/':
                break
            else:
                result.append(ch)
                i += 1
    
    return ''.join(result)

def minify_js_simple(js_content):
    lines = js_content.split('\n')
    result_lines = []
    in_block_comment = False
    
    for line in lines:
        if in_block_comment:
            if '*/' in line:
                in_block_comment = False
                idx = line.find('*/')
                line = line[idx + 2:]
            else:
                continue
        
        if '/*' in line:
            start = line.find('/*')
            end = line.find('*/', start + 2)
            if end != -1:
                line = line[:start] + line[end + 2:]
            else:
                in_block_comment = True
                line = line[:start]
        
        stripped = line.strip()
        if stripped == '':
            continue
        
        cleaned = remove_line_comment(line)
        cleaned = cleaned.rstrip()
        if cleaned.strip():
            result_lines.append(cleaned)
    
    js_content = '\n'.join(result_lines)
    js_content = re.sub(r'\n\s*\n', '\n', js_content)
    
    return js_content

def process_file(filepath, minify_func, output_ext='.min'):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    minified = minify_func(content)
    
    base, ext = os.path.splitext(filepath)
    output_path = base + output_ext + ext
    
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(minified)
    
    original_size = len(content)
    minified_size = len(minified)
    ratio = (1 - minified_size / original_size) * 100
    
    print(f"{filepath}")
    print(f"  -> {output_path}")
    print(f"  Original: {original_size:,} bytes")
    print(f"  Minified: {minified_size:,} bytes")
    print(f"  Saved: {ratio:.1f}%")
    print()
    
    return output_path

if __name__ == '__main__':
    docs_dir = os.path.dirname(os.path.abspath(__file__))
    
    css_files = [
        os.path.join(docs_dir, 'css', 'style.css'),
        os.path.join(docs_dir, 'css', 'help.css'),
    ]
    
    js_files = [
        os.path.join(docs_dir, 'js', 'flash.js'),
    ]
    
    print("=" * 50)
    print("Minifying CSS files...")
    print("=" * 50)
    for f in css_files:
        if os.path.exists(f):
            process_file(f, minify_css)
    
    print("=" * 50)
    print("Minifying JS files...")
    print("=" * 50)
    for f in js_files:
        if os.path.exists(f):
            process_file(f, minify_js_simple)
    
    print("Done!")

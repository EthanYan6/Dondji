#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Scan App/*.c for CJK in string literals; compare with extract_chinese_font MENU_CHARS."""

import re
from pathlib import Path


def strip_c_comments(text):
    out = []
    i = 0
    n = len(text)
    in_block = False
    while i < n:
        if in_block:
            if i + 1 < n and text[i : i + 2] == "*/":
                in_block = False
                i += 2
            else:
                i += 1
            continue
        if i + 1 < n and text[i : i + 2] == "//":
            while i < n and text[i] != "\n":
                i += 1
            continue
        if i + 1 < n and text[i : i + 2] == "/*":
            in_block = True
            i += 2
            continue
        out.append(text[i])
        i += 1
    return "".join(out)


def decode_c_string_to_bytes(s):
    out = bytearray()
    i = 0
    while i < len(s):
        c = s[i]
        if c == "\\" and i + 1 < len(s):
            nxt = s[i + 1]
            if nxt == "x" and i + 3 < len(s):
                hx = s[i + 2 : i + 4]
                if all(ch in "0123456789abcdefABCDEF" for ch in hx):
                    out.append(int(hx, 16))
                    i += 4
                    continue
            if nxt == "n":
                out.append(10)
                i += 2
                continue
            if nxt == "r":
                out.append(13)
                i += 2
                continue
            if nxt == "t":
                out.append(9)
                i += 2
                continue
            if nxt == "0" and i + 2 < len(s) and s[i + 2] in "01234567":
                j = i + 2
                while j < len(s) and j < i + 5 and s[j] in "01234567":
                    j += 1
                out.append(int(s[i + 2 : j], 8) & 0xFF)
                i = j
                continue
            out.append(ord(nxt))
            i += 2
            continue
        o = ord(c)
        if o < 128:
            out.append(o)
            i += 1
        else:
            for bb in c.encode("utf-8"):
                out.append(bb)
            i += 1
    return bytes(out)


def is_cjk(ch):
    o = ord(ch)
    if 0x4E00 <= o <= 0x9FFF:
        return True
    if 0x3400 <= o <= 0x4DBF:
        return True
    if 0x3000 <= o <= 0x303F:
        return True
    if 0xFF00 <= o <= 0xFFEF:
        return True
    return False


def extract_strings(src):
    pat = re.compile(r'(?:u8)?"((?:[^"\\]|\\.)*)"', re.DOTALL)
    return [m.group(1) for m in pat.finditer(src)]


def collect_used_from_app(app_root: Path):
    used = set()
    for pattern in ("*.c", "*.h"):
        for path in app_root.rglob(pattern):
            raw = path.read_text(encoding="utf-8", errors="replace")
            body = strip_c_comments(raw)
            for lit in extract_strings(body):
                byte_buf = decode_c_string_to_bytes(lit)
                try:
                    decoded = byte_buf.decode("utf-8")
                except UnicodeDecodeError:
                    decoded = byte_buf.decode("utf-8", errors="replace")
                for ch in decoded:
                    if is_cjk(ch):
                        used.add(ch)
    return used


def menu_chars_needing_chinese_glyph(menu_text):
    """MENU_CHARS 里仅中文字模相关字符（排除 ASCII 如 ROGER、#）。"""
    result = set()
    for ch in menu_text.replace("\n", "").replace(" ", ""):
        if is_cjk(ch) or ord(ch) > 0x7F:
            result.add(ch)
    return result


def main():
    script_dir = Path(__file__).resolve().parent
    app_root = script_dir.parent
    used = collect_used_from_app(app_root)

    import extract_chinese_font as ecf

    menu_text = ecf.MENU_CHARS
    menu_cjk = menu_chars_needing_chinese_glyph(menu_text)
    only_old = menu_cjk - used
    only_used = used - menu_cjk

    minimal_line = "".join(sorted(used))
    lines = [
        "used CJK count: %d" % len(used),
        "in MENU(CJK) not in code count: %d" % len(only_old),
        "in MENU(CJK) not in code: %s" % "".join(sorted(only_old)),
        "in code not in MENU count: %d" % len(only_used),
        "in code not in MENU: %s" % "".join(sorted(only_used)),
        "--- minimal MENU_CHARS body (sorted, one line) ---",
        minimal_line,
    ]
    for line in lines:
        print(line)


if __name__ == "__main__":
    main()

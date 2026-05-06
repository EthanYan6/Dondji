#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""一次性检查 gen_cn_font.py 中 PINYIN_MAP：重复键、与 pypinyin 默认音差异。"""

from __future__ import annotations

import ast
import re
import sys
from collections import Counter
from pathlib import Path


def strip_digits(pinyin_segment):
    result_chars = []
    for single_char in pinyin_segment:
        if not single_char.isdigit():
            result_chars.append(single_char)
    return "".join(result_chars)


def main():
    script_dir = Path(__file__).resolve().parent
    gen_path = script_dir / "gen_cn_font.py"
    source_text = gen_path.read_text(encoding="utf-8")

    start_marker = "PINYIN_MAP = {"
    start_index = source_text.index(start_marker)
    brace_start = start_index + len("PINYIN_MAP = ")
    depth = 0
    scan_index = brace_start
    while scan_index < len(source_text):
        if source_text[scan_index] == "{":
            depth += 1
        elif source_text[scan_index] == "}":
            depth -= 1
            if depth == 0:
                end_index = scan_index + 1
                break
        scan_index += 1
    else:
        print("未找到 PINYIN_MAP 闭合")
        sys.exit(1)

    dict_literal_text = source_text[brace_start:end_index]
    pinyin_map_dict = ast.literal_eval(dict_literal_text)

    key_value_pattern = re.compile(r"'([^'])':\s*'([^']*)'")
    raw_pairs = []
    for match in key_value_pattern.finditer(dict_literal_text):
        key_char = match.group(1)
        value_raw = match.group(2)
        raw_pairs.append((key_char, value_raw))

    occurrence_counter = Counter(pair[0] for pair in raw_pairs)
    duplicate_key_chars = sorted(
        single_char for single_char, count in occurrence_counter.items() if count > 1
    )

    try:
        from pypinyin import Style, pinyin
    except ImportError:
        print("请先安装: pip install pypinyin")
        sys.exit(1)

    mismatch_default_list = []
    for single_char, py_raw in pinyin_map_dict.items():
        base_parts = []
        for comma_part in py_raw.split(","):
            stripped_segment = strip_digits(comma_part.strip())
            if stripped_segment:
                base_parts.append(stripped_segment)
        reference_default = pinyin(single_char, style=Style.NORMAL)[0][0]
        matched_any = False
        for base_py in base_parts:
            if base_py == reference_default:
                matched_any = True
                break
        if not matched_any and base_parts:
            mismatch_default_list.append(
                (single_char, py_raw, reference_default, base_parts)
            )

    print("PINYIN_MAP 唯一键数量:", len(pinyin_map_dict))
    print(
        "源码字面量中重复定义的字（后者覆盖前者）:",
        len(duplicate_key_chars),
        "个",
    )
    if duplicate_key_chars:
        for dup_char in duplicate_key_chars:
            print(
                "  ",
                repr(dup_char),
                "出现",
                occurrence_counter[dup_char],
                "次",
            )

    print(
        "与 pypinyin 默认读音（Style.NORMAL）不一致的条目:",
        len(mismatch_default_list),
        "条（多为 intentional：地名、多音字；少数可能是笔误）",
    )
    for item in mismatch_default_list:
        single_char = item[0]
        py_raw = item[1]
        reference_default = item[2]
        base_parts = item[3]
        code_point = ord(single_char)
        print(
            "  %s U+%04X  MAP=%s -> %s | pypinyin默认=%s"
            % (single_char, code_point, py_raw, base_parts, reference_default)
        )


if __name__ == "__main__":
    main()

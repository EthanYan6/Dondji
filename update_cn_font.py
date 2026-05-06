#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
一键：追加汉字 → 运行 gen_cn_font.py → 同步固件/网页常量。

默认流程：
  1.（可选）把 --append 中的字追加到 App/tools/cn_chars_append.txt
  2. 运行 App/tools/gen_cn_font.py（生成 cn_font_data.h、docs/font(s)/cn_font.bin）
  3. 从 App/cn_font_data.h 读取 CN_FONT_*，写回 App/settings.h、docs/js/flash.js
  4. 更新 docs/index.html 中「支持 N 个中文字符」的 N

依赖：与 gen_cn_font.py 相同（pypinyin），请先 pip install pypinyin。

新字若拼音候选不对，请在 App/tools/gen_cn_font.py 的 PINYIN_MAP 中补充后再运行本脚本。
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


HEADER_DEFINE_NAMES_ORDERED = [
    "CN_FONT_FLASH_BASE",
    "CN_FONT_CHAR_COUNT",
    "CN_FONT_BITMAP_SIZE",
    "CN_FONT_INDEX_SIZE",
    "CN_FONT_PY_OFFSET",
    "CN_FONT_PY_COUNT",
    "CN_FONT_VERSION",
    "CN_FONT_VERSION_OFFSET",
    "CN_FONT_PY_TOTAL_SIZE",
]


def get_repository_root_path():
    script_path = Path(__file__).resolve()
    repository_root = script_path.parent
    return repository_root


def read_text_file_utf8(file_path):
    text_content = file_path.read_text(encoding="utf-8")
    return text_content


def write_text_file_utf8(file_path, text_content):
    file_path.write_text(text_content, encoding="utf-8", newline="\n")


def parse_cn_font_header_defines(header_text):
    define_values = {}
    for define_name in HEADER_DEFINE_NAMES_ORDERED:
        if define_name == "CN_FONT_FLASH_BASE":
            pattern = r"^#define\s+CN_FONT_FLASH_BASE\s+(0x[0-9A-Fa-f]+)u"
        else:
            pattern = r"^#define\s+" + re.escape(define_name) + r"\s+(\d+)u"
        match = re.search(pattern, header_text, re.MULTILINE)
        if not match:
            message = "无法在 cn_font_data.h 中解析: " + define_name
            raise ValueError(message)
        raw_value = match.group(1)
        define_values[define_name] = raw_value
    return define_values


def build_settings_h_cn_font_define_lines(define_values):
    flash_base_token = define_values["CN_FONT_FLASH_BASE"]
    char_count_token = define_values["CN_FONT_CHAR_COUNT"]
    bitmap_size_token = define_values["CN_FONT_BITMAP_SIZE"]
    index_size_token = define_values["CN_FONT_INDEX_SIZE"]
    py_offset_token = define_values["CN_FONT_PY_OFFSET"]
    py_count_token = define_values["CN_FONT_PY_COUNT"]
    version_token = define_values["CN_FONT_VERSION"]
    version_offset_token = define_values["CN_FONT_VERSION_OFFSET"]
    py_total_token = define_values["CN_FONT_PY_TOTAL_SIZE"]

    line_flash = "#define CN_FONT_FLASH_BASE      %su" % flash_base_token
    line_char = "#define CN_FONT_CHAR_COUNT      %su" % char_count_token
    line_bitmap = "#define CN_FONT_BITMAP_SIZE     %su" % bitmap_size_token
    line_index = "#define CN_FONT_INDEX_SIZE      %su" % index_size_token
    line_py_off = "#define CN_FONT_PY_OFFSET       %su" % py_offset_token
    line_py_cnt = "#define CN_FONT_PY_COUNT        %su" % py_count_token
    line_ver = "#define CN_FONT_VERSION         %su" % version_token
    line_ver_off = "#define CN_FONT_VERSION_OFFSET  %su" % version_offset_token
    line_py_tot = "#define CN_FONT_PY_TOTAL_SIZE   %su" % py_total_token

    define_lines = [
        line_flash,
        line_char,
        line_bitmap,
        line_index,
        line_py_off,
        line_py_cnt,
        line_ver,
        line_ver_off,
        line_py_tot,
    ]
    define_text = "\n".join(define_lines) + "\n"
    return define_text


def replace_settings_h_cn_font_block(settings_text, define_lines_text):
    anchor_start = "// CN font SPI Flash layout (data written via web tool)\n"
    anchor_note = "// NOTE: these must match the output of gen_cn_font.py / cn_font_data.h\n"
    block_pattern = re.compile(
        re.escape(anchor_start)
        + re.escape(anchor_note)
        + r"(?:#define CN_FONT_[^\n]+\n)+",
        re.MULTILINE,
    )
    match = block_pattern.search(settings_text)
    if not match:
        raise ValueError("未在 App/settings.h 中找到 CN_FONT_* 定义块（注释锚点不匹配）。")
    replacement_text = anchor_start + anchor_note + define_lines_text
    updated_text = settings_text[: match.start()] + replacement_text + settings_text[match.end() :]
    return updated_text


def replace_flash_js_constants(flash_js_text, define_values):
    version_offset_value = define_values["CN_FONT_VERSION_OFFSET"]
    bitmap_size_value = define_values["CN_FONT_BITMAP_SIZE"]
    char_count_value = define_values["CN_FONT_CHAR_COUNT"]
    version_value = define_values["CN_FONT_VERSION"]

    updated_text = flash_js_text
    updated_text = re.sub(
        r"^const CN_FONT_VERSION_OFFSET = \d+;",
        "const CN_FONT_VERSION_OFFSET = %s;" % version_offset_value,
        updated_text,
        count=1,
        flags=re.MULTILINE,
    )
    updated_text = re.sub(
        r"^const CN_FONT_BITMAP_SIZE = \d+;",
        "const CN_FONT_BITMAP_SIZE = %s;" % bitmap_size_value,
        updated_text,
        count=1,
        flags=re.MULTILINE,
    )
    updated_text = re.sub(
        r"^const CN_FONT_CHAR_COUNT = \d+;",
        "const CN_FONT_CHAR_COUNT = %s;" % char_count_value,
        updated_text,
        count=1,
        flags=re.MULTILINE,
    )
    updated_text = re.sub(
        r"^const CN_FONT_VERSION\s*=\s*\d+;",
        "const CN_FONT_VERSION     = %s;" % version_value,
        updated_text,
        count=1,
        flags=re.MULTILINE,
    )
    return updated_text


def replace_index_html_char_count(index_html_text, char_count_int):
    count_string = str(char_count_int)
    pattern = r"(支持 )\d+( 个中文字符)"
    match = re.search(pattern, index_html_text)
    if not match:
        raise ValueError(
            "未在 docs/index.html 中匹配到「支持 N 个中文字符」，请手动改页面文案。"
        )
    replacement = r"\g<1>" + count_string + r"\g<2>"
    updated_text = re.sub(pattern, replacement, index_html_text, count=1)
    return updated_text


def append_chars_to_append_file(append_file_path, chars_to_append):
    if not chars_to_append:
        return
    append_file_path.parent.mkdir(parents=True, exist_ok=True)
    existing_text = ""
    if append_file_path.is_file():
        existing_text = append_file_path.read_text(encoding="utf-8")
    with open(append_file_path, "a", encoding="utf-8") as append_file:
        if existing_text and not existing_text.endswith("\n"):
            append_file.write("\n")
        append_file.write(chars_to_append)
        append_file.write("\n")
    print("已追加到 %s" % append_file_path)


def run_gen_cn_font(repository_root):
    gen_script_path = repository_root / "App" / "tools" / "gen_cn_font.py"
    completed = subprocess.run(
        [sys.executable, str(gen_script_path)],
        cwd=str(repository_root),
        check=False,
    )
    exit_code = completed.returncode
    if exit_code != 0:
        raise RuntimeError("gen_cn_font.py 退出码: %d" % exit_code)


def verify_cn_font_bin_matches_header(repository_root, define_values):
    """
    校验 docs/font/cn_font.bin 与头文件宏一致，并扫描拼音区能否解析且含 zhong。
    用于尽早发现「脚本未同步」或 bin 损坏；避免刷机后拼音检索整体失效。
    """
    bin_font_path = repository_root / "docs" / "font" / "cn_font.bin"
    bin_fonts_path = repository_root / "docs" / "fonts" / "cn_font.bin"
    if not bin_font_path.is_file():
        print("跳过 cn_font.bin 校验：未找到 %s" % bin_font_path)
        return

    bin_bytes = bin_font_path.read_bytes()
    version_offset_int = int(define_values["CN_FONT_VERSION_OFFSET"])
    expected_byte_length = version_offset_int + 1
    actual_byte_length = len(bin_bytes)
    if actual_byte_length != expected_byte_length:
        length_mismatch_message = (
            "cn_font.bin 长度 %d 与 CN_FONT_VERSION_OFFSET+1=%d 不符"
            % (actual_byte_length, expected_byte_length)
        )
        raise RuntimeError(length_mismatch_message)

    if bin_fonts_path.is_file():
        alt_bytes = bin_fonts_path.read_bytes()
        if alt_bytes != bin_bytes:
            fonts_mismatch_message = (
                "docs/font/cn_font.bin 与 docs/fonts/cn_font.bin 内容不一致，"
                "请重新运行 App/tools/gen_cn_font.py"
            )
            raise RuntimeError(fonts_mismatch_message)

    py_offset_int = int(define_values["CN_FONT_PY_OFFSET"])
    py_total_int = int(define_values["CN_FONT_PY_TOTAL_SIZE"])
    py_count_int = int(define_values["CN_FONT_PY_COUNT"])

    relative_offset = 0
    found_zhong = False
    for syllable_index in range(py_count_int):
        if relative_offset >= py_total_int:
            overflow_message = (
                "拼音表在第 %d 个音节前已越过 PY_TOTAL（relative_offset=%d）"
                % (syllable_index, relative_offset)
            )
            raise RuntimeError(overflow_message)
        absolute_length_pos = py_offset_int + relative_offset
        string_length = bin_bytes[absolute_length_pos]
        relative_offset += 1
        syllable_start = py_offset_int + relative_offset
        syllable_end = syllable_start + string_length
        syllable_text = bin_bytes[syllable_start:syllable_end].decode("ascii")
        relative_offset += string_length
        char_count_pos = py_offset_int + relative_offset
        char_count_byte = bin_bytes[char_count_pos]
        relative_offset += 1
        index_payload_bytes = char_count_byte * 2
        relative_offset += index_payload_bytes
        if syllable_text == "zhong":
            found_zhong = True

    if relative_offset != py_total_int:
        total_mismatch_message = (
            "拼音表解析累计字节 %d 与 CN_FONT_PY_TOTAL_SIZE=%d 不符"
            % (relative_offset, py_total_int)
        )
        raise RuntimeError(total_mismatch_message)

    if not found_zhong:
        raise RuntimeError("校验失败：拼音数据中未找到音节 zhong")

    print(
        "校验通过：cn_font.bin 长度、拼音区解析与音节 zhong 均与 cn_font_data.h 一致。"
    )


def print_post_run_summary(
    did_run_generator,
    did_append_argument,
    char_count_int,
    cn_font_bin_bytes,
):
    """
    执行结束后在终端打印：脚本已做事项 + 用户仍需跟进的项。
    """
    separator_line = "-" * 56
    print("")
    print(separator_line)
    print("【本脚本已自动完成】")
    if did_append_argument:
        print("  - 已将 --append 内容追加到 App/tools/cn_chars_append.txt")
    if did_run_generator:
        print("  - 已运行 gen_cn_font.py：生成/更新 App/cn_font_data.h")
        print("  - 已生成 docs/font/cn_font.bin，并复制到 docs/fonts/cn_font.bin")
    else:
        print("  - 未运行生成（你使用了 --no-generate），沿用现有 cn_font_data.h")
        print("    （磁盘上的 cn_font.bin 若未单独生成，可能与头文件不一致，请核对）")
    print("  - 已从 cn_font_data.h 同步宏到 App/settings.h（CN_FONT_*）")
    print("  - 已同步 docs/js/flash.js 中与字库布局相关的常量")
    print(
        "  - 已更新 docs/index.html 中「支持 %d 个中文字符」"
        % char_count_int
    )
    print("")
    print("【你还需要手动做】")
    print("  1. 若信道拼音输入法里某新字候选不对：编辑 App/tools/gen_cn_font.py")
    print("     中的 PINYIN_MAP，保存后再执行：python update_cn_font.py")
    print("  2. 若 README.md、README.zh.md、README.en.md、docs/fonts/README.md")
    print("     里仍写旧字数/旧 bin 体积，需要自行改成与当前一致（脚本不改这些）")
    print("  3. 用更新后的 App/settings.h 重新编译固件并烧录到对讲机（布局常量变了必须刷固件）")
    print("  4. 再刷入或与站点一同部署与当前头文件一致的 cn_font.bin（font/ 与 fonts/ 两处副本须一致）")
    print(
        "     按当前 cn_font_data.h，bin 文件长度应为 %d 字节（CN_FONT_VERSION_OFFSET+1）"
        % cn_font_bin_bytes
    )
    print(
        "     若曾用「旧版网页」刷字库：旧 flash.js 可能把版本字节写到错误地址并破坏拼音区；"
        "请部署当前 docs/js/flash.js 后强制刷新（Ctrl+F5）再刷字库。"
    )
    print("  5. git 提交本次变更的文件后再推送/发布页面")
    print(separator_line)
    print("")


def sync_derived_files(repository_root, define_values):
    char_count_string = define_values["CN_FONT_CHAR_COUNT"]
    char_count_int = int(char_count_string)

    settings_path = repository_root / "App" / "settings.h"
    settings_text = read_text_file_utf8(settings_path)
    settings_define_lines = build_settings_h_cn_font_define_lines(define_values)
    new_settings_text = replace_settings_h_cn_font_block(settings_text, settings_define_lines)
    write_text_file_utf8(settings_path, new_settings_text)
    print("已更新: %s" % settings_path)

    flash_js_path = repository_root / "docs" / "js" / "flash.js"
    flash_js_text = read_text_file_utf8(flash_js_path)
    new_flash_text = replace_flash_js_constants(flash_js_text, define_values)
    write_text_file_utf8(flash_js_path, new_flash_text)
    print("已更新: %s" % flash_js_path)

    index_html_path = repository_root / "docs" / "index.html"
    index_html_text = read_text_file_utf8(index_html_path)
    new_index_text = replace_index_html_char_count(index_html_text, char_count_int)
    write_text_file_utf8(index_html_path, new_index_text)
    print("已更新: %s（字数 %d）" % (index_html_path, char_count_int))


def main():
    parser = argparse.ArgumentParser(
        description="追加字库并同步 cn_font_data.h → settings.h / flash.js / index.html"
    )
    parser.add_argument(
        "--append",
        default="",
        help="可选：要追加的汉字（写入 App/tools/cn_chars_append.txt 后再生成）",
    )
    parser.add_argument(
        "--no-generate",
        action="store_true",
        help="不运行 gen_cn_font.py，仅根据现有 App/cn_font_data.h 同步其它文件",
    )
    args = parser.parse_args()

    repository_root = get_repository_root_path()
    append_file_path = repository_root / "App" / "tools" / "cn_chars_append.txt"
    header_path = repository_root / "App" / "cn_font_data.h"

    append_argument = args.append
    if append_argument:
        append_chars_to_append_file(append_file_path, append_argument)

    skip_generate = args.no_generate
    if not skip_generate:
        run_gen_cn_font(repository_root)

    header_text = read_text_file_utf8(header_path)
    define_values = parse_cn_font_header_defines(header_text)
    sync_derived_files(repository_root, define_values)
    verify_cn_font_bin_matches_header(repository_root, define_values)

    version_offset_token = define_values["CN_FONT_VERSION_OFFSET"]
    version_offset_int = int(version_offset_token)
    cn_font_bin_byte_count = version_offset_int + 1
    char_count_for_summary = int(define_values["CN_FONT_CHAR_COUNT"])

    did_run_gen_flag = not skip_generate
    did_append_flag = bool(append_argument)
    print_post_run_summary(
        did_run_gen_flag,
        did_append_flag,
        char_count_for_summary,
        cn_font_bin_byte_count,
    )


if __name__ == "__main__":
    main()

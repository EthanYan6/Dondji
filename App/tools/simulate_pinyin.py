#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simulate the pinyin candidate retrieval logic
"""

def simulate_get_candidates(pinyin, max_count, start_offset):
    """
    Simulate SETTINGS_CNGetPinyinCandidates
    Returns: (total_count, filled_count, unicode_list)
    """
    # Simulate "wei" with 60 characters
    char_count = 60
    total = char_count
    count = 0
    unicode_list = []
    
    for j in range(char_count):
        # Simulate reading unicode
        unicode_val = 0x4E3A + j  # Just for simulation
        
        if j >= start_offset and count < max_count:
            unicode_list.append(unicode_val)
            count += 1
    
    return total, count, unicode_list

def simulate_menu_pinyin_search(offset):
    """
    Simulate MENU_PinyinSearch
    """
    CN_CANDIDATE_MAX = 6
    
    raw_total, filled_count, unicode_list = simulate_get_candidates("wei", CN_CANDIDATE_MAX, offset)
    
    gCNCandidateTotal = raw_total if raw_total >= 0 else 0
    gCNCandidateCount = gCNCandidateTotal - offset
    
    if gCNCandidateCount > CN_CANDIDATE_MAX:
        gCNCandidateCount = CN_CANDIDATE_MAX
    
    print(f"Offset: {offset}")
    print(f"  raw_total: {raw_total}")
    print(f"  gCNCandidateTotal: {gCNCandidateTotal}")
    print(f"  gCNCandidateCount: {gCNCandidateCount}")
    print(f"  filled_count: {filled_count}")
    print(f"  unicode_list length: {len(unicode_list)}")
    print(f"  unicode_list: {[hex(u) for u in unicode_list]}")
    print()

# Test different pages
print("Page 1 (offset=0):")
simulate_menu_pinyin_search(0)

print("Page 2 (offset=6):")
simulate_menu_pinyin_search(6)

print("Page 3 (offset=12):")
simulate_menu_pinyin_search(12)

print("Page 10 (offset=54):")
simulate_menu_pinyin_search(54)

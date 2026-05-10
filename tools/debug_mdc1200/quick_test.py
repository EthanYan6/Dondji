#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MDC1200 快速测试脚本 - 命令行版本
用于快速检查音频文件和生成调试建议
"""

import sys
import os

# 添加调试器模块路径
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from mdc1200_debugger import MDC1200Analyzer


def print_color(text, color_code=32):
    """彩色输出"""
    print(f"\033[{color_code}m{text}\033[0m")


def quick_test(filepath):
    """快速测试"""
    print("=" * 70)
    print_color("MDC1200 快速测试", 36)
    print("=" * 70)
    
    analyzer = MDC1200Analyzer()
    
    # 加载文件
    print(f"\n加载文件：{filepath}")
    if not analyzer.load_wav_file(filepath):
        print_color("✗ 加载失败", 31)
        return False
    
    print_color(f"✓ 加载成功 - 采样率：{analyzer.sample_rate}Hz, 时长：{len(analyzer.recorded_data)/analyzer.sample_rate:.2f}s", 32)
    
    # 频谱分析
    print("\n" + "-" * 70)
    print_color("频谱分析", 33)
    print("-" * 70)
    
    tones = analyzer.detect_fsk_tones()
    if not tones:
        print_color("✗ 未检测到明显的音调频率", 31)
    else:
        print("检测到的频率成分:")
        for freq, amp in tones:
            marker = ""
            color = 37
            if abs(freq - 4700) < 300:
                marker = "← Mark 频率"
                color = 32
            elif abs(freq - 2300) < 300:
                marker = "← Space 频率"
                color = 32
            elif abs(freq - 1200) < 200:
                marker = "← 载波中心"
                color = 36
            print_color(f"  {freq:7.1f} Hz  (幅度：{amp:.2e})  {marker}", color)
    
    # 频偏测量
    print("\n" + "-" * 70)
    print_color("频偏测量", 33)
    print("-" * 70)
    
    dev = analyzer.measure_frequency_deviation()
    if not dev:
        print_color("✗ 无法测量频偏", 31)
    else:
        print(f"  中心频率：{dev['center_freq']:.1f} Hz")
        print(f"  最大频率：{dev['max_freq']:.1f} Hz")
        print(f"  最小频率：{dev['min_freq']:.1f} Hz")
        print(f"  实测频偏：{dev['deviation']:.1f} Hz")
        print(f"  标准频偏：3500 Hz")
        
        error = dev['deviation_error']
        if abs(error) < 300:
            print_color(f"  ✓ 频偏正常 (误差：{error:+.1f} Hz)", 32)
        elif abs(error) < 800:
            print_color(f"  ⚠ 频偏略有偏差 (误差：{error:+.1f} Hz)", 33)
        else:
            print_color(f"  ✗ 频偏异常 (误差：{error:+.1f} Hz)", 31)
    
    # 生成建议
    print("\n" + "-" * 70)
    print_color("调试建议", 33)
    print("-" * 70)
    
    if dev:
        error = dev['deviation_error']
        
        if abs(error) > 1000:
            print_color("\n【严重问题】频偏偏差过大！", 31)
            print("\n建议措施:")
            print("  1. 修改 bk4829.c 中的频偏设置:")
            print("     在 BK4819_PlayMDC1200 函数中，找到:")
            print("       uint16_t deviation = 850;")
            
            if error > 0:
                print(f"     修改为：uint16_t deviation = {int(3500 - error)};  // 减小频偏")
            else:
                print(f"     修改为：uint16_t deviation = {int(3500 - error)};  // 增大频偏")
            
            print("  2. 重新编译固件")
            print("  3. 再次测试验证")
            
        elif abs(error) > 400:
            print_color("\n【轻微问题】频偏略有偏差", 33)
            print("\n建议措施:")
            print("  可以微调频偏设置以匹配标准:")
            print(f"     当前值：850")
            print(f"     建议值：{int(850 * 3500 / dev['deviation'])}")
        
        else:
            print_color("\n【正常】频偏在可接受范围内", 32)
            print("\n如果仍有通信问题，请检查:")
            print("  1. 接收带宽设置 (REG_58)")
            print("  2. 接收增益设置 (REG_58)")
            print("  3. 同步头检测算法")
    
    # 检查 Mark/Space
    mark_found = any(abs(t[0] - 4700) < 300 for t in tones)
    space_found = any(abs(t[0] - 2300) < 300 for t in tones)
    
    print("\n" + "-" * 70)
    print_color("调制检查", 33)
    print("-" * 70)
    
    if mark_found and space_found:
        print_color("✓ FSK 调制正常 (Mark 和 Space 频率都存在)", 32)
    elif mark_found:
        print_color("⚠ 只检测到 Mark 频率，Space 频率缺失或太弱", 33)
        print("  可能原因:")
        print("    - 频偏设置过小")
        print("    - 音频路径非线性失真")
        print("    - 调制深度不足")
    elif space_found:
        print_color("⚠ 只检测到 Space 频率，Mark 频率缺失或太弱", 33)
        print("  可能原因:")
        print("    - 频偏设置异常")
        print("    - 载波频率偏移")
    else:
        print_color("✗ 未检测到 FSK 调制特征", 31)
        print("  可能原因:")
        print("    - 信号不是 FSK 调制")
        print("    - 信号质量太差")
        print("    - 录音有问题")
    
    # 最终评估
    print("\n" + "=" * 70)
    print_color("总体评估", 36)
    print("=" * 70)
    
    score = 0
    max_score = 100
    
    if dev and abs(dev['deviation_error']) < 300:
        score += 40
    elif dev and abs(dev['deviation_error']) < 800:
        score += 20
    
    if mark_found:
        score += 20
    if space_found:
        score += 20
    
    if len(tones) > 0:
        score += 20
    
    print(f"得分：{score}/{max_score}")
    
    if score >= 80:
        print_color("评级：优秀 - 信号质量很好", 32)
    elif score >= 60:
        print_color("评级：良好 - 信号基本正常，可以优化", 36)
    elif score >= 40:
        print_color("评级：一般 - 需要调整参数", 33)
    else:
        print_color("评级：较差 - 存在严重问题", 31)
    
    print("\n" + "=" * 70)
    
    return True


def main():
    if len(sys.argv) < 2:
        print("MDC1200 快速测试工具")
        print("\n用法:")
        print("  python quick_test.py <audio_file.wav>")
        print("\n示例:")
        print("  python quick_test.py recording.wav")
        print("\n如果没有录音文件，可以先用调试工具录制:")
        print("  python mdc1200_debugger.py")
        sys.exit(1)
    
    filepath = sys.argv[1]
    
    if not os.path.exists(filepath):
        print_color(f"错误：文件不存在 - {filepath}", 31)
        sys.exit(1)
    
    quick_test(filepath)


if __name__ == "__main__":
    main()

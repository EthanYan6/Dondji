#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MDC1200 调试工具
用于分析和调试 MDC1200 信号的发射和接收
支持音频录制、分析、频偏测量、波形显示
"""

import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
from scipy.io import wavfile
import sounddevice as sd
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import threading
import queue
import os
import sys
from datetime import datetime
import struct

# MDC1200 标准参数
MDC1200_MARK_FREQ = 1200 + 3500  # 4700 Hz
MDC1200_SPACE_FREQ = 1200 - 3500  # -2300 Hz (实际是相位反转)
MDC1200_BAUD_RATE = 1200
MDC1200_SYNC = bytes([0x07, 0x09, 0x2a, 0x44, 0x6f])


class MDC1200Analyzer:
    """MDC1200 信号分析器"""
    
    def __init__(self, sample_rate=48000):
        self.sample_rate = sample_rate
        self.audio_queue = queue.Queue()
        self.is_recording = False
        self.recorded_data = None
        
    def record_audio(self, duration=5.0, device_index=None):
        """录制音频"""
        self.is_recording = True
        self.recorded_data = None
        
        def record_thread():
            try:
                print(f"开始录制 {duration} 秒...")
                data = sd.rec(int(duration * self.sample_rate), 
                             samplerate=self.sample_rate, 
                             channels=1,
                             dtype='float32',
                             device=device_index)
                sd.wait()
                self.recorded_data = data.flatten()
                print(f"录制完成，采样点数：{len(self.recorded_data)}")
            except Exception as e:
                print(f"录制失败：{e}")
                if hasattr(self, 'root'):
                    self.root.after(0, lambda: messagebox.showerror("错误", f"录音失败：{str(e)}"))
            finally:
                self.is_recording = False
        
        thread = threading.Thread(target=record_thread, daemon=True)
        thread.start()
        return thread
    
    def stop_recording(self):
        """停止录制"""
        sd.stop()
        self.is_recording = False
    
    def load_wav_file(self, filepath):
        """加载 WAV 文件"""
        try:
            sample_rate, data = wavfile.read(filepath)
            self.sample_rate = sample_rate
            if len(data.shape) > 1:
                data = data.mean(axis=1)  # 转为单声道
            self.recorded_data = data.astype('float32') / 32768.0  # 归一化
            return True
        except Exception as e:
            print(f"加载 WAV 文件失败：{e}")
            return False
    
    def compute_spectrum(self, data=None, nfft=2048):
        """计算频谱"""
        if data is None:
            data = self.recorded_data
        if data is None:
            return None, None
            
        frequencies, spectrum = signal.welch(data, self.sample_rate, nperseg=nfft)
        return frequencies, spectrum
    
    def detect_fsk_tones(self, data=None):
        """检测 FSK 音调频率"""
        if data is None:
            data = self.recorded_data
        if data is None:
            return []
        
        # 计算频谱
        frequencies, spectrum = self.compute_spectrum(data)
        
        # 寻找峰值
        peaks, properties = signal.find_peaks(spectrum, height=np.max(spectrum) * 0.1)
        
        # 返回主要频率成分
        detected_tones = []
        for peak in peaks:
            freq = frequencies[peak]
            if 500 < freq < 6000:  # 只关心 500-6000Hz 范围内的频率
                detected_tones.append((freq, spectrum[peak]))
        
        # 按幅度排序
        detected_tones.sort(key=lambda x: x[1], reverse=True)
        return detected_tones[:5]  # 返回前 5 个最强的频率
    
    def measure_frequency_deviation(self, data=None):
        """测量频偏"""
        if data is None:
            data = self.recorded_data
        if data is None:
            return None
        
        # 使用短时傅里叶变换分析瞬时频率
        nperseg = 256
        noverlap = 200
        f, t, Zxx = signal.stft(data, self.sample_rate, nperseg=nperseg, noverlap=noverlap)
        
        # 找到每个时间点的 dominant frequency
        dominant_freqs = []
        for i in range(Zxx.shape[1]):
            spectrum = np.abs(Zxx[:, i])
            peak_idx = np.argmax(spectrum)
            freq = f[peak_idx]
            if 500 < freq < 6000:
                dominant_freqs.append(freq)
        
        if len(dominant_freqs) < 10:
            return None
        
        # 计算频偏
        median_freq = np.median(dominant_freqs)
        max_freq = np.max(dominant_freqs)
        min_freq = np.min(dominant_freqs)
        deviation = (max_freq - min_freq) / 2
        
        return {
            'center_freq': median_freq,
            'max_freq': max_freq,
            'min_freq': min_freq,
            'deviation': deviation,
            'expected_deviation': 3500,
            'deviation_error': deviation - 3500
        }
    
    def demodulate_fsk(self, data=None):
        """FSK 解调"""
        if data is None:
            data = self.recorded_data
        if data is None:
            return None
        
        # 使用鉴频器解调
        delayed = np.roll(data, 1)
        delayed[0] = 0
        demodulated = data * delayed
        
        return demodulated
    
    def detect_sync(self, data=None):
        """检测同步头"""
        if data is None:
            data = self.recorded_data
        if data is None:
            return False
        
        # 简单的能量检测
        energy = np.sum(data ** 2)
        threshold = len(data) * 0.01  # 简单阈值
        
        return energy > threshold
    
    def plot_waveform(self, data=None, title="波形"):
        """绘制波形"""
        if data is None:
            data = self.recorded_data
        if data is None:
            return
        
        plt.figure(figsize=(12, 8))
        
        # 波形
        plt.subplot(3, 1, 1)
        time = np.arange(len(data)) / self.sample_rate
        plt.plot(time, data)
        plt.xlabel('时间 (s)')
        plt.ylabel('幅度')
        plt.title(f'{title} - 时域波形')
        plt.grid(True)
        
        # 频谱
        plt.subplot(3, 1, 2)
        frequencies, spectrum = self.compute_spectrum(data)
        plt.semilogy(frequencies, spectrum)
        plt.xlabel('频率 (Hz)')
        plt.ylabel('功率谱密度')
        plt.title('频谱分析')
        plt.grid(True)
        plt.xlim(0, 6000)
        
        # 语谱图
        plt.subplot(3, 1, 3)
        f, t, Sxx = signal.spectrogram(data, self.sample_rate, nperseg=512)
        plt.pcolormesh(t, f, 10 * np.log10(Sxx + 1e-10), shading='gouraud')
        plt.ylabel('频率 (Hz)')
        plt.xlabel('时间 (秒)')
        plt.title('语谱图')
        plt.ylim(0, 6000)
        plt.colorbar(label='dB')
        
        plt.tight_layout()
        plt.show()
    
    def analyze_and_report(self, data=None):
        """分析并生成报告"""
        if data is None:
            data = self.recorded_data
        if data is None:
            return "没有数据可分析"
        
        report = []
        report.append("=" * 60)
        report.append("MDC1200 信号分析报告")
        report.append("=" * 60)
        report.append(f"采样率：{self.sample_rate} Hz")
        report.append(f"数据长度：{len(data)} 采样点")
        report.append(f"时长：{len(data) / self.sample_rate:.3f} 秒")
        report.append("")
        
        # 检测音调
        tones = self.detect_fsk_tones(data)
        if tones:
            report.append("检测到的主要频率:")
            for freq, amp in tones:
                report.append(f"  {freq:.1f} Hz (幅度：{amp:.2e})")
            
            # 检查是否接近 MDC1200 标准
            mark_detected = any(abs(t[0] - 4700) < 500 for t in tones)
            space_detected = any(abs(t[0] - 2300) < 500 for t in tones)
            
            report.append("")
            if mark_detected:
                report.append("✓ 检测到 Mark 频率 (~4700 Hz)")
            else:
                report.append("✗ 未检测到 Mark 频率 (~4700 Hz)")
            
            if space_detected:
                report.append("✓ 检测到 Space 频率 (~2300 Hz)")
            else:
                report.append("✗ 未检测到 Space 频率 (~2300 Hz)")
        else:
            report.append("未检测到明显的音调频率")
        
        report.append("")
        
        # 测量频偏
        dev_result = self.measure_frequency_deviation(data)
        if dev_result:
            report.append("频偏测量:")
            report.append(f"  中心频率：{dev_result['center_freq']:.1f} Hz")
            report.append(f"  最大频率：{dev_result['max_freq']:.1f} Hz")
            report.append(f"  最小频率：{dev_result['min_freq']:.1f} Hz")
            report.append(f"  实测频偏：{dev_result['deviation']:.1f} Hz")
            report.append(f"  标准频偏：3500 Hz")
            report.append(f"  频偏误差：{dev_result['deviation_error']:+.1f} Hz")
            
            if abs(dev_result['deviation_error']) < 500:
                report.append("  ✓ 频偏正常")
            elif abs(dev_result['deviation_error']) < 1000:
                report.append("  ⚠ 频偏略有偏差，可能需要调整")
            else:
                report.append("  ✗ 频偏异常，需要调整 REG_40 设置")
        else:
            report.append("无法测量频偏")
        
        report.append("")
        report.append("=" * 60)
        
        return "\n".join(report)


class MDC1200DebuggerGUI:
    """MDC1200 调试工具 GUI"""
    
    def __init__(self, root):
        self.root = root
        self.root.title("MDC1200 调试工具")
        self.root.geometry("900x700")
        
        self.analyzer = MDC1200Analyzer()
        self.analyzer.root = root
        
        self.setup_ui()
        self.update_device_list()
        
    def setup_ui(self):
        """设置 UI"""
        # 主框架
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # 设备选择
        device_frame = ttk.LabelFrame(main_frame, text="音频设备", padding="5")
        device_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Label(device_frame, text="输入设备:").grid(row=0, column=0, padx=5)
        self.device_combo = ttk.Combobox(device_frame, width=50)
        self.device_combo.grid(row=0, column=1, padx=5)
        self.device_combo.bind('<<ComboboxSelected>>', self.on_device_selected)
        
        ttk.Button(device_frame, text="刷新设备", command=self.update_device_list).grid(row=0, column=2, padx=5)
        
        # 录制控制
        record_frame = ttk.LabelFrame(main_frame, text="录制控制", padding="5")
        record_frame.grid(row=1, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Label(record_frame, text="录制时长 (秒):").grid(row=0, column=0, padx=5)
        self.duration_var = tk.StringVar(value="5.0")
        ttk.Entry(record_frame, textvariable=self.duration_var, width=10).grid(row=0, column=1, padx=5)
        
        self.record_btn = ttk.Button(record_frame, text="开始录制", command=self.toggle_recording)
        self.record_btn.grid(row=0, column=2, padx=5)
        
        ttk.Button(record_frame, text="加载 WAV 文件", command=self.load_wav).grid(row=0, column=3, padx=5)
        
        # 分析控制
        analyze_frame = ttk.LabelFrame(main_frame, text="分析功能", padding="5")
        analyze_frame.grid(row=2, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)
        
        ttk.Button(analyze_frame, text="频谱分析", command=self.analyze_spectrum).grid(row=0, column=0, padx=5)
        ttk.Button(analyze_frame, text="频偏测量", command=self.measure_deviation).grid(row=0, column=1, padx=5)
        ttk.Button(analyze_frame, text="波形显示", command=self.show_waveform).grid(row=0, column=2, padx=5)
        ttk.Button(analyze_frame, text="完整报告", command=self.show_report).grid(row=0, column=3, padx=5)
        
        # 结果显示
        result_frame = ttk.LabelFrame(main_frame, text="分析结果", padding="5")
        result_frame.grid(row=3, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S), pady=5)
        
        self.result_text = tk.Text(result_frame, height=20, width=100)
        self.result_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        scrollbar = ttk.Scrollbar(result_frame, orient=tk.VERTICAL, command=self.result_text.yview)
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.result_text.configure(yscrollcommand=scrollbar.set)
        
        # 状态栏
        self.status_var = tk.StringVar(value="就绪")
        status_bar = ttk.Label(main_frame, textvariable=self.status_var, relief=tk.SUNKEN)
        status_bar.grid(row=4, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=5)
        
        # 配置网格权重
        main_frame.columnconfigure(0, weight=1)
        main_frame.rowconfigure(3, weight=1)
    
    def update_device_list(self):
        """更新设备列表"""
        try:
            devices = sd.query_devices()
            input_devices = [d['name'] for d in devices if d['max_input_channels'] > 0]
            self.device_combo['values'] = input_devices
            if input_devices:
                self.device_combo.current(0)
            self.status_var.set(f"找到 {len(input_devices)} 个输入设备")
        except Exception as e:
            self.status_var.set(f"错误：{str(e)}")
    
    def on_device_selected(self, event):
        """设备选择回调"""
        pass
    
    def toggle_recording(self):
        """切换录制状态"""
        if self.analyzer.is_recording:
            self.analyzer.stop_recording()
            self.record_btn.configure(text="开始录制")
            self.status_var.set("录制已停止")
        else:
            try:
                duration = float(self.duration_var.get())
                device_name = self.device_combo.get()
                device_index = None
                if device_name:
                    devices = sd.query_devices()
                    for i, d in enumerate(devices):
                        if d['name'] == device_name and d['max_input_channels'] > 0:
                            device_index = i
                            break
                
                self.analyzer.record_audio(duration, device_index)
                self.record_btn.configure(text="停止录制")
                self.status_var.set(f"正在录制 {duration} 秒...")
            except Exception as e:
                messagebox.showerror("错误", f"开始录制失败：{str(e)}")
    
    def load_wav(self):
        """加载 WAV 文件"""
        filepath = filedialog.askopenfilename(
            title="选择 WAV 文件",
            filetypes=[("WAV 文件", "*.wav"), ("所有文件", "*.*")]
        )
        if filepath:
            if self.analyzer.load_wav_file(filepath):
                self.status_var.set(f"已加载：{os.path.basename(filepath)}")
                self.result_text.insert(tk.END, f"已加载文件：{filepath}\n")
                self.result_text.insert(tk.END, f"采样率：{self.analyzer.sample_rate} Hz\n")
                self.result_text.insert(tk.END, f"数据长度：{len(self.analyzer.recorded_data)} 采样点\n\n")
            else:
                messagebox.showerror("错误", "加载 WAV 文件失败")
    
    def analyze_spectrum(self):
        """频谱分析"""
        if self.analyzer.recorded_data is None:
            messagebox.showwarning("警告", "请先录制或加载音频数据")
            return
        
        tones = self.analyzer.detect_fsk_tones()
        self.result_text.delete(1.0, tk.END)
        self.result_text.insert(tk.END, "频谱分析结果:\n\n")
        
        if tones:
            for freq, amp in tones:
                marker = ""
                if abs(freq - 4700) < 500:
                    marker = " ← Mark 频率"
                elif abs(freq - 2300) < 500:
                    marker = " ← Space 频率"
                self.result_text.insert(tk.END, f"{freq:.1f} Hz (幅度：{amp:.2e}){marker}\n")
        else:
            self.result_text.insert(tk.END, "未检测到明显的音调频率\n")
    
    def measure_deviation(self):
        """测量频偏"""
        if self.analyzer.recorded_data is None:
            messagebox.showwarning("警告", "请先录制或加载音频数据")
            return
        
        result = self.analyzer.measure_frequency_deviation()
        self.result_text.delete(1.0, tk.END)
        
        if result:
            report = self.analyzer.analyze_and_report()
            self.result_text.insert(tk.END, report)
        else:
            self.result_text.insert(tk.END, "无法测量频偏\n")
    
    def show_waveform(self):
        """显示波形"""
        if self.analyzer.recorded_data is None:
            messagebox.showwarning("警告", "请先录制或加载音频数据")
            return
        
        self.status_var.set("正在绘制波形...")
        self.root.update()
        
        try:
            self.analyzer.plot_waveform(title="录制的信号")
            self.status_var.set("波形已显示")
        except Exception as e:
            self.status_var.set(f"绘图失败：{str(e)}")
            messagebox.showerror("错误", f"绘图失败：{str(e)}")
    
    def show_report(self):
        """显示完整报告"""
        if self.analyzer.recorded_data is None:
            messagebox.showwarning("警告", "请先录制或加载音频数据")
            return
        
        report = self.analyzer.analyze_and_report()
        self.result_text.delete(1.0, tk.END)
        self.result_text.insert(tk.END, report)


def main():
    """主函数"""
    print("=" * 60)
    print("MDC1200 调试工具")
    print("=" * 60)
    print("\n正在启动 GUI...")
    
    try:
        root = tk.Tk()
        app = MDC1200DebuggerGUI(root)
        root.mainloop()
    except Exception as e:
        print(f"启动失败：{e}")
        print("\n尝试命令行模式...")
        
        # 命令行模式
        analyzer = MDC1200Analyzer()
        
        if len(sys.argv) > 1:
            # 从文件加载
            filepath = sys.argv[1]
            if analyzer.load_wav_file(filepath):
                report = analyzer.analyze_and_report()
                print(report)
                
                if len(sys.argv) > 2 and sys.argv[2] == '--plot':
                    analyzer.plot_waveform()
            else:
                print(f"无法加载文件：{filepath}")
        else:
            print("\n用法:")
            print("  python mdc1200_debugger.py              - 启动 GUI")
            print("  python mdc1200_debugger.py file.wav     - 分析 WAV 文件")
            print("  python mdc1200_debugger.py file.wav --plot - 分析并绘图")
            print("\n需要安装依赖:")
            print("  pip install numpy matplotlib scipy sounddevice pyinstaller")


if __name__ == "__main__":
    main()

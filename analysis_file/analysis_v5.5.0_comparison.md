# F4HWN Fusion v5.5.0 vs Dondji 对比分析报告

分析日期：2026-05-08
上游版本：[armel/uv-k1-k5v3-firmware-custom v5.5.0](https://github.com/armel/uv-k1-k5v3-firmware-custom/releases/tag/v5.5.0)
Dondji 分支：motorola_r7

---

## v5.5.0 Improved 项分析

### 1. Improved fast-scan detection and displayed metadata
**Dondji 状态：基本已实现，有小差异**
- 快速扫描通过 `ENABLE_FASTER_CHANNEL_SCAN` 标志启用，暂停延迟 90ms (`chFrScanner.c:257-261`)
- 扫描 UI (`ui/scanner.c`) 显示频率、CTCSS (`CTC:%u.%uHz`)、DCS (`DCS:D%03oN`)
- 与上游的差异：上游 v5.5.0 新增了 `ENABLE_FEAT_F4HWN_SCAN_FASTER` 和 scan-range 排除槽位等更高级的扫描功能，Dondji 使用的是较简单的快速扫描实现

### 2. Improved ChName editing with long-press behavior
**Dondji 状态：已实现**
- 信道名编辑支持多输入模式：小写、大写、数字、符号 (`menu.c:2033-2201`)
- 使用按键 1-6 候选选择符号，1-4 选择字母
- 上游改进的"长按行为"可能指长按快速切换字符类型，Dondji 的实现方式不同但功能完整

### 3. Improved spectrum analyzer persistence by saving additional parameters
**Dondji 状态：未确认是否有相同实现**
- 频谱分析仪设置通过 `SpectrumSettings` 结构体管理 (`spectrum.c`)
- 上游新增了更多参数的持久化保存（如 RSSI 触发电平、带宽等），需确认 Dondji 是否在 EEPROM 中保存了完整的频谱参数

### 4. Improved spectrum scan-range UI by showing bandwidth
**Dondji 状态：未实现**
- 上游在频谱扫描范围 UI 中显示带宽替代了之前的 `frequencyChangeStep`
- Dondji 的频谱 UI 仍使用 step 作为显示参数

### 5. Improved DCS display format (homologated index on 2 digits)
**Dondji 状态：已实现**
- CTCSS 使用 2 位数字显示认证索引：`sprintf(top_right_badge, "%02d/%02d", ...)` (`menu.c:3612-3637`)
- DCS 使用 3 位八进制格式 `D%03oN`/`D%03oI` (`menu.c:2034-2042`)
- `DCS_GetCtcssApprovedIndex()` 函数已实现认证索引计算 (`dcs.c:118-145`)

### 6. Improved secondary channel UX by displaying VFO_Lock while receiving on first channel
**Dondji 状态：部分实现，逻辑不同**
- `BITMAP_VFO_Lock` 已定义 (`bitmaps.h:40`)
- 显示条件：TX 频率检查失败 + `TX_LOCK == true` + `!FUNCTION_IsRx()` (`main.c:2729-2735`)
- **关键差异**：Dondji 的 VFO_Lock 显示基于 TX 频率合法性和 TX_LOCK 设置，**不是**专门针对双守时第一信道正在接收的场景。上游的改进是：当双守模式下第一个 VFO 正在接收时，第二个 VFO 应显示 `VFO_Lock` 提示用户不能发射

---

## v5.5.0 Fixed 项分析

### 1. Fixed S-meter behavior during scan reception
**Dondji 状态：基本无此问题，有微小间隙**
- 扫描期间 S 表在 `FUNCTION_INCOMING` 阶段（~200ms）不显示，因为 `app.c:165,179` 的 `gUpdateDisplay = true` 被注释掉了
- 进入 `FUNCTION_RECEIVE` 后 S 表正常显示和更新（每 500ms）
- DualVFO 模式下完全无此问题（S 表直接在 `UI_DisplayMain` 中渲染）
- **结论**：~200ms 的间隙在实际使用中几乎不可察觉，不是功能性 bug

### 2. Fixed quick-PTT stuck squelch (listenPrevRssi seeding)
**Dondji 状态：不存在此问题**
- 上游修复的 `listenPrevRssi` 变量在 Dondji 中**不存在**
- Dondji 将静噪完全交给 BK4819 硬件处理，`BK4819_SetupSquelch()` (`bk4819.c:729`) 直接写硬件寄存器
- TX->RX 转换时 `RADIO_SetupRegisters()` 完全重置 BK4819（REG_30=0 再重新启用），不存在残留的软件 RSSI 值
- **结论**：纯硬件静噪方案天然规避了此问题

### 3. Fixed Breakout beep audio setup order
**Dondji 状态：需单独确认**
- Breakout 游戏在 `app/breakout.c` 中实现
- 音频初始化顺序问题可能影响蜂鸣音播放，但这是低优先级的彩蛋功能

### 4. Fixed manual scan resume from active RX
**Dondji 状态：已修复**
- `chFrScanner.c:120-135` 中的 `CHFRSCANNER_ContinueScanning()` 使用统一条件判断：
  ```c
  if (gCurrentFunction == FUNCTION_INCOMING &&
      (IS_FREQ_CHANNEL(gNextMrChannel) || gCurrentCodeType == CODE_TYPE_OFF))
      APP_StartListening(...);
  else
      IS_FREQ_CHANNEL(gNextMrChannel) ? NextFreqChannel() : NextMemChannel();
  ```
- 信号仍在时继续监听，信号消失后正确推进到下一信道
- **结论**：扫描恢复逻辑正确

### 5. Fixed scan resume flow, AM/FM precheck, and list refresh behavior
**Dondji 状态：基本无问题，有小瑕疵**
- 扫描恢复使用参数化延迟系统（值>80 用缩放延迟，<=80 用 0 延迟），流程正确
- AM/FM 调制模式通过 VFO 结构体隐式传递链处理（`RADIO_ConfigureChannel()` -> `RADIO_SetupRegisters()`），不显式预检但工作正确
- **小瑕疵**：用户在扫描中切换扫描列表时，`currentScanList` 状态变量不立即重置，新列表在下一轮扫描周期才生效

### 6. Fixed display overlap bug
**Dondji 状态：存在潜在风险**
- `ui/main.c` 使用大量硬编码像素坐标，UI 元素绘制在同一 framebuffer 行中
- 扫描列表标记使用 XOR 帧缓冲操作 (`main.c:2990-2994`)，可能与其他元素冲突
- 频谱状态栏已有防重叠保护（`SPECTRUM_STATUS_CH_NAME_X0`/`SPECTRUM_STATUS_RIGHT_RESERVED_X` 常量）
- **结论**：DualVFO 布局有较好的区域分隔，但旧版双 VFO 布局有重叠风险

### 7. Fixed manual squelch spectrum line behavior (predictable 1 dB step)
**Dondji 状态：已实现**
- `UpdateRssiTriggerLevel` 每次按键改变 2 个 RSSI 单位 (`spectrum.c:679-690`)
- `Rssi2DBm` 公式 `rssi/2 - 160` 确保 2 RSSI = 精确 1 dBm
- `ClampRssiTriggerLevel` 强制边界检查
- **结论**：1 dB 步进已正确实现

### 8. Fixed manual spectrum behavior by skipping curve smoothing in manual mode
**Dondji 状态：可能存在问题**
- `SmoothTopY` 在 `DrawSpectrum` 中**无条件调用** (`spectrum.c:1098`)
- STILL 模式有独立渲染路径不调用平滑，但如果用户在频谱扫描状态下启用 monitor 模式观察信号，曲线平滑仍会运行
- **建议**：考虑在 `monitorMode == true` 时跳过 `SmoothTopY`

### 9. Fixed aircopy replication by including VFO area in Settings map
**Dondji 状态：需单独确认**
- 空中复制功能在 `app/aircopy.c` 中实现
- 上游修复了 VFO 区域未包含在设置映射中导致扫描范围源频率不正确的问题

---

## 总结

| 分类 | 项目 | Dondji 状态 |
|------|------|-------------|
| **Improved** | 快速扫描检测和元数据 | 基本实现，缺高级 scan-range |
| | 信道名长按编辑 | 已实现 |
| | 频谱参数持久化 | 未确认 |
| | 频谱带宽显示 | **未实现** |
| | DCS 2位索引显示 | 已实现 |
| | 双守 VFO_Lock | **部分实现，逻辑不同** |
| **Fixed** | S表扫描显示 | 基本无问题（200ms间隙） |
| | PTT静噪卡住 | **不存在此问题** |
| | Breakout蜂鸣顺序 | 未确认 |
| | 手动扫描恢复 | 已修复 |
| | 扫描恢复/AM-FM/列表刷新 | 基本无问题 |
| | 显示重叠 | **有潜在风险** |
| | 频谱静噪线1dB步进 | 已实现 |
| | 频谱手动模式曲线平滑 | **可能存在问题** |
| | 空中复制VFO区域 | 未确认 |

### 需要关注的 3 个主要差异

1. **VFO_Lock 双守显示**：Dondji 的逻辑与上游不同，不是专门针对双守接收场景
2. **频谱曲线平滑**：手动/monitor 模式下可能需要跳过 `SmoothTopY`
3. **显示重叠**：旧版 VFO 布局中的 XOR 操作存在潜在冲突风险

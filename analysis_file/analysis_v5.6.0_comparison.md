# F4HWN Fusion v5.6.0 vs Dondji 对比分析报告

分析日期：2026-06-15
上游版本：[armel/uv-k1-k5v3-firmware-custom v5.6.0](https://github.com/armel/uv-k1-k5v3-firmware-custom/releases/tag/v5.6.0)
Dondji 分支：motorola_r7
基线对比：v5.5.0（上次分析见 `analysis_v5.5.0_comparison.md`）

---

## 一、Added 新增功能分析

### 1. SetSav 屏保系统（LOGO / LOGO+ / MATRIX 三种模式）
**Dondji 状态：❌ 缺失**

上游新增了完整的屏保子系统：
- 新菜单项 `SetSav`，选项：OFF / LOGO / LOGO+ / MATRIX
- LOGO 模式：背光超时后显示开机 Logo 位图
- LOGO+ 模式：Logo 每 160ms 左移 1 像素（循环滚动效果）
- MATRIX 模式：32 列 "矩阵雨" 动画，独立字形集和 PRNG，80ms 刷新
- 设置持久化到 EEPROM（2-bit 存储在 0x00A158 共享字节）
- 编译开关：`ENABLE_FEAT_F4HWN_LOGO_SAV`（依赖 `ENABLE_FEAT_F4HWN_LOGO`）
- 唤醒行为：任意键退出屏保，非 PTT 键被消费（不触发误操作）

Dondji 现有：仅有 `ENABLE_FEAT_F4HWN_SLEEP` 深度睡眠（关屏+休眠），无屏保显示功能。

**建议：⭐ 高优先级添加。** 屏保是用户期待的高频功能，且 LOGO 模式与 Dondji 已有的自定义开机 Logo 功能天然配合。MATRIX 模式有差异化卖点。实现量约 310 行，不依赖其他 v5.6.0 改动。

---

### 2. ScanRange 子音频检测（CTCSS/DCS 自动识别）
**Dondji 状态：⚠️ 部分实现（CTCSS/DCS 扫描存在，但未集成到扫描范围）**

上游实现：
- 编译开关：`ENABLE_FEAT_F4HWN_SCAN_SUBAUDIBLE`
- `CHFRSCANNER_UpdateCssDetection()` 每 10ms 调用，读取 BK4819 的 CSS 扫描结果寄存器
- CDCSS 立即接受，CTCSS 需连续 2 次匹配（去抖动）
- 主屏幕显示检测到的音调："CTCSS 67.0Hz" 或 "D023N"

Dondji 现有：
- `App/app/scanner.c` 有完整的 CTCSS/DCS 扫描逻辑（`gScanCssResultType`、`BK4819_GetCxCSSScanResult`）
- 但仅在单频停留时工作，扫描范围（`ENABLE_SCAN_RANGES`）模式下无 CSS 检测

**建议：⭐ 高优先级添加。** Dondji 已有底层 CSS 扫描驱动，只需在扫描范围的快速预检中加入 CSS 检测调用，并在主屏 UI 添加显示。工作量中等。

---

### 3. 扫描 RSSI 火花线指示器
**Dondji 状态：❌ 缺失**

上游实现：
- 编译开关：`ENABLE_FEAT_F4HWN_SCAN_RSSI`
- 24 样本环形缓冲区记录扫描时的 RSSI 值
- 在扫描 VFO 行绘制 24 像素宽的迷你图表（sparkline）
- 3 抽头平滑滤波器 + 垂直桥接防止断裂对角线
- 仅在扫描中且未接收时绘制

**建议：🟡 中优先级添加。** 视觉反馈增强，用户可直观看到信号强度分布。实现约 150 行，独立功能。

---

### 4. 开机蜂鸣支持 LOGO 启动模式（Issue #340）
**Dondji 状态：❌ 缺失**

上游修复：当 `POnMsg` 设置为 LOGO 时，开机蜂鸣被跳过。修复后 LOGO 模式也播放蜂鸣。

Dondji 现有：`App/ui/welcome.c` 中 LOGO 路径直接 return，`App/main.c` 的蜂鸣逻辑在欢迎屏之后执行，需确认是否受影响。

**建议：🟢 低优先级修复。** 6 行代码，但影响用户体验一致性。

---

### 5. 背光灯泡图标空心/实心区分
**Dondji 状态：⚠️ 部分实现**

上游实现：
- 新增 `gFontLightOff[9]` 位图（空心灯泡，去掉内部灯丝像素）
- 手动背光模式下：灯亮显示实心，灯灭显示空心

Dondji 现有：`gFontLight` 存在于 `App/bitmaps.c`，但不是传统灯泡形状，且无空心变体。

**建议：🟢 低优先级。** 纯 UI 细节，但实现简单（9 字节位图 + 条件判断）。

---

## 二、Improved 改进分析

### 6. 屏保唤醒行为优化（首键仅唤醒，不触发操作）
**Dondji 状态：❌ 缺失（无屏保）**

上游实现：屏保激活时，按任意键仅退出屏保并点亮背光，按键事件被消费（beep 抑制、key release 在处理前执行），不会误触发扫描、切换等操作。

**建议：** 随屏保功能一起实现。

---

### 7. 屏保活动期间防护（TX/RX/FM扫描/M-SCAN/BEAM/CSS扫描）
**Dondji 状态：❌ 缺失（无屏保，但睡眠模式也有同样问题）**

上游实现：在 `ScreenSaverCanDisplay()` 中添加多条件守卫：
- TX 时不激活
- RX 时不激活
- FM 扫描时不激活
- M-SCAN 时不激活
- BEAM 时不激活
- CSS 后台扫描时不激活

**Dondji 现有睡眠模式问题：** `App/app/app.c:1687-1718` 的 `gSleepModeCountdown_500ms` 递减逻辑**未检查扫描状态**。当 `FUNCTION_IsRx()` 为 false 且非 TX 时，扫描中也会进入睡眠。对比：节电路径（line 1112-1129）正确检查了 `gScanStateDir != SCAN_OFF`。

**建议：⭐ 高优先级修复。** 即使不加屏保，Dondji 的睡眠模式也应添加 `gScanStateDir == SCAN_OFF` 守卫，防止扫描中误入睡眠。这是一行代码的修复。

---

### 8. 背光超时 OFF/常亮时的屏保激活检查
**Dondji 状态：❌ 缺失**

上游修复：`BLTime=0`（常关）或 `BLTime>=61`（常亮）时，屏保不应激活。

**建议：** 随屏保功能一起实现。

---

### 9. 屏保与 K5Viewer/截图同步
**Dondji 状态：❌ 缺失**

上游实现：屏保显示时调用 `APP_IsScreenSaverDisplayed()` 抑制状态栏和显示更新，避免截图捕获到半刷新状态。

**建议：** 随屏保功能一起实现。

---

### 10. F+4 频率复制扫描 UI 改进
**Dondji 状态：✅ 已实现**

Dondji 的 `ENABLE_COPY_CHAN_TO_VFO` 已实现（`App/app/main.c:162-196`），功能逻辑正确。

上游改进主要是 UI 文案重写（"Search Freq"/"Search Tone"/"Scan Complete"/"Scan Failed"），Dondji 的扫描 UI 有独立设计，无需同步。

**建议：** 无需操作。

---

### 11. 扫描器屏幕可读性提升（频率/音调/保存/扫描状态标签）
**Dondji 状态：⚠️ 可借鉴**

上游重写了 `UI_DisplayScanner()`，使用三行专用缓冲区，标签更清晰。Dondji 的扫描 UI (`App/ui/scanner.c`) 使用自己的设计，但可参考上游的标签层次改进。

**建议：🟡 低优先级参考。** 非必要同步，可作为 UI 优化参考。

---

### 12. VFO Lock 图标扫描时位置优化
**Dondji 状态：⚠️ 部分实现**

上游改进：
- 扫描时在活跃扫描 VFO 上抑制 Lock 图标（避免与扫描 UI 重叠）
- Lock 图标从 x=25 移到 x=24

Dondji 现有：Lock 图标渲染不检查 `gScanStateDir`，扫描时可能重叠。

**建议：🟡 中优先级。** 2 行代码改动，防止扫描时 UI 重叠。

---

### 13. FM 电台频率处理集中化（FM_SetFrequency 重构）
**Dondji 状态：⚠️ 可参考**

上游将分散的 BK1080 频率更新调用统一到 `FM_SetFrequency()` 辅助函数。Dondji 的 FM 模块 (`App/app/fm.c`) 使用直接调用方式。

**建议：🟢 低优先级重构。** 代码整洁性改进，不影响功能。

---

### 14. 截图/K5Viewer 流式传输优化（三阶段）
**Dondji 状态：⚠️ 部分实现，可进一步优化**

上游三阶段优化：
1. 用 per-chunk hash 替换全帧 `previousFrame[1024]` → 省 896 字节 SRAM
2. 按需计算 chunk 替换 1024 字节栈帧缓冲 → 省 ~1.3 KB 栈
3. 16-bit 指纹替换 8-bit → 消除哈希碰撞导致的陈旧 chunk

Dondji 现有（`App/screenshot.c`）：
- 使用 `previousFrame[1024]` 静态缓冲 + 按需 chunk 计算（已有阶段 2 的部分优化）
- 使用 8-bit 指纹（有碰撞风险）
- 版本标记 0xFF

**建议：⭐ 高优先级优化。** Dondji 已有按需 chunk 计算，但仍有 `previousFrame[1024]` 占用 SRAM。可直接移植 hash 替换方案（省 896 字节）+ 16-bit 指纹（+128 字节，净省 768 字节）。对 RAM 受限的 MCU 来说很重要。

---

### 15. 背光 PWM 内存优化（省 128 字节 SRAM）
**Dondji 状态：❌ 未优化**

上游改动：`DUTY_CYCLE_LEVELS` 从 64 降到 32，调整亮度表值保持 8 级区分度。

**建议：🟡 中优先级优化。** 5 行代码改动，省 128 字节 SRAM，视觉差异不可察觉。

---

### 16. LCD 刷新行为优化（保持 RX 闪烁可见）
**Dondji 状态：⚠️ 可参考**

上游优化：减少 LCD 重绘次数同时保持接收指示灯闪烁可见。

**建议：🟢 低优先级参考。**

---

### 17. SetTmr 状态刷新节流
**Dondji 状态：⚠️ 可参考**

上游优化：减少 SetTmr（睡眠定时器）状态刷新频率，避免不必要的显示更新。

**建议：🟢 低优先级参考。**

---

## 三、Fixed 修复分析

### 18. AM 到 FM 双守 RX 重配置（Issue #389）🔴 关键修复
**Dondji 状态：⚠️ 可能存在同样问题**

上游修复：
- 在 `RADIO_SetupRegisters()` 中，`BK4819_SetFrequency()` 之后添加 `RADIO_SetModulation(gRxVfo->Modulation)`
- 修正 `RADIO_GetAMFilterBandwidth()` 使用 `gRxVfo` 替代 `gCurrentVfo`

根因：双守从 AM 切到 FM 时，BK4819 解调模式未切换，导致无音频或失真。

Dondji 现有：`App/radio.c` 的 `RADIO_SetupRegisters()` 中，调制模式设置依赖 `RADIO_ConfigureChannel()` 的预配置，**未在 `SetFrequency()` 之后显式调用 `SetModulation()`**。`DualwatchAlternate()` (`App/app/app.c:586`) 调用 `RADIO_SetupRegisters(false)` 但不单独设置调制模式。

**建议：🔴 必须修复。** 双守 AM/FM 混合场景是常见用法，此 bug 会导致音频丢失。需检查 `RADIO_SetupRegisters()` 确保在 `SetFrequency()` 后调用 `SetModulation(gRxVfo->Modulation)`，并确认 `GetAMFilterBandwidth` 使用正确的 VFO 指针。

---

### 19. 频谱 8.33 kHz 频率取整（Issue #380）
**Dondji 状态：✅ 已有基础实现**

Dondji 的 `App/frequencies.c:74,147-153` 已有 8.33 kHz 步进和通道号转换逻辑。上游修复的是频谱分析仪（bandscope）中的频率取整，需确认 Dondji 的频谱模块是否正确应用了取整。

**建议：🟡 需验证。** 检查 `App/app/spectrum.c` 中频谱显示/峰值频率是否调用了 `FREQUENCY_RoundToStep()`。

---

### 20. 截图 chunk 陈旧问题（16-bit 指纹）
**Dondji 状态：⚠️ 存在此问题**

Dondji 使用 8-bit 指纹，128 个 chunk 有较高碰撞概率（~1/256），可能导致截图画面出现持续 ~4 秒的视觉瑕疵。

**建议：** 随 #14 截图优化一起修复。

---

### 21. BLTime=OFF 时屏保行为修复
**Dondji 状态：❌ 不适用（无屏保），但睡眠模式有类似问题**

**建议：** 参考 #7 的睡眠模式守卫修复。

---

### 22. 屏保 TX/RX/FM扫描/M-SCAN/BEAM 期间激活修复
**Dondji 状态：❌ 不适用（无屏保），但睡眠模式有同样问题**

**建议：** 参考 #7。

---

### 23. RX_TX_TIMER 与 DEBUG 编译开关冲突修复
**Dondji 状态：需验证**

上游修复了 `ENABLE_FEAT_F4HWN_RX_TX_TIMER` 和 `ENABLE_FEAT_F4HWN_DEBUG` 同时启用时的编译守卫错误。

**建议：🟢 低优先级验证。** 检查 Dondji 的 `App/ui/main.c` 中 RX/TX Timer 相关的 `#ifdef` 守卫是否正确。

---

### 24. gFM_ChannelPosition 处理修复
**Dondji 状态：需验证**

上游修正了 FM 信道位置变量的边界检查。

**建议：🟢 低优先级验证。**

---

### 25. 手动背光状态图标显示修复
**Dondji 状态：⚠️ 与 #5 相同**

手动背光模式下灯灭时应显示空心图标。

**建议：** 随 #5 一起处理。

---

## 四、维护性改进（Maintenance）参考

以下为上游代码重构，Dondji 可选择性参考：

| 项目 | 说明 | 建议 |
|------|------|------|
| LOGO 加载/显示代码重构为可复用 | 屏保复用开机 Logo 渲染 | 随屏保功能一起 |
| 子菜单字符串表改为 `const char* const` | 节省 RAM | 🟡 可参考 |
| 截图帧比较和 chunk 生成重构 | 已在 #14 中覆盖 | 随截图优化一起 |
| 小型 UI/状态/无线电代码路径重构 | 多处小改动 | 🟢 低优先级 |
| 静噪阈值加载和钳位逻辑重构 | 代码整洁性 | 🟢 低优先级 |
| FM 频率更新调用统一 | 已在 #13 中覆盖 | 随 FM 重构一起 |
| 小号反色文本渲染辅助函数 | UI 工具函数 | 🟢 可参考 |

---

## 五、总结与优先级排序

### 🔴 必须修复（Bug Fix，影响基本功能）

| # | 项目 | 工作量 | 说明 |
|---|------|--------|------|
| 18 | AM→FM 双守 RX 重配置 | 小（~10 行） | 双守 AM/FM 混合时音频丢失 |
| 7 | 睡眠模式扫描状态守卫 | 极小（1 行） | 扫描中误入睡眠 |

### ⭐ 高优先级添加（用户期待的功能 + 内存优化）

| # | 项目 | 工作量 | 说明 |
|---|------|--------|------|
| 1 | SetSav 屏保系统 | 大（~310 行） | 用户高频需求，与 LOGO 功能配合 |
| 2 | ScanRange 子音频检测 | 中（~110 行） | 底层驱动已有，需集成 |
| 14 | 截图 SRAM 优化（hash 替换 + 16-bit 指纹） | 中（~70 行） | 净省 ~768 字节 SRAM |

### 🟡 中优先级改进

| # | 项目 | 工作量 | 说明 |
|---|------|--------|------|
| 3 | 扫描 RSSI 火花线 | 中（~150 行） | 扫描可视化增强 |
| 12 | VFO Lock 图标扫描时位置 | 极小（2 行） | 防止 UI 重叠 |
| 15 | 背光 PWM 内存优化 | 极小（5 行） | 省 128 字节 SRAM |
| 19 | 频谱 8.33 kHz 取整验证 | 验证 | 确认频谱模块正确性 |

### 🟢 低优先级参考

| # | 项目 | 说明 |
|---|------|------|
| 4 | 开机蜂鸣 LOGO 模式 | 6 行修复 |
| 5/25 | 背光灯泡空心图标 | 9 字节位图 |
| 11 | 扫描器 UI 文案改进 | 可参考设计 |
| 13 | FM 频率集中化重构 | 代码整洁 |
| 16 | LCD 刷新优化 | 性能微调 |
| 17 | SetTmr 刷新节流 | 性能微调 |
| 23/24 | 编译守卫/FM 位置验证 | 需验证 |

---

### 内存影响汇总

| 项目 | SRAM 变化 | 栈变化 |
|------|-----------|--------|
| 截图 hash 替换 | -896 字节 | 0 |
| 截图 16-bit 指纹 | +128 字节 | 0 |
| 截图按需 chunk（Dondji 已有） | 0 | -1024 字节 |
| 背光 PWM 优化 | -128 字节 | 0 |
| **净节省** | **-896 字节** | **-1024 字节** |

对于 PY32F071（~16 KB SRAM），这约 2 KB 的节省意味着可用 RAM 增加约 12%。

---

*报告基于 v5.6.0 46 个 commit 的代码级分析和 Dondji motorola_r7 分支代码审计。*

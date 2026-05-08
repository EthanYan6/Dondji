<div align="center">

# 🚀 Dondji（叮咚鸡）

> 基于 PY32F071 的 UV-K1 / UV-K5 V3 固件增强

<p>
  <a href="./README.md">🏠 首页</a> |
  <a href="./README.en.md">🇺🇸 English</a>
</p>

</div>

---

## ✨ 功能特性

* 基于 `F4HWN` 固件 `v5.3.1` 开发
* 仿 Motorola R7 UI（分支：`motorola_r7`）
* 双信道界面优化（参考 UV-KX-firmware）
* 经典手机风格图标菜单
* 锁屏界面重绘
* 菜单汉化
* 信道名称编辑逻辑调整，并支持仿手机九宫格（T9）输入

---

## 📸 主界面

<img width="643" height="407" src="https://github.com/user-attachments/assets/2097c20d-58fc-4577-ba84-dbfc83876e03" />

---

## 🧭 如何进入 MAIN ONLY

1. 按 Menu
2. 进入第 3 项：显示
3. 选择第 2 项：接收模式
4. 选择：仅主信道
5. 退出菜单

---

## ⏱ 计时功能

开启：

```bash id="zh1"
-DENABLE_FEAT_F4HWN_RX_TX_TIMER=ON
```

---

## 🔀 双 VFO 页面

<img width="643" height="407" alt="image" src="https://github.com/user-attachments/assets/d606de69-2cc5-4f27-871b-d4d28c5e967e" />

---

## 🌐 语言切换

Menu → 显示 → 显示语言 → 中文 / English

---

## 🌐 在线刷机

无需安装任何软件，浏览器直接刷入固件和字库。

👉 **https://ethanyan6.github.io/Dondji/**

| 功能 | 说明 |
|------|------|
| 刷固件 | 从 GitHub Releases 拉取最新固件，或选择本地 .bin 文件 |
| 刷字库 | 1281 个中文字符写入 SPI Flash，支持信道中文命名 |
| 备份校准 | 导出设备校准数据 |
| 恢复校准 | 从备份文件恢复校准数据 |

**步骤：**
1. Chrome / Edge 打开 [刷机页面](https://ethanyan6.github.io/Dondji/)
2. **刷固件**：按住 PTT 开机 → USB 连电脑 → 点「刷固件」
3. **刷字库**：固件启动后 → USB 连电脑 → 点「刷字库」

> 刷字库需先刷入本固件。字库通过 USB 写入 SPI Flash，首次写入后固件启动时自动跳过重复写入。

**字库技术参数：**

| 参数 | 值 |
|------|------|
| 字库文件 | `docs/font/cn_font.bin` |
| SPI Flash 起始地址 | `0x010200` |
| 字符数量 | 1281 个 |
| 字库大小 | 40,151 字节 (约 39.2 KB) |
| SPI Flash 占用 | 1.90% (总容量 2MB) |

---

## ⚠️ 注意

* 刷机存在风险
* 本项目为非官方固件

---

## ❤️ 支持

如果觉得不错，欢迎点个 ⭐

---

## 🙏 致谢

* BA4QHC

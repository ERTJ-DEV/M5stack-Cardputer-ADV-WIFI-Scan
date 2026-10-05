# Cardputer WiFi 扫描器 / Cardputer WiFi Scanner

> 一个专为 **M5Stack Cardputer-ADV** 打造的 WiFi 扫描与管理工具，界面简洁，功能实用，支持中英文切换、信号强度可视化、密码保存、NTP 时间同步、电池电量显示等。
>
> A WiFi scanner and manager designed for **M5Stack Cardputer-ADV**. Clean UI, practical features, bilingual (Chinese/English) support, signal strength visualization, password saving, NTP time sync, and battery display.

![Platform](https://img.shields.io/badge/Platform-M5Stack%20Cardputer-blue) ![Version](https://img.shields.io/badge/Version-1.0.0-green) ![License](https://img.shields.io/badge/License-MIT-yellow)

---

## 🇨🇳 中文说明

### ✨ 功能特性

- 📡 **实时扫描**：一键扫描周围所有 WiFi 网络，按信号强度从强到弱排序
- 📶 **信号可视化**：5 格信号强度条 + RSSI 数值 + 信号等级描述（极强/强/中/弱/极弱）
- 🔐 **连接与保存**：支持选择 WiFi 并输入密码连接，密码自动保存至 NVS（掉电不丢失）
- 🌐 **中英文切换**：内置中英文双语界面，默认英文，可随时在设置中切换，语言设置持久化保存
- ⏰ **时间同步**：连接 WiFi 后自动通过 NTP 同步北京时间，顶部状态栏实时显示
- 🔋 **电量显示**：顶部状态栏显示电池百分比与充电状态图标
- 🎨 **开屏动画**：WiFi 图标弹性缩放 + 文字滑入淡入，非线性动画流畅自然
- 🔔 **开机反馈**：开机时 LED 白色闪烁 + 蜂鸣器“噔”一声
- ❓ **内置帮助**：设置中提供完整键位与功能说明，随时查阅

### ⌨️ 键位说明

> **Cardputer 键盘提示**：  
> `;` 和 `.` 分别对应 **上** 和 **下** 方向。  
> `Fn + `` ` `` 表示按住 `Fn` 键，再按键盘左上角的 `` ` `` 键（即 `~` 键）。

| 界面 | 操作 | 功能 |
|------|------|------|
| **主列表** | `;` / `.` | 上下选择 WiFi |
| | `Enter` | 查看 WiFi 详情 |
| | `空格` | 重新扫描 |
| | `H` | 进入设置菜单 |
| **WiFi 详情** | `;` / `.` | 切换按钮（连接 / 保存 / 返回） |
| | `Enter` | 确认当前按钮 |
| | `Fn + `` ` `` | 返回主列表 |
| **密码输入** | 直接输入 | 输入 WiFi 密码 |
| | `Enter` | 尝试连接 |
| | `退格` | 删除最后一个字符 |
| | `Fn + `` ` `` | 取消并返回详情 |
| **设置菜单** | `;` / `.` | 选择「语言」或「帮助」 |
| | `Enter` | 进入子菜单 |
| | `Fn + `` ` `` | 返回主列表 |
| **语言子菜单** | `;` / `.` | 选择语言（中文 / English / 退出） |
| | `Enter` | 确认选择 |
| | `Fn + `` ` `` | 返回设置菜单 |
| **帮助子菜单** | `Enter` 或 `Fn + `` ` `` | 返回设置菜单 |

### 🚀 快速上手

1. **烧录固件**：使用 M5Burner 选择本固件，起始地址填 `0x0`，点击烧录。
2. **开机**：设备启动后会显示开屏动画，随后自动扫描 WiFi。
3. **浏览网络**：使用 `;` 和 `.` 上下选择，按 `Enter` 查看详情。
4. **连接网络**：在详情页选择「连接」，输入密码后按 `Enter` 连接。连接成功后自动同步时间并保存密码。
5. **切换语言**：按 `H` 进入设置 → 选择「语言」→ 选择「中文」或「English」→ 按 `Enter` 确认。
6. **查看帮助**：按 `H` 进入设置 → 选择「帮助」→ 查看所有按键说明。

### 📝 注意事项

- **SSID 长度**：保存密码时，SSID 长度不能超过 15 个字符（NVS 键名限制），超长会提示无法保存。
- **中文显示**：依赖 M5GFX 内置的 `efontCN_12` 字体，若显示乱码请更新 M5Unified / M5GFX 库。
- **时间同步**：需要连接可访问外网的 WiFi 才能同步北京时间。

---

## 🇬🇧 English Guide

### ✨ Features

- 📡 **Real-time Scanning**: One-tap scan of all nearby WiFi networks, sorted by signal strength (strongest first)
- 📶 **Signal Visualization**: 5-bar signal meter + RSSI value + signal level description (Excellent/Good/Fair/Weak/Very Weak)
- 🔐 **Connect & Save**: Select a WiFi network, enter password, and connect. Passwords are auto-saved to NVS (survives power loss)
- 🌐 **Bilingual UI**: Built-in Chinese/English interface. Default is English; switch anytime in Settings. Language preference is persisted
- ⏰ **Time Sync**: Automatically syncs Beijing time via NTP after connecting to WiFi; displayed in top status bar
- 🔋 **Battery Display**: Shows battery percentage and charging icon in the top status bar
- 🎨 **Boot Animation**: WiFi icon with elastic scale + text slide-in/fade-in using non-linear easing
- 🔔 **Boot Feedback**: White LED flash + buzzer beep on startup
- ❓ **Built-in Help**: Full key bindings and feature guide available in Settings

### ⌨️ Key Bindings

> **Cardputer Keyboard Tip**:  
> `;` and `.` act as **Up** and **Down** respectively.  
> `Fn + `` ` `` means hold `Fn` and press the top-left `` ` `` key (i.e., `~` key).

| Screen | Key | Action |
|--------|-----|--------|
| **Main List** | `;` / `.` | Select WiFi up/down |
| | `Enter` | View WiFi details |
| | `Space` | Rescan |
| | `H` | Open Settings |
| **WiFi Detail** | `;` / `.` | Switch buttons (Connect / Save / Back) |
| | `Enter` | Confirm current button |
| | `Fn + `` ` `` | Back to main list |
| **Password Input** | Type directly | Enter WiFi password |
| | `Enter` | Attempt connection |
| | `Backspace` | Delete last character |
| | `Fn + `` ` `` | Cancel and return to detail |
| **Settings Menu** | `;` / `.` | Select "Language" or "Help" |
| | `Enter` | Enter submenu |
| | `Fn + `` ` `` | Back to main list |
| **Language Submenu** | `;` / `.` | Choose language (Chinese / English / Exit) |
| | `Enter` | Confirm |
| | `Fn + `` ` `` | Back to Settings |
| **Help Submenu** | `Enter` or `Fn + `` ` `` | Back to Settings |

### 🚀 Quick Start

1. **Flash Firmware**: Use M5Burner, select this firmware, set start address to `0x0`, and click Burn.
2. **Power On**: Boot animation plays, then WiFi scan starts automatically.
3. **Browse Networks**: Use `;` and `.` to navigate, `Enter` to view details.
4. **Connect**: In detail page, select "Connect", enter password, press `Enter`. On success, time syncs and password is saved.
5. **Switch Language**: Press `H` → Settings → Language → select "中文" or "English" → `Enter`.
6. **View Help**: Press `H` → Settings → Help → read all key bindings.

### 📝 Notes

- **SSID Length**: Passwords can only be saved for SSIDs up to 15 characters (NVS key limit). Longer SSIDs will show a save error.
- **Chinese Display**: Uses M5GFX built-in `efontCN_12` font. If garbled, update M5Unified / M5GFX libraries.
- **Time Sync**: Requires an internet-accessible WiFi network to sync Beijing time.

---

## 📄 更新日志 / Changelog

### v1.0.0
- 初始版本发布 / Initial release
- 支持 WiFi 扫描、信号强度可视化、连接与密码保存 / WiFi scanning, signal visualization, connect & password saving
- 支持中英文切换、NTP 时间同步、电池电量显示 / Bilingual UI, NTP time sync, battery display
- 加入开屏动画/ Boot animation
- 内置帮助页面 / Built-in help page

---

## 🙏 鸣谢 / Credits

- 固件由 **DeepSeek** 辅助开发 / Firmware developed with assistance from **DeepSeek**
- 基于 **M5Stack Cardputer-ADV** 硬件平台 / Based on **M5Stack Cardputer-ADV** hardware
- 使用 **M5Unified**、**M5Cardputer** 库 / Uses **M5Unified** and **M5Cardputer** libraries

---

**Enjoy your Cardputer WiFi Scanner!** 🎉

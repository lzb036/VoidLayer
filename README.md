# VoidLayer

VoidLayer 是一个 Windows 专用的轻量窗口透明度控制工具。它常驻系统托盘，通过全局热键调节当前前台窗口的不透明度，并能为常用窗口保存规则，在窗口重建或透明度被重置后自动重新应用。

<img src="assets/app-icon.png" alt="VoidLayer 图标" width="96">

## 适合做什么

- 让模拟器、工具窗口、参考资料窗口保持半透明。
- 用热键快速降低或恢复当前窗口透明度。
- 为指定应用或窗口标题固定透明度规则。
- 处理 MuMu 模拟器等会重置窗口样式的软件。

## 功能

- 系统托盘常驻，主流程不占用桌面空间。
- 全局热键控制前台窗口透明度。
- 支持固定/取消固定当前窗口规则。
- 强兼容模式会监听窗口事件，并定时重新应用透明度。
- 配置保存到 `%APPDATA%\VoidLayer\settings.ini`。
- 支持中文和英文界面。
- 使用原生 C++17 + Win32 API，无 Electron、Qt、WPF、Tauri 或 WinUI 运行时依赖。

## 默认热键

| 热键 | 功能 |
| --- | --- |
| `Alt + 左方向键` | 降低当前窗口不透明度 |
| `Alt + 右方向键` | 提高当前窗口不透明度 |
| `Alt + 0` | 将当前窗口恢复到 100% |
| `Alt + P` | 固定或取消固定当前窗口规则 |

## 使用方式

1. 启动 `VoidLayer.exe`，程序会进入系统托盘。
2. 点击需要调整的目标窗口，让它成为前台窗口。
3. 使用默认热键调整透明度。
4. 右键托盘图标可以打开设置、恢复当前窗口、暂停强兼容、打开配置目录或退出。

如果目标窗口以管理员权限运行，请也用管理员权限启动 VoidLayer，否则 Windows 可能阻止透明度修改。

## 设置说明

- **界面语言**：在中文和英文之间切换。
- **透明度步长**：每次热键调整的不透明度变化幅度。
- **最低不透明度**：避免窗口被调到过低后难以找回。
- **强兼容**：适合模拟器或会重建窗口样式的软件，会更积极地重新应用已固定规则。

## 构建

需要 Windows、CMake 3.20+ 和 Visual Studio 2022 C++ 工具链。

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

构建产物：

```text
build\Release\VoidLayer.exe
```

## 项目结构

```text
assets/              应用图标资源
src/                 Win32/C++ 源码
src/VoidLayer.rc     Windows 图标与版本信息资源
DESIGN.md            界面设计规范
README.md            项目说明
```

## 仓库描述

可用于 GitHub About 的一句话描述：

```text
Windows 专用轻量窗口透明度控制工具，支持托盘常驻、全局热键、固定透明度规则和模拟器强兼容。
```

推荐 topics：

```text
windows, win32, cpp17, opacity, transparency, tray-utility, hotkeys
```

## 注意事项

- 当前目标平台是 Windows 10/11 x64。
- 调整管理员权限窗口时，需要以管理员身份运行 VoidLayer。
- 程序是托盘优先工具，悬浮托盘图标会显示应用名称 `VoidLayer`。
- 透明度规则保存在本机用户配置目录，不会写入项目目录。

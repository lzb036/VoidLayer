# VoidLayer

VoidLayer is a lightweight Windows-only tray utility for adjusting and keeping window opacity. It uses native C++17 and Win32 APIs only, with no Electron, Qt, WPF, Tauri, or WinUI runtime.

## Goals

- Stay tiny and fast.
- Control the foreground window with global hotkeys.
- Keep opacity applied for windows that reset their layered style, especially MuMu emulator windows.
- Store pinned opacity rules in `%APPDATA%\VoidLayer\settings.ini`.
- Switch the interface between English and Simplified Chinese.
- Show built-in usage help from the tray menu.

## Default Hotkeys

- `Alt + Left`: lower opacity.
- `Alt + Right`: raise opacity.
- `Alt + 0`: restore current window to 100%.
- `Alt + P`: pin or unpin the current window rule.

## Build

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The executable is generated at:

```text
build\Release\VoidLayer.exe
```

## Notes

- First release targets Windows 10/11 x64.
- Run as administrator if you need to control elevated windows.
- The app is tray-first. Right-click the tray icon for settings and exit.
- Change language from the settings window. The choice is saved in the INI config.
- Open Help from the tray menu to see the default hotkeys and emulator compatibility notes.

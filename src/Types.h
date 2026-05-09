#pragma once

#include <windows.h>

#include <cstdint>
#include <optional>
#include <string>

namespace voidlayer {

constexpr UINT WM_VOIDLAYER_TRAY = WM_APP + 1;
constexpr UINT WM_VOIDLAYER_REAPPLY_EVENT = WM_APP + 2;
constexpr UINT WM_VOIDLAYER_SETTINGS_SAVED = WM_APP + 3;

constexpr UINT_PTR TIMER_REAPPLY = 1;
constexpr UINT REAPPLY_INTERVAL_MS = 800;

enum class HotkeyAction {
    DecreaseOpacity,
    IncreaseOpacity,
    RestoreOpacity,
    TogglePin
};

struct HotkeyConfig {
    UINT modifiers = MOD_ALT | MOD_NOREPEAT;
    UINT key = 0;
    bool enabled = true;
};

struct AppSettings {
    int opacityStepPercent = 10;
    int minOpacityPercent = 25;
    bool strongCompatibility = true;

    HotkeyConfig decreaseOpacity{MOD_ALT | MOD_NOREPEAT, VK_LEFT, true};
    HotkeyConfig increaseOpacity{MOD_ALT | MOD_NOREPEAT, VK_RIGHT, true};
    HotkeyConfig restoreOpacity{MOD_ALT | MOD_NOREPEAT, '0', true};
    HotkeyConfig togglePin{MOD_ALT | MOD_NOREPEAT, 'P', true};
};

struct WindowIdentity {
    HWND hwnd = nullptr;
    DWORD processId = 0;
    std::wstring processPath;
    std::wstring className;
    std::wstring title;
    HWND rootOwner = nullptr;
};

struct OpacityTarget {
    WindowIdentity identity;
    uint8_t alpha = 255;
    bool sticky = false;
    bool originalLayered = false;
    LONG_PTR originalExStyle = 0;
};

enum class RuleMatchMode {
    ProcessAndClass = 0,
    ProcessPath = 1,
    ClassName = 2,
    TitleContains = 3
};

struct OpacityRule {
    RuleMatchMode mode = RuleMatchMode::ProcessAndClass;
    std::wstring processPath;
    std::wstring className;
    std::wstring titleContains;
    uint8_t alpha = 204;
    bool enabled = true;
};

enum class ApplyStatus {
    Ok,
    InvalidWindow,
    AccessDenied,
    Unsupported,
    Failed
};

struct ApplyResult {
    ApplyStatus status = ApplyStatus::Ok;
    DWORD errorCode = ERROR_SUCCESS;
    std::wstring message;

    explicit operator bool() const {
        return status == ApplyStatus::Ok;
    }
};

int ClampPercent(int value, int minPercent, int maxPercent);
uint8_t PercentToAlpha(int percent);
int AlphaToPercent(uint8_t alpha);
std::wstring ToLower(std::wstring value);
bool IEquals(const std::wstring& left, const std::wstring& right);
bool ContainsInsensitive(const std::wstring& text, const std::wstring& needle);
std::wstring FormatWin32Error(DWORD errorCode);

}  // namespace voidlayer

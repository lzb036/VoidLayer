#include "HotkeyManager.h"

#include "Localization.h"

namespace voidlayer {

void HotkeyManager::Configure(const AppSettings& settings) {
    settings_ = settings;
}

bool HotkeyManager::RegisterAll(HWND hwnd, std::vector<std::wstring>* failures) {
    UnregisterAll(hwnd);

    bool ok = true;
    ok = RegisterOne(hwnd, ID_DECREASE, settings_.decreaseOpacity, T(settings_.language, TextId::HotkeyDecrease).c_str(), failures) && ok;
    ok = RegisterOne(hwnd, ID_INCREASE, settings_.increaseOpacity, T(settings_.language, TextId::HotkeyIncrease).c_str(), failures) && ok;
    ok = RegisterOne(hwnd, ID_RESTORE, settings_.restoreOpacity, T(settings_.language, TextId::HotkeyRestore).c_str(), failures) && ok;
    ok = RegisterOne(hwnd, ID_TOGGLE_PIN, settings_.togglePin, T(settings_.language, TextId::HotkeyPin).c_str(), failures) && ok;
    return ok;
}

void HotkeyManager::UnregisterAll(HWND hwnd) {
    if (hwnd == nullptr) {
        return;
    }

    UnregisterHotKey(hwnd, ID_DECREASE);
    UnregisterHotKey(hwnd, ID_INCREASE);
    UnregisterHotKey(hwnd, ID_RESTORE);
    UnregisterHotKey(hwnd, ID_TOGGLE_PIN);
}

std::optional<HotkeyAction> HotkeyManager::ActionFromId(WPARAM id) const {
    switch (static_cast<int>(id)) {
    case ID_DECREASE:
        return HotkeyAction::DecreaseOpacity;
    case ID_INCREASE:
        return HotkeyAction::IncreaseOpacity;
    case ID_RESTORE:
        return HotkeyAction::RestoreOpacity;
    case ID_TOGGLE_PIN:
        return HotkeyAction::TogglePin;
    default:
        return std::nullopt;
    }
}

bool HotkeyManager::RegisterOne(
    HWND hwnd,
    int id,
    const HotkeyConfig& config,
    const wchar_t* label,
    std::vector<std::wstring>* failures) {
    if (!config.enabled) {
        return true;
    }

    UINT modifiers = config.modifiers;
    if ((modifiers & MOD_NOREPEAT) == 0) {
        modifiers |= MOD_NOREPEAT;
    }

    if (RegisterHotKey(hwnd, id, modifiers, config.key)) {
        return true;
    }

    if (failures != nullptr) {
        std::wstring message = label;
        message += T(settings_.language, TextId::HotkeyRegisterFailed);
        const DWORD errorCode = GetLastError();
        const std::wstring details = FormatWin32Error(errorCode);
        if (!details.empty()) {
            message += L" ";
            message += details;
        }
        failures->push_back(message);
    }

    return false;
}

}  // namespace voidlayer

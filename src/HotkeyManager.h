#pragma once

#include "Types.h"

#include <optional>
#include <string>
#include <vector>

namespace voidlayer {

class HotkeyManager {
public:
    void Configure(const AppSettings& settings);
    bool RegisterAll(HWND hwnd, std::vector<std::wstring>* failures = nullptr);
    void UnregisterAll(HWND hwnd);
    std::optional<HotkeyAction> ActionFromId(WPARAM id) const;

private:
    static constexpr int ID_DECREASE = 3101;
    static constexpr int ID_INCREASE = 3102;
    static constexpr int ID_RESTORE = 3103;
    static constexpr int ID_TOGGLE_PIN = 3104;

    AppSettings settings_;

    bool RegisterOne(HWND hwnd, int id, const HotkeyConfig& config, const wchar_t* label, std::vector<std::wstring>* failures);
};

}  // namespace voidlayer

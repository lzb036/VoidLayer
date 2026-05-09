#include "RuleStore.h"

#include <shlobj.h>

#include <algorithm>

namespace voidlayer {
namespace {

std::wstring BuildFallbackConfigDirectory() {
    wchar_t buffer[MAX_PATH] = {};
    const DWORD length = GetEnvironmentVariableW(L"APPDATA", buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        return std::wstring(buffer) + L"\\VoidLayer";
    }
    return L".\\VoidLayer";
}

std::wstring BuildConfigDirectory() {
    PWSTR appData = nullptr;
    std::wstring directory;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appData)) && appData != nullptr) {
        directory = std::wstring(appData) + L"\\VoidLayer";
        CoTaskMemFree(appData);
    } else {
        directory = BuildFallbackConfigDirectory();
    }

    CreateDirectoryW(directory.c_str(), nullptr);
    return directory;
}

RuleMatchMode ReadMatchMode(int value) {
    switch (value) {
    case 1:
        return RuleMatchMode::ProcessPath;
    case 2:
        return RuleMatchMode::ClassName;
    case 3:
        return RuleMatchMode::TitleContains;
    case 0:
    default:
        return RuleMatchMode::ProcessAndClass;
    }
}

int WriteMatchMode(RuleMatchMode mode) {
    return static_cast<int>(mode);
}

HotkeyConfig LoadHotkey(const RuleStore& store, const wchar_t* prefix, HotkeyConfig fallback) {
    HotkeyConfig config = fallback;
    const std::wstring enabledKey = std::wstring(prefix) + L"Enabled";
    const std::wstring modifiersKey = std::wstring(prefix) + L"Modifiers";
    const std::wstring keyKey = std::wstring(prefix) + L"Key";

    config.enabled = GetPrivateProfileIntW(L"Hotkeys", enabledKey.c_str(), fallback.enabled ? 1 : 0, store.ConfigPath().c_str()) != 0;
    config.modifiers = static_cast<UINT>(GetPrivateProfileIntW(L"Hotkeys", modifiersKey.c_str(), static_cast<int>(fallback.modifiers), store.ConfigPath().c_str()));
    config.key = static_cast<UINT>(GetPrivateProfileIntW(L"Hotkeys", keyKey.c_str(), static_cast<int>(fallback.key), store.ConfigPath().c_str()));

    if ((config.modifiers & MOD_NOREPEAT) == 0) {
        config.modifiers |= MOD_NOREPEAT;
    }
    return config;
}

void SaveHotkey(const RuleStore& store, const wchar_t* prefix, const HotkeyConfig& config) {
    const std::wstring enabledKey = std::wstring(prefix) + L"Enabled";
    const std::wstring modifiersKey = std::wstring(prefix) + L"Modifiers";
    const std::wstring keyKey = std::wstring(prefix) + L"Key";

    WritePrivateProfileStringW(L"Hotkeys", enabledKey.c_str(), config.enabled ? L"1" : L"0", store.ConfigPath().c_str());
    WritePrivateProfileStringW(L"Hotkeys", modifiersKey.c_str(), std::to_wstring(config.modifiers).c_str(), store.ConfigPath().c_str());
    WritePrivateProfileStringW(L"Hotkeys", keyKey.c_str(), std::to_wstring(config.key).c_str(), store.ConfigPath().c_str());
}

}  // namespace

RuleStore::RuleStore() : configDirectory_(BuildConfigDirectory()), configPath_(configDirectory_ + L"\\settings.ini") {}

AppSettings RuleStore::LoadSettings() const {
    AppSettings settings;
    settings.opacityStepPercent = ClampPercent(ReadInt(L"Settings", L"OpacityStepPercent", settings.opacityStepPercent), 1, 50);
    settings.minOpacityPercent = ClampPercent(ReadInt(L"Settings", L"MinOpacityPercent", settings.minOpacityPercent), 5, 95);
    settings.strongCompatibility = ReadInt(L"Settings", L"StrongCompatibility", settings.strongCompatibility ? 1 : 0) != 0;

    settings.decreaseOpacity = LoadHotkey(*this, L"DecreaseOpacity", settings.decreaseOpacity);
    settings.increaseOpacity = LoadHotkey(*this, L"IncreaseOpacity", settings.increaseOpacity);
    settings.restoreOpacity = LoadHotkey(*this, L"RestoreOpacity", settings.restoreOpacity);
    settings.togglePin = LoadHotkey(*this, L"TogglePin", settings.togglePin);
    return settings;
}

void RuleStore::SaveSettings(const AppSettings& settings) const {
    WriteInt(L"Settings", L"OpacityStepPercent", ClampPercent(settings.opacityStepPercent, 1, 50));
    WriteInt(L"Settings", L"MinOpacityPercent", ClampPercent(settings.minOpacityPercent, 5, 95));
    WriteInt(L"Settings", L"StrongCompatibility", settings.strongCompatibility ? 1 : 0);

    SaveHotkey(*this, L"DecreaseOpacity", settings.decreaseOpacity);
    SaveHotkey(*this, L"IncreaseOpacity", settings.increaseOpacity);
    SaveHotkey(*this, L"RestoreOpacity", settings.restoreOpacity);
    SaveHotkey(*this, L"TogglePin", settings.togglePin);
    WritePrivateProfileStringW(nullptr, nullptr, nullptr, configPath_.c_str());
}

std::vector<OpacityRule> RuleStore::LoadRules() const {
    std::vector<OpacityRule> rules;
    const int count = std::clamp(ReadInt(L"Rules", L"Count", 0), 0, 512);
    rules.reserve(static_cast<size_t>(count));

    for (int i = 0; i < count; ++i) {
        const std::wstring section = L"Rule" + std::to_wstring(i);
        OpacityRule rule;
        rule.enabled = ReadInt(section.c_str(), L"Enabled", 1) != 0;
        rule.mode = ReadMatchMode(ReadInt(section.c_str(), L"Mode", 0));
        rule.processPath = ReadString(section.c_str(), L"ProcessPath", L"");
        rule.className = ReadString(section.c_str(), L"ClassName", L"");
        rule.titleContains = ReadString(section.c_str(), L"TitleContains", L"");
        rule.alpha = PercentToAlpha(ClampPercent(ReadInt(section.c_str(), L"OpacityPercent", AlphaToPercent(rule.alpha)), 5, 100));

        if (!rule.processPath.empty() || !rule.className.empty() || !rule.titleContains.empty()) {
            rules.push_back(rule);
        }
    }

    return rules;
}

void RuleStore::SaveRules(const std::vector<OpacityRule>& rules) const {
    const int oldCount = std::clamp(ReadInt(L"Rules", L"Count", 0), 0, 512);
    for (int i = 0; i < oldCount; ++i) {
        DeleteSection(L"Rule" + std::to_wstring(i));
    }

    WriteInt(L"Rules", L"Count", static_cast<int>(rules.size()));
    for (size_t i = 0; i < rules.size(); ++i) {
        const std::wstring section = L"Rule" + std::to_wstring(i);
        const OpacityRule& rule = rules[i];
        WriteInt(section.c_str(), L"Enabled", rule.enabled ? 1 : 0);
        WriteInt(section.c_str(), L"Mode", WriteMatchMode(rule.mode));
        WriteString(section.c_str(), L"ProcessPath", rule.processPath);
        WriteString(section.c_str(), L"ClassName", rule.className);
        WriteString(section.c_str(), L"TitleContains", rule.titleContains);
        WriteInt(section.c_str(), L"OpacityPercent", AlphaToPercent(rule.alpha));
    }

    WritePrivateProfileStringW(nullptr, nullptr, nullptr, configPath_.c_str());
}

int RuleStore::ReadInt(const wchar_t* section, const wchar_t* key, int fallback) const {
    return GetPrivateProfileIntW(section, key, fallback, configPath_.c_str());
}

std::wstring RuleStore::ReadString(const wchar_t* section, const wchar_t* key, const wchar_t* fallback) const {
    wchar_t buffer[4096] = {};
    GetPrivateProfileStringW(section, key, fallback, buffer, static_cast<DWORD>(ARRAYSIZE(buffer)), configPath_.c_str());
    return buffer;
}

void RuleStore::WriteInt(const wchar_t* section, const wchar_t* key, int value) const {
    WriteString(section, key, std::to_wstring(value));
}

void RuleStore::WriteString(const wchar_t* section, const wchar_t* key, const std::wstring& value) const {
    WritePrivateProfileStringW(section, key, value.c_str(), configPath_.c_str());
}

void RuleStore::DeleteSection(const std::wstring& section) const {
    WritePrivateProfileStringW(section.c_str(), nullptr, nullptr, configPath_.c_str());
}

}  // namespace voidlayer

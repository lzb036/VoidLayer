#pragma once

#include "Types.h"

#include <string>
#include <vector>

namespace voidlayer {

class RuleStore {
public:
    RuleStore();

    const std::wstring& ConfigPath() const {
        return configPath_;
    }

    const std::wstring& ConfigDirectory() const {
        return configDirectory_;
    }

    AppSettings LoadSettings() const;
    void SaveSettings(const AppSettings& settings) const;

    std::vector<OpacityRule> LoadRules() const;
    void SaveRules(const std::vector<OpacityRule>& rules) const;

private:
    std::wstring configDirectory_;
    std::wstring configPath_;

    int ReadInt(const wchar_t* section, const wchar_t* key, int fallback) const;
    std::wstring ReadString(const wchar_t* section, const wchar_t* key, const wchar_t* fallback) const;
    void WriteInt(const wchar_t* section, const wchar_t* key, int value) const;
    void WriteString(const wchar_t* section, const wchar_t* key, const std::wstring& value) const;
    void DeleteSection(const std::wstring& section) const;
};

}  // namespace voidlayer

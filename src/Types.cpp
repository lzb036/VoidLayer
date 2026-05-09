#include "Types.h"

#include <algorithm>
#include <cwctype>

namespace voidlayer {

int ClampPercent(int value, int minPercent, int maxPercent) {
    return std::clamp(value, minPercent, maxPercent);
}

uint8_t PercentToAlpha(int percent) {
    percent = ClampPercent(percent, 0, 100);
    return static_cast<uint8_t>((percent * 255 + 50) / 100);
}

int AlphaToPercent(uint8_t alpha) {
    return static_cast<int>((static_cast<int>(alpha) * 100 + 127) / 255);
}

std::wstring ToLower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return value;
}

bool IEquals(const std::wstring& left, const std::wstring& right) {
    return ToLower(left) == ToLower(right);
}

bool ContainsInsensitive(const std::wstring& text, const std::wstring& needle) {
    if (needle.empty()) {
        return true;
    }
    return ToLower(text).find(ToLower(needle)) != std::wstring::npos;
}

std::wstring FormatWin32Error(DWORD errorCode) {
    if (errorCode == ERROR_SUCCESS) {
        return L"";
    }

    wchar_t* buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr);

    std::wstring message;
    if (length > 0 && buffer != nullptr) {
        message.assign(buffer, length);
        LocalFree(buffer);
    }

    while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n' || message.back() == L' ')) {
        message.pop_back();
    }

    return message;
}

}  // namespace voidlayer

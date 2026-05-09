#pragma once

#include "Types.h"

#include <map>

namespace voidlayer {

class TransparencyEngine {
public:
    ApplyResult ApplyOpacity(const WindowIdentity& identity, uint8_t alpha);
    ApplyResult RestoreOpacity(const WindowIdentity& identity);
    std::optional<uint8_t> GetCurrentAlpha(HWND hwnd) const;
    bool NeedsReapply(HWND hwnd, uint8_t desiredAlpha) const;

private:
    struct OriginalLayerState {
        LONG_PTR exStyle = 0;
        bool layered = false;
        bool hasAttributes = false;
        COLORREF colorKey = 0;
        BYTE alpha = 255;
        DWORD flags = 0;
    };

    std::map<HWND, OriginalLayerState> originalStates_;

    ApplyResult EnsureLayered(HWND hwnd);
    ApplyResult SetAlpha(HWND hwnd, uint8_t alpha);
    ApplyResult SetExStyle(HWND hwnd, LONG_PTR exStyle);
    std::optional<LONG_PTR> GetExStyle(HWND hwnd, DWORD* errorCode = nullptr) const;
    void CaptureOriginalState(HWND hwnd);
    ApplyResult MakeResultFromLastError(ApplyStatus fallbackStatus, const wchar_t* context) const;
};

}  // namespace voidlayer

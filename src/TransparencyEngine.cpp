#include "TransparencyEngine.h"

namespace voidlayer {
namespace {

ApplyStatus StatusFromError(DWORD errorCode, ApplyStatus fallback) {
    if (errorCode == ERROR_ACCESS_DENIED || errorCode == ERROR_PRIVILEGE_NOT_HELD) {
        return ApplyStatus::AccessDenied;
    }
    return fallback;
}

void RefreshWindowFrame(HWND hwnd) {
    SetWindowPos(
        hwnd,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
}

}  // namespace

ApplyResult TransparencyEngine::ApplyOpacity(const WindowIdentity& identity, uint8_t alpha) {
    HWND hwnd = identity.hwnd;
    if (hwnd == nullptr || !IsWindow(hwnd)) {
        return {ApplyStatus::InvalidWindow, ERROR_INVALID_WINDOW_HANDLE, L"Target window is no longer available."};
    }

    if (alpha >= 255) {
        return RestoreOpacity(identity);
    }

    CaptureOriginalState(hwnd);

    if (auto layered = EnsureLayered(hwnd); !layered) {
        return layered;
    }

    return SetAlpha(hwnd, alpha);
}

ApplyResult TransparencyEngine::RestoreOpacity(const WindowIdentity& identity) {
    HWND hwnd = identity.hwnd;
    if (hwnd == nullptr || !IsWindow(hwnd)) {
        return {ApplyStatus::InvalidWindow, ERROR_INVALID_WINDOW_HANDLE, L"Target window is no longer available."};
    }

    const auto currentExStyle = GetExStyle(hwnd);
    if (!currentExStyle) {
        return MakeResultFromLastError(ApplyStatus::Failed, L"Unable to read window style.");
    }

    const auto original = originalStates_.find(hwnd);
    if (original == originalStates_.end()) {
        if ((*currentExStyle & WS_EX_LAYERED) != 0) {
            return SetAlpha(hwnd, 255);
        }
        return {};
    }

    LONG_PTR nextStyle = *currentExStyle;
    if (original->second.layered) {
        nextStyle |= WS_EX_LAYERED;
    } else {
        nextStyle &= ~WS_EX_LAYERED;
    }

    if (auto styleResult = SetExStyle(hwnd, nextStyle); !styleResult) {
        return styleResult;
    }

    if (original->second.layered) {
        if (original->second.hasAttributes && original->second.flags != 0) {
            if (!SetLayeredWindowAttributes(
                    hwnd,
                    original->second.colorKey,
                    original->second.alpha,
                    original->second.flags)) {
                return MakeResultFromLastError(ApplyStatus::Failed, L"Unable to restore layered attributes.");
            }
        } else {
            if (!SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA)) {
                return MakeResultFromLastError(ApplyStatus::Failed, L"Unable to restore opacity.");
            }
        }
    }

    RefreshWindowFrame(hwnd);
    originalStates_.erase(original);
    return {};
}

std::optional<uint8_t> TransparencyEngine::GetCurrentAlpha(HWND hwnd) const {
    if (hwnd == nullptr || !IsWindow(hwnd)) {
        return std::nullopt;
    }

    const auto exStyle = GetExStyle(hwnd);
    if (!exStyle) {
        return std::nullopt;
    }

    if ((*exStyle & WS_EX_LAYERED) == 0) {
        return static_cast<uint8_t>(255);
    }

    BYTE alpha = 255;
    COLORREF colorKey = 0;
    DWORD flags = 0;
    if (!GetLayeredWindowAttributes(hwnd, &colorKey, &alpha, &flags)) {
        return std::nullopt;
    }

    if ((flags & LWA_ALPHA) == 0) {
        return static_cast<uint8_t>(255);
    }

    return alpha;
}

bool TransparencyEngine::NeedsReapply(HWND hwnd, uint8_t desiredAlpha) const {
    if (desiredAlpha >= 255) {
        return false;
    }

    const auto current = GetCurrentAlpha(hwnd);
    if (!current) {
        return true;
    }

    return *current != desiredAlpha;
}

ApplyResult TransparencyEngine::EnsureLayered(HWND hwnd) {
    const auto currentExStyle = GetExStyle(hwnd);
    if (!currentExStyle) {
        return MakeResultFromLastError(ApplyStatus::Failed, L"Unable to read window style.");
    }

    if ((*currentExStyle & WS_EX_LAYERED) != 0) {
        return {};
    }

    return SetExStyle(hwnd, *currentExStyle | WS_EX_LAYERED);
}

ApplyResult TransparencyEngine::SetAlpha(HWND hwnd, uint8_t alpha) {
    SetLastError(ERROR_SUCCESS);
    if (!SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA)) {
        return MakeResultFromLastError(ApplyStatus::Failed, L"Unable to set opacity.");
    }

    RefreshWindowFrame(hwnd);
    return {};
}

ApplyResult TransparencyEngine::SetExStyle(HWND hwnd, LONG_PTR exStyle) {
    SetLastError(ERROR_SUCCESS);
    const LONG_PTR previous = SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);
    const DWORD errorCode = GetLastError();
    if (previous == 0 && errorCode != ERROR_SUCCESS) {
        ApplyResult result;
        result.status = StatusFromError(errorCode, ApplyStatus::Failed);
        result.errorCode = errorCode;
        result.message = L"Unable to change window style.";
        return result;
    }

    RefreshWindowFrame(hwnd);
    return {};
}

std::optional<LONG_PTR> TransparencyEngine::GetExStyle(HWND hwnd, DWORD* errorCode) const {
    SetLastError(ERROR_SUCCESS);
    const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    const DWORD lastError = GetLastError();
    if (errorCode != nullptr) {
        *errorCode = lastError;
    }

    if (exStyle == 0 && lastError != ERROR_SUCCESS) {
        return std::nullopt;
    }

    return exStyle;
}

void TransparencyEngine::CaptureOriginalState(HWND hwnd) {
    if (originalStates_.find(hwnd) != originalStates_.end()) {
        return;
    }

    OriginalLayerState state;
    if (const auto exStyle = GetExStyle(hwnd)) {
        state.exStyle = *exStyle;
        state.layered = ((*exStyle & WS_EX_LAYERED) != 0);
    }

    if (state.layered) {
        state.hasAttributes = GetLayeredWindowAttributes(hwnd, &state.colorKey, &state.alpha, &state.flags) != FALSE;
    }

    originalStates_[hwnd] = state;
}

ApplyResult TransparencyEngine::MakeResultFromLastError(ApplyStatus fallbackStatus, const wchar_t* context) const {
    const DWORD errorCode = GetLastError();
    ApplyResult result;
    result.status = StatusFromError(errorCode, fallbackStatus);
    result.errorCode = errorCode;
    result.message = context != nullptr ? context : L"Window operation failed.";
    if (errorCode != ERROR_SUCCESS) {
        const std::wstring details = FormatWin32Error(errorCode);
        if (!details.empty()) {
            result.message += L" ";
            result.message += details;
        }
    }
    return result;
}

}  // namespace voidlayer

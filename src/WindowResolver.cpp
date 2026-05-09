#include "WindowResolver.h"

#include <dwmapi.h>

#include <array>

namespace voidlayer {
namespace {

std::wstring ReadWindowText(HWND hwnd) {
    const int length = GetWindowTextLengthW(hwnd);
    if (length <= 0) {
        return L"";
    }

    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    const int copied = GetWindowTextW(hwnd, text.data(), length + 1);
    text.resize(copied > 0 ? static_cast<size_t>(copied) : 0);
    return text;
}

std::wstring ReadClassName(HWND hwnd) {
    std::array<wchar_t, 256> buffer = {};
    const int copied = GetClassNameW(hwnd, buffer.data(), static_cast<int>(buffer.size()));
    if (copied <= 0) {
        return L"";
    }
    return std::wstring(buffer.data(), static_cast<size_t>(copied));
}

std::wstring ReadProcessPath(DWORD processId) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (process == nullptr) {
        return L"";
    }

    std::wstring path(32768, L'\0');
    DWORD size = static_cast<DWORD>(path.size());
    if (!QueryFullProcessImageNameW(process, 0, path.data(), &size)) {
        CloseHandle(process);
        return L"";
    }

    CloseHandle(process);
    path.resize(size);
    return path;
}

bool IsCloaked(HWND hwnd) {
    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))) {
        return cloaked != FALSE;
    }
    return false;
}

bool ClassIsSystemShell(const std::wstring& className) {
    const std::wstring lowered = ToLower(className);
    return lowered == L"progman" ||
           lowered == L"workerw" ||
           lowered == L"shell_traywnd" ||
           lowered == L"button" ||
           lowered == L"dv2controlhost";
}

struct EnumContext {
    const WindowResolver* resolver = nullptr;
    std::vector<WindowIdentity>* windows = nullptr;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    auto* context = reinterpret_cast<EnumContext*>(lParam);
    if (context == nullptr || context->resolver == nullptr || context->windows == nullptr) {
        return TRUE;
    }

    if (auto identity = context->resolver->ResolveWindow(hwnd)) {
        context->windows->push_back(*identity);
    }

    return TRUE;
}

bool PathMatches(const std::wstring& left, const std::wstring& right) {
    if (left.empty() || right.empty()) {
        return false;
    }
    return IEquals(left, right);
}

}  // namespace

void WindowResolver::SetIgnoredWindows(HWND hostWindow, HWND settingsWindow) {
    hostWindow_ = hostWindow;
    settingsWindow_ = settingsWindow;
}

std::optional<WindowIdentity> WindowResolver::ResolveForegroundWindow() const {
    HWND foreground = GetForegroundWindow();
    if (foreground == nullptr) {
        return std::nullopt;
    }

    HWND rootOwner = GetAncestor(foreground, GA_ROOTOWNER);
    if (rootOwner != nullptr && IsCandidateWindow(rootOwner)) {
        if (auto identity = ResolveWindow(rootOwner)) {
            return identity;
        }
    }

    HWND root = GetAncestor(foreground, GA_ROOT);
    if (root != nullptr && IsCandidateWindow(root)) {
        if (auto identity = ResolveWindow(root)) {
            return identity;
        }
    }

    return ResolveWindow(foreground);
}

std::optional<WindowIdentity> WindowResolver::ResolveWindow(HWND hwnd) const {
    if (!IsCandidateWindow(hwnd)) {
        return std::nullopt;
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0) {
        return std::nullopt;
    }

    WindowIdentity identity;
    identity.hwnd = hwnd;
    identity.processId = processId;
    identity.processPath = ReadProcessPath(processId);
    identity.className = ReadClassName(hwnd);
    identity.title = ReadWindowText(hwnd);
    identity.rootOwner = GetAncestor(hwnd, GA_ROOTOWNER);
    if (identity.rootOwner == nullptr) {
        identity.rootOwner = hwnd;
    }

    return identity;
}

std::optional<WindowIdentity> WindowResolver::FindBestMatchingWindow(const WindowIdentity& previous) const {
    const auto windows = EnumerateCandidateWindows();

    for (const auto& window : windows) {
        if (window.hwnd == previous.hwnd && IsWindow(window.hwnd)) {
            return window;
        }
    }

    for (const auto& window : windows) {
        if (PathMatches(window.processPath, previous.processPath) && IEquals(window.className, previous.className)) {
            return window;
        }
    }

    for (const auto& window : windows) {
        if (PathMatches(window.processPath, previous.processPath) && ContainsInsensitive(window.title, previous.title)) {
            return window;
        }
    }

    return std::nullopt;
}

std::vector<WindowIdentity> WindowResolver::EnumerateCandidateWindows() const {
    std::vector<WindowIdentity> windows;
    EnumContext context{this, &windows};
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&context));
    return windows;
}

bool WindowResolver::MatchesRule(const WindowIdentity& identity, const OpacityRule& rule) const {
    if (!rule.enabled) {
        return false;
    }

    switch (rule.mode) {
    case RuleMatchMode::ProcessPath:
        return PathMatches(identity.processPath, rule.processPath);
    case RuleMatchMode::ClassName:
        return !rule.className.empty() && IEquals(identity.className, rule.className);
    case RuleMatchMode::TitleContains:
        return !rule.titleContains.empty() && ContainsInsensitive(identity.title, rule.titleContains);
    case RuleMatchMode::ProcessAndClass:
    default:
        return PathMatches(identity.processPath, rule.processPath) &&
               (rule.className.empty() || IEquals(identity.className, rule.className)) &&
               (rule.titleContains.empty() || ContainsInsensitive(identity.title, rule.titleContains));
    }
}

bool WindowResolver::IsCandidateWindow(HWND hwnd) const {
    if (hwnd == nullptr || !IsWindow(hwnd) || IsIgnoredWindow(hwnd)) {
        return false;
    }

    if (!IsWindowVisible(hwnd) || IsIconic(hwnd) || IsCloaked(hwnd)) {
        return false;
    }

    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if ((style & WS_CHILD) != 0) {
        return false;
    }

    const std::wstring className = ReadClassName(hwnd);
    if (ClassIsSystemShell(className)) {
        return false;
    }

    return true;
}

bool WindowResolver::IsIgnoredWindow(HWND hwnd) const {
    if (hwnd == hostWindow_ || hwnd == settingsWindow_) {
        return true;
    }

    if (hostWindow_ != nullptr && GetAncestor(hwnd, GA_ROOTOWNER) == hostWindow_) {
        return true;
    }

    if (settingsWindow_ != nullptr && GetAncestor(hwnd, GA_ROOTOWNER) == settingsWindow_) {
        return true;
    }

    return false;
}

}  // namespace voidlayer

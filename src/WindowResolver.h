#pragma once

#include "Types.h"

#include <optional>
#include <vector>

namespace voidlayer {

class WindowResolver {
public:
    void SetIgnoredWindows(HWND hostWindow, HWND settingsWindow);

    std::optional<WindowIdentity> ResolveForegroundWindow() const;
    std::optional<WindowIdentity> ResolveWindow(HWND hwnd) const;
    std::optional<WindowIdentity> FindBestMatchingWindow(const WindowIdentity& previous) const;
    std::vector<WindowIdentity> EnumerateCandidateWindows() const;

    bool MatchesRule(const WindowIdentity& identity, const OpacityRule& rule) const;
    bool IsCandidateWindow(HWND hwnd) const;

private:
    HWND hostWindow_ = nullptr;
    HWND settingsWindow_ = nullptr;

    bool IsIgnoredWindow(HWND hwnd) const;
};

}  // namespace voidlayer

#pragma once

#include "HotkeyManager.h"
#include "ReapplyService.h"
#include "RuleStore.h"
#include "SettingsDialog.h"
#include "TransparencyEngine.h"
#include "WindowResolver.h"

#include <shellapi.h>

#include <memory>
#include <string>

namespace voidlayer {

class AppHost {
public:
    AppHost();
    ~AppHost();

    int Run(HINSTANCE instance, int showCommand);

private:
    HINSTANCE instance_ = nullptr;
    HWND hwnd_ = nullptr;
    HANDLE mutex_ = nullptr;
    NOTIFYICONDATAW trayIcon_ = {};
    bool trayCreated_ = false;
    bool reapplyPaused_ = false;

    AppSettings settings_;
    RuleStore store_;
    WindowResolver resolver_;
    TransparencyEngine transparency_;
    HotkeyManager hotkeys_;
    ReapplyService reapply_;
    std::unique_ptr<SettingsDialog> settingsDialog_;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    bool Initialize(HINSTANCE instance);
    void Shutdown();
    void RegisterWindowClass();
    void CreateTrayIcon();
    void RemoveTrayIcon();
    void ShowTrayMenu();
    void ShowBalloon(const std::wstring& title, const std::wstring& message, DWORD icon = NIIF_INFO);
    void UpdateTrayTip(const std::wstring& suffix = L"");

    void OpenSettings();
    void ShowHelp();
    void OnSettingsSaved();
    void ToggleReapplyPause();
    void Exit();

    void HandleHotkey(WPARAM id);
    void AdjustForegroundOpacity(int direction);
    void RestoreForegroundOpacity();
    void TogglePinForForeground();

    std::optional<WindowIdentity> ResolveTargetOrNotify();
    void NotifyApplyFailure(const ApplyResult& result);
};

}  // namespace voidlayer

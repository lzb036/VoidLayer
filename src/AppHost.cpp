#include "AppHost.h"

#include <commctrl.h>
#include <shellapi.h>

#include <algorithm>

namespace voidlayer {
namespace {

constexpr wchar_t kHostClassName[] = L"VoidLayerHostWindow";
constexpr UINT kTrayIconId = 1;

constexpr UINT IDM_SETTINGS = 5101;
constexpr UINT IDM_RESTORE = 5102;
constexpr UINT IDM_PAUSE = 5103;
constexpr UINT IDM_OPEN_CONFIG = 5104;
constexpr UINT IDM_EXIT = 5105;

std::wstring PercentTextFromAlpha(uint8_t alpha) {
    return std::to_wstring(AlphaToPercent(alpha)) + L"%";
}

int StepAlphaFromPercent(int percent) {
    return std::max(1, static_cast<int>(PercentToAlpha(percent)));
}

}  // namespace

AppHost::AppHost() : reapply_(resolver_, transparency_, store_) {}

AppHost::~AppHost() {
    Shutdown();
}

int AppHost::Run(HINSTANCE instance, int) {
    if (!Initialize(instance)) {
        return 1;
    }

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}

bool AppHost::Initialize(HINSTANCE instance) {
    instance_ = instance;

    mutex_ = CreateMutexW(nullptr, TRUE, L"Local\\VoidLayer.SingleInstance");
    if (mutex_ == nullptr) {
        MessageBoxW(nullptr, L"Unable to create single-instance lock.", L"VoidLayer", MB_OK | MB_ICONERROR);
        return false;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"VoidLayer is already running.", L"VoidLayer", MB_OK | MB_ICONINFORMATION);
        return false;
    }

    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    settings_ = store_.LoadSettings();
    hotkeys_.Configure(settings_);
    reapply_.Configure(settings_);
    reapply_.LoadRules();

    RegisterWindowClass();
    hwnd_ = CreateWindowExW(
        0,
        kHostClassName,
        L"VoidLayer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        320,
        240,
        nullptr,
        nullptr,
        instance_,
        this);

    if (hwnd_ == nullptr) {
        MessageBoxW(nullptr, L"Unable to create host window.", L"VoidLayer", MB_OK | MB_ICONERROR);
        return false;
    }

    resolver_.SetIgnoredWindows(hwnd_, nullptr);
    CreateTrayIcon();

    std::vector<std::wstring> hotkeyFailures;
    if (!hotkeys_.RegisterAll(hwnd_, &hotkeyFailures) && !hotkeyFailures.empty()) {
        ShowBalloon(L"Hotkey conflict", hotkeyFailures.front(), NIIF_WARNING);
    }

    reapply_.Start(hwnd_);
    SetTimer(hwnd_, TIMER_REAPPLY, REAPPLY_INTERVAL_MS, nullptr);
    reapply_.ReconcileStickyTargets(true);

    ShowBalloon(L"VoidLayer", L"Running in the system tray. Use Alt+Left/Right to adjust opacity.");
    return true;
}

void AppHost::Shutdown() {
    if (hwnd_ != nullptr) {
        KillTimer(hwnd_, TIMER_REAPPLY);
    }

    hotkeys_.UnregisterAll(hwnd_);
    reapply_.Stop();
    settingsDialog_.reset();
    RemoveTrayIcon();

    if (hwnd_ != nullptr) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }

    if (mutex_ != nullptr) {
        ReleaseMutex(mutex_);
        CloseHandle(mutex_);
        mutex_ = nullptr;
    }
}

LRESULT CALLBACK AppHost::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    AppHost* host = reinterpret_cast<AppHost*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        host = reinterpret_cast<AppHost*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(host));
        host->hwnd_ = hwnd;
    }

    if (host != nullptr) {
        return host->HandleMessage(message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT AppHost::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_HOTKEY:
        HandleHotkey(wParam);
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_REAPPLY && !reapplyPaused_) {
            reapply_.ReconcileStickyTargets(true);
            if (settingsDialog_ != nullptr && settingsDialog_->IsOpen()) {
                settingsDialog_->SetActiveTargetCount(reapply_.ActiveTargetCount());
            }
        }
        return 0;

    case WM_VOIDLAYER_REAPPLY_EVENT:
        if (!reapplyPaused_) {
            reapply_.OnWinEvent();
        }
        return 0;

    case WM_VOIDLAYER_SETTINGS_SAVED:
        OnSettingsSaved();
        return 0;

    case WM_VOIDLAYER_TRAY:
        if (LOWORD(lParam) == WM_CONTEXTMENU || LOWORD(lParam) == WM_RBUTTONUP) {
            ShowTrayMenu();
            return 0;
        }
        if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
            OpenSettings();
            return 0;
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_SETTINGS:
            OpenSettings();
            return 0;
        case IDM_RESTORE:
            RestoreForegroundOpacity();
            return 0;
        case IDM_PAUSE:
            ToggleReapplyPause();
            return 0;
        case IDM_OPEN_CONFIG:
            ShellExecuteW(hwnd_, L"open", store_.ConfigDirectory().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        case IDM_EXIT:
            Exit();
            return 0;
        default:
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

void AppHost::RegisterWindowClass() {
    WNDCLASSEXW wc = {};
    if (GetClassInfoExW(instance_, kHostClassName, &wc)) {
        return;
    }

    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = AppHost::WndProc;
    wc.hInstance = instance_;
    wc.lpszClassName = kHostClassName;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);
}

void AppHost::CreateTrayIcon() {
    trayIcon_ = {};
    trayIcon_.cbSize = sizeof(trayIcon_);
    trayIcon_.hWnd = hwnd_;
    trayIcon_.uID = kTrayIconId;
    trayIcon_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    trayIcon_.uCallbackMessage = WM_VOIDLAYER_TRAY;
    trayIcon_.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcscpy_s(trayIcon_.szTip, L"VoidLayer - opacity control");

    trayCreated_ = Shell_NotifyIconW(NIM_ADD, &trayIcon_) != FALSE;
    if (trayCreated_) {
        trayIcon_.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &trayIcon_);
    }
}

void AppHost::RemoveTrayIcon() {
    if (trayCreated_) {
        Shell_NotifyIconW(NIM_DELETE, &trayIcon_);
        trayCreated_ = false;
    }
}

void AppHost::ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }

    AppendMenuW(menu, MF_STRING, IDM_SETTINGS, L"Settings");
    AppendMenuW(menu, MF_STRING, IDM_RESTORE, L"Restore current window");
    AppendMenuW(menu, MF_STRING, IDM_PAUSE, reapplyPaused_ ? L"Resume strong compatibility" : L"Pause strong compatibility");
    AppendMenuW(menu, MF_STRING, IDM_OPEN_CONFIG, L"Open config folder");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_EXIT, L"Exit");

    POINT point = {};
    GetCursorPos(&point);
    SetForegroundWindow(hwnd_);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN, point.x, point.y, 0, hwnd_, nullptr);
    DestroyMenu(menu);
}

void AppHost::ShowBalloon(const std::wstring& title, const std::wstring& message, DWORD icon) {
    if (!trayCreated_) {
        return;
    }

    trayIcon_.uFlags = NIF_INFO;
    trayIcon_.dwInfoFlags = icon;
    wcsncpy_s(trayIcon_.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(trayIcon_.szInfo, message.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &trayIcon_);
    trayIcon_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
}

void AppHost::UpdateTrayTip(const std::wstring& suffix) {
    if (!trayCreated_) {
        return;
    }

    std::wstring tip = L"VoidLayer - opacity control";
    if (!suffix.empty()) {
        tip += L" - ";
        tip += suffix;
    }

    trayIcon_.uFlags = NIF_TIP;
    wcsncpy_s(trayIcon_.szTip, tip.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &trayIcon_);
    trayIcon_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
}

void AppHost::OpenSettings() {
    if (settingsDialog_ == nullptr) {
        settingsDialog_ = std::make_unique<SettingsDialog>(instance_, hwnd_, settings_, store_);
    }

    HWND settingsWindow = settingsDialog_->Show();
    settingsDialog_->SetActiveTargetCount(reapply_.ActiveTargetCount());
    resolver_.SetIgnoredWindows(hwnd_, settingsWindow);
}

void AppHost::OnSettingsSaved() {
    settings_ = store_.LoadSettings();
    hotkeys_.Configure(settings_);
    reapply_.Configure(settings_);

    std::vector<std::wstring> failures;
    hotkeys_.RegisterAll(hwnd_, &failures);
    if (!failures.empty()) {
        ShowBalloon(L"Hotkey conflict", failures.front(), NIIF_WARNING);
    } else {
        ShowBalloon(L"VoidLayer", L"Settings saved.");
    }
}

void AppHost::ToggleReapplyPause() {
    reapplyPaused_ = !reapplyPaused_;
    if (reapplyPaused_) {
        KillTimer(hwnd_, TIMER_REAPPLY);
        reapply_.Stop();
        ShowBalloon(L"VoidLayer", L"Strong compatibility paused.");
    } else {
        reapply_.Start(hwnd_);
        SetTimer(hwnd_, TIMER_REAPPLY, REAPPLY_INTERVAL_MS, nullptr);
        reapply_.ReconcileStickyTargets(true);
        ShowBalloon(L"VoidLayer", L"Strong compatibility resumed.");
    }
}

void AppHost::Exit() {
    Shutdown();
    PostQuitMessage(0);
}

void AppHost::HandleHotkey(WPARAM id) {
    const auto action = hotkeys_.ActionFromId(id);
    if (!action) {
        return;
    }

    switch (*action) {
    case HotkeyAction::DecreaseOpacity:
        AdjustForegroundOpacity(-1);
        break;
    case HotkeyAction::IncreaseOpacity:
        AdjustForegroundOpacity(1);
        break;
    case HotkeyAction::RestoreOpacity:
        RestoreForegroundOpacity();
        break;
    case HotkeyAction::TogglePin:
        TogglePinForForeground();
        break;
    }
}

void AppHost::AdjustForegroundOpacity(int direction) {
    auto identity = ResolveTargetOrNotify();
    if (!identity) {
        return;
    }

    const uint8_t currentAlpha = transparency_.GetCurrentAlpha(identity->hwnd).value_or(255);
    const int step = StepAlphaFromPercent(settings_.opacityStepPercent);
    const int minAlpha = static_cast<int>(PercentToAlpha(settings_.minOpacityPercent));
    const int nextAlphaInt = std::clamp(static_cast<int>(currentAlpha) + direction * step, minAlpha, 255);
    const uint8_t nextAlpha = static_cast<uint8_t>(nextAlphaInt);

    ApplyResult result;
    if (nextAlpha >= 255) {
        result = transparency_.RestoreOpacity(*identity);
        reapply_.RemoveSessionTarget(identity->hwnd);
        reapply_.RemovePersistentRule(*identity);
    } else {
        result = transparency_.ApplyOpacity(*identity, nextAlpha);
        if (result) {
            reapply_.AddOrUpdateSessionTarget(*identity, nextAlpha, true);
            reapply_.UpdatePersistentRuleAlpha(*identity, nextAlpha);
            UpdateTrayTip(L"current " + PercentTextFromAlpha(nextAlpha));
        }
    }

    if (!result) {
        NotifyApplyFailure(result);
    }
}

void AppHost::RestoreForegroundOpacity() {
    auto identity = ResolveTargetOrNotify();
    if (!identity) {
        return;
    }

    const ApplyResult result = transparency_.RestoreOpacity(*identity);
    reapply_.RemoveSessionTarget(identity->hwnd);
    const bool unpinned = reapply_.RemovePersistentRule(*identity);

    if (!result) {
        NotifyApplyFailure(result);
        return;
    }

    UpdateTrayTip(L"restored");
    ShowBalloon(L"VoidLayer", unpinned ? L"Window restored and rule removed." : L"Window restored.");
}

void AppHost::TogglePinForForeground() {
    auto identity = ResolveTargetOrNotify();
    if (!identity) {
        return;
    }

    const uint8_t alpha = transparency_.GetCurrentAlpha(identity->hwnd).value_or(255);
    const bool pinned = reapply_.TogglePersistentRule(*identity, alpha);
    if (pinned && alpha < 255) {
        reapply_.AddOrUpdateSessionTarget(*identity, alpha, true);
    }

    ShowBalloon(L"VoidLayer", pinned ? L"Pinned opacity rule for this app window." : L"Removed pinned opacity rule.");
}

std::optional<WindowIdentity> AppHost::ResolveTargetOrNotify() {
    HWND settingsWindow = settingsDialog_ != nullptr ? settingsDialog_->Window() : nullptr;
    resolver_.SetIgnoredWindows(hwnd_, settingsWindow);

    auto identity = resolver_.ResolveForegroundWindow();
    if (!identity) {
        ShowBalloon(L"VoidLayer", L"No controllable foreground window found.", NIIF_WARNING);
    }
    return identity;
}

void AppHost::NotifyApplyFailure(const ApplyResult& result) {
    std::wstring message = result.message.empty() ? L"Unable to change this window." : result.message;
    if (result.status == ApplyStatus::AccessDenied) {
        message += L" Try running VoidLayer as administrator.";
    }
    ShowBalloon(L"VoidLayer", message, NIIF_WARNING);
}

}  // namespace voidlayer

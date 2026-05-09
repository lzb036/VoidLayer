#include "SettingsDialog.h"

#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <uxtheme.h>

#include <string>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace voidlayer {
namespace {

constexpr wchar_t kSettingsClassName[] = L"VoidLayerSettingsWindow";

constexpr int IDC_STEP_TRACK = 4101;
constexpr int IDC_MIN_TRACK = 4102;
constexpr int IDC_STRONG_CHECK = 4103;
constexpr int IDC_SAVE = 4104;
constexpr int IDC_CLOSE = 4105;
constexpr int IDC_OPEN_CONFIG = 4106;

COLORREF HexColor(BYTE r, BYTE g, BYTE b) {
    return RGB(r, g, b);
}

HFONT CreateUiFont(int pointSize, int weight) {
    HDC screen = GetDC(nullptr);
    const int height = -MulDiv(pointSize, GetDeviceCaps(screen, LOGPIXELSY), 72);
    ReleaseDC(nullptr, screen);

    LOGFONTW font = {};
    font.lfHeight = height;
    font.lfWeight = weight;
    wcscpy_s(font.lfFaceName, L"Segoe UI Variable");
    HFONT handle = CreateFontIndirectW(&font);
    if (handle == nullptr) {
        wcscpy_s(font.lfFaceName, L"Segoe UI");
        handle = CreateFontIndirectW(&font);
    }
    return handle;
}

void SetControlFont(HWND hwnd, HFONT font) {
    if (hwnd != nullptr && font != nullptr) {
        SendMessageW(hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}

void SetExplorerTheme(HWND hwnd) {
    if (hwnd != nullptr) {
        SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
    }
}

}  // namespace

SettingsDialog::SettingsDialog(HINSTANCE instance, HWND owner, AppSettings& settings, RuleStore& store)
    : instance_(instance),
      owner_(owner),
      settings_(settings),
      store_(store),
      draft_(settings) {
    canvasBrush_ = CreateSolidBrush(HexColor(1, 1, 2));
    surfaceBrush_ = CreateSolidBrush(HexColor(15, 16, 17));
    titleFont_ = CreateUiFont(15, FW_SEMIBOLD);
    bodyFont_ = CreateUiFont(9, FW_NORMAL);
}

SettingsDialog::~SettingsDialog() {
    if (hwnd_ != nullptr) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
    DeleteObject(canvasBrush_);
    DeleteObject(surfaceBrush_);
    DeleteObject(titleFont_);
    DeleteObject(bodyFont_);
}

HWND SettingsDialog::Show() {
    if (hwnd_ != nullptr) {
        ShowWindow(hwnd_, SW_SHOWNORMAL);
        SetForegroundWindow(hwnd_);
        return hwnd_;
    }

    RegisterClassIfNeeded();
    draft_ = settings_;

    hwnd_ = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        kSettingsClassName,
        L"VoidLayer Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        500,
        390,
        owner_,
        nullptr,
        instance_,
        this);

    if (hwnd_ == nullptr) {
        return nullptr;
    }

    RECT rect = {};
    GetWindowRect(hwnd_, &rect);
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    ShowWindow(hwnd_, SW_SHOWNORMAL);
    UpdateWindow(hwnd_);
    return hwnd_;
}

void SettingsDialog::SetActiveTargetCount(size_t count) {
    activeTargetCount_ = count;
    UpdateLabels();
}

LRESULT CALLBACK SettingsDialog::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    SettingsDialog* dialog = reinterpret_cast<SettingsDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        dialog = reinterpret_cast<SettingsDialog*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
        dialog->hwnd_ = hwnd;
    }

    if (dialog != nullptr) {
        return dialog->HandleMessage(message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT SettingsDialog::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        ApplyDarkMode();
        CreateControls();
        UpdateLabels();
        return 0;

    case WM_HSCROLL:
        if (reinterpret_cast<HWND>(lParam) == stepTrack_ || reinterpret_cast<HWND>(lParam) == minTrack_) {
            draft_.opacityStepPercent = static_cast<int>(SendMessageW(stepTrack_, TBM_GETPOS, 0, 0));
            draft_.minOpacityPercent = static_cast<int>(SendMessageW(minTrack_, TBM_GETPOS, 0, 0));
            UpdateLabels();
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_STRONG_CHECK:
            draft_.strongCompatibility = SendMessageW(strongCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
            return 0;
        case IDC_SAVE:
            Save();
            return 0;
        case IDC_CLOSE:
            Close();
            return 0;
        case IDC_OPEN_CONFIG:
            ShellExecuteW(hwnd_, L"open", store_.ConfigDirectory().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            return 0;
        default:
            break;
        }
        break;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, HexColor(247, 248, 248));
        SetBkColor(dc, HexColor(1, 1, 2));
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(canvasBrush_);
    }

    case WM_ERASEBKGND: {
        RECT rect = {};
        GetClientRect(hwnd_, &rect);
        FillRect(reinterpret_cast<HDC>(wParam), &rect, canvasBrush_);
        return 1;
    }

    case WM_CLOSE:
        Close();
        return 0;

    case WM_NCDESTROY:
    {
        HWND destroyed = hwnd_;
        hwnd_ = nullptr;
        return DefWindowProcW(destroyed, message, wParam, lParam);
    }

    default:
        break;
    }

    return DefWindowProcW(hwnd_, message, wParam, lParam);
}

void SettingsDialog::RegisterClassIfNeeded() {
    WNDCLASSEXW wc = {};
    if (GetClassInfoExW(instance_, kSettingsClassName, &wc)) {
        return;
    }

    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SettingsDialog::WndProc;
    wc.hInstance = instance_;
    wc.lpszClassName = kSettingsClassName;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = canvasBrush_;
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);
    RegisterClassExW(&wc);
}

void SettingsDialog::CreateControls() {
    HWND title = CreateWindowExW(0, L"STATIC", L"VoidLayer", WS_VISIBLE | WS_CHILD, 24, 20, 180, 26, hwnd_, nullptr, instance_, nullptr);
    HWND subtitle = CreateWindowExW(
        0,
        L"STATIC",
        L"Lightweight tray opacity control for Windows.",
        WS_VISIBLE | WS_CHILD,
        24,
        48,
        360,
        22,
        hwnd_,
        nullptr,
        instance_,
        nullptr);

    stepLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_VISIBLE | WS_CHILD, 24, 92, 300, 22, hwnd_, nullptr, instance_, nullptr);
    stepTrack_ = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS, 24, 118, 420, 34, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STEP_TRACK)), instance_, nullptr);
    SendMessageW(stepTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(1, 50));
    SendMessageW(stepTrack_, TBM_SETPOS, TRUE, draft_.opacityStepPercent);
    SendMessageW(stepTrack_, TBM_SETTICFREQ, 5, 0);

    minLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_VISIBLE | WS_CHILD, 24, 166, 360, 22, hwnd_, nullptr, instance_, nullptr);
    minTrack_ = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS, 24, 192, 420, 34, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MIN_TRACK)), instance_, nullptr);
    SendMessageW(minTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(5, 95));
    SendMessageW(minTrack_, TBM_SETPOS, TRUE, draft_.minOpacityPercent);
    SendMessageW(minTrack_, TBM_SETTICFREQ, 10, 0);

    strongCheck_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"Strong compatibility: reapply opacity every 800 ms and on window events",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        24,
        238,
        430,
        24,
        hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STRONG_CHECK)),
        instance_,
        nullptr);
    SendMessageW(strongCheck_, BM_SETCHECK, draft_.strongCompatibility ? BST_CHECKED : BST_UNCHECKED, 0);

    statusLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_VISIBLE | WS_CHILD, 24, 278, 430, 22, hwnd_, nullptr, instance_, nullptr);

    HWND openButton = CreateWindowExW(0, L"BUTTON", L"Open config", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 24, 318, 112, 30, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_OPEN_CONFIG)), instance_, nullptr);
    HWND saveButton = CreateWindowExW(0, L"BUTTON", L"Save", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 276, 318, 80, 30, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SAVE)), instance_, nullptr);
    HWND closeButton = CreateWindowExW(0, L"BUTTON", L"Close", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 364, 318, 80, 30, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CLOSE)), instance_, nullptr);

    SetControlFont(title, titleFont_);
    SetControlFont(subtitle, bodyFont_);
    SetControlFont(stepLabel_, bodyFont_);
    SetControlFont(stepTrack_, bodyFont_);
    SetControlFont(minLabel_, bodyFont_);
    SetControlFont(minTrack_, bodyFont_);
    SetControlFont(strongCheck_, bodyFont_);
    SetControlFont(statusLabel_, bodyFont_);
    SetControlFont(openButton, bodyFont_);
    SetControlFont(saveButton, bodyFont_);
    SetControlFont(closeButton, bodyFont_);

    SetExplorerTheme(stepTrack_);
    SetExplorerTheme(minTrack_);
    SetExplorerTheme(strongCheck_);
    SetExplorerTheme(openButton);
    SetExplorerTheme(saveButton);
    SetExplorerTheme(closeButton);
}

void SettingsDialog::ApplyDarkMode() {
    BOOL useDark = TRUE;
    DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));
}

void SettingsDialog::UpdateLabels() {
    if (stepLabel_ != nullptr) {
        const std::wstring text = L"Opacity step: " + std::to_wstring(draft_.opacityStepPercent) + L"%";
        SetWindowTextW(stepLabel_, text.c_str());
    }

    if (minLabel_ != nullptr) {
        const std::wstring text = L"Minimum opacity: " + std::to_wstring(draft_.minOpacityPercent) + L"%";
        SetWindowTextW(minLabel_, text.c_str());
    }

    if (statusLabel_ != nullptr) {
        const std::wstring text = L"Active sticky targets and pinned rules: " + std::to_wstring(activeTargetCount_);
        SetWindowTextW(statusLabel_, text.c_str());
    }
}

void SettingsDialog::Save() {
    draft_.opacityStepPercent = static_cast<int>(SendMessageW(stepTrack_, TBM_GETPOS, 0, 0));
    draft_.minOpacityPercent = static_cast<int>(SendMessageW(minTrack_, TBM_GETPOS, 0, 0));
    draft_.strongCompatibility = SendMessageW(strongCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;

    settings_ = draft_;
    store_.SaveSettings(settings_);
    PostMessageW(owner_, WM_VOIDLAYER_SETTINGS_SAVED, 0, 0);
    Close();
}

void SettingsDialog::Close() {
    if (hwnd_ != nullptr) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

}  // namespace voidlayer

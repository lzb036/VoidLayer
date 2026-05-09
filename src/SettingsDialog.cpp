#include "SettingsDialog.h"

#include "Localization.h"
#include "resource.h"

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
constexpr int IDC_LANGUAGE_COMBO = 4107;

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

HICON LoadVoidLayerIcon(HINSTANCE instance, int width, int height) {
    HICON icon = static_cast<HICON>(
        LoadImageW(instance, MAKEINTRESOURCEW(IDI_VOIDLAYER_APP), IMAGE_ICON, width, height, LR_SHARED));
    return icon != nullptr ? icon : LoadIconW(nullptr, IDI_APPLICATION);
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
        T(settings_.language, TextId::SettingsTitle).c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        500,
        430,
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
        case IDC_LANGUAGE_COMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                const LRESULT selected = SendMessageW(languageCombo_, CB_GETCURSEL, 0, 0);
                draft_.language = selected == 1 ? AppLanguage::ChineseSimplified : AppLanguage::English;
                UpdateLabels();
            }
            return 0;
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
    wc.hIcon = LoadVoidLayerIcon(instance_, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    wc.hIconSm = LoadVoidLayerIcon(instance_, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    RegisterClassExW(&wc);
}

void SettingsDialog::CreateControls() {
    titleLabel_ = CreateWindowExW(0, L"STATIC", L"VoidLayer", WS_VISIBLE | WS_CHILD, 24, 20, 180, 26, hwnd_, nullptr, instance_, nullptr);
    subtitleLabel_ = CreateWindowExW(
        0,
        L"STATIC",
        L"",
        WS_VISIBLE | WS_CHILD,
        24,
        48,
        360,
        22,
        hwnd_,
        nullptr,
        instance_,
        nullptr);

    languageLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_VISIBLE | WS_CHILD, 24, 88, 160, 22, hwnd_, nullptr, instance_, nullptr);
    languageCombo_ = CreateWindowExW(
        0,
        WC_COMBOBOXW,
        L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        190,
        84,
        254,
        110,
        hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LANGUAGE_COMBO)),
        instance_,
        nullptr);
    SendMessageW(languageCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(T(AppLanguage::English, TextId::LanguageEnglish).c_str()));
    SendMessageW(languageCombo_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(T(AppLanguage::ChineseSimplified, TextId::LanguageChinese).c_str()));
    SendMessageW(languageCombo_, CB_SETCURSEL, draft_.language == AppLanguage::ChineseSimplified ? 1 : 0, 0);

    stepLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_VISIBLE | WS_CHILD, 24, 126, 300, 22, hwnd_, nullptr, instance_, nullptr);
    stepTrack_ = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS, 24, 152, 420, 34, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STEP_TRACK)), instance_, nullptr);
    SendMessageW(stepTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(1, 50));
    SendMessageW(stepTrack_, TBM_SETPOS, TRUE, draft_.opacityStepPercent);
    SendMessageW(stepTrack_, TBM_SETTICFREQ, 5, 0);

    minLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_VISIBLE | WS_CHILD, 24, 200, 360, 22, hwnd_, nullptr, instance_, nullptr);
    minTrack_ = CreateWindowExW(0, TRACKBAR_CLASSW, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS, 24, 226, 420, 34, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_MIN_TRACK)), instance_, nullptr);
    SendMessageW(minTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(5, 95));
    SendMessageW(minTrack_, TBM_SETPOS, TRUE, draft_.minOpacityPercent);
    SendMessageW(minTrack_, TBM_SETTICFREQ, 10, 0);

    strongCheck_ = CreateWindowExW(
        0,
        L"BUTTON",
        L"",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        24,
        272,
        430,
        24,
        hwnd_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STRONG_CHECK)),
        instance_,
        nullptr);
    SendMessageW(strongCheck_, BM_SETCHECK, draft_.strongCompatibility ? BST_CHECKED : BST_UNCHECKED, 0);

    statusLabel_ = CreateWindowExW(0, L"STATIC", L"", WS_VISIBLE | WS_CHILD, 24, 312, 430, 22, hwnd_, nullptr, instance_, nullptr);

    openButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 24, 352, 112, 30, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_OPEN_CONFIG)), instance_, nullptr);
    saveButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 276, 352, 80, 30, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_SAVE)), instance_, nullptr);
    closeButton_ = CreateWindowExW(0, L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 364, 352, 80, 30, hwnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CLOSE)), instance_, nullptr);

    SetControlFont(titleLabel_, titleFont_);
    SetControlFont(subtitleLabel_, bodyFont_);
    SetControlFont(languageLabel_, bodyFont_);
    SetControlFont(languageCombo_, bodyFont_);
    SetControlFont(stepLabel_, bodyFont_);
    SetControlFont(stepTrack_, bodyFont_);
    SetControlFont(minLabel_, bodyFont_);
    SetControlFont(minTrack_, bodyFont_);
    SetControlFont(strongCheck_, bodyFont_);
    SetControlFont(statusLabel_, bodyFont_);
    SetControlFont(openButton_, bodyFont_);
    SetControlFont(saveButton_, bodyFont_);
    SetControlFont(closeButton_, bodyFont_);

    SetExplorerTheme(languageCombo_);
    SetExplorerTheme(stepTrack_);
    SetExplorerTheme(minTrack_);
    SetExplorerTheme(strongCheck_);
    SetExplorerTheme(openButton_);
    SetExplorerTheme(saveButton_);
    SetExplorerTheme(closeButton_);
}

void SettingsDialog::ApplyDarkMode() {
    BOOL useDark = TRUE;
    DwmSetWindowAttribute(hwnd_, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));
}

void SettingsDialog::UpdateLabels() {
    if (hwnd_ != nullptr) {
        SetWindowTextW(hwnd_, T(draft_.language, TextId::SettingsTitle).c_str());
    }

    if (subtitleLabel_ != nullptr) {
        SetWindowTextW(subtitleLabel_, T(draft_.language, TextId::SettingsSubtitle).c_str());
    }

    if (languageLabel_ != nullptr) {
        SetWindowTextW(languageLabel_, T(draft_.language, TextId::LanguageLabel).c_str());
    }

    if (stepLabel_ != nullptr) {
        const std::wstring text = T(draft_.language, TextId::OpacityStepLabel) + std::to_wstring(draft_.opacityStepPercent) + L"%";
        SetWindowTextW(stepLabel_, text.c_str());
    }

    if (minLabel_ != nullptr) {
        const std::wstring text = T(draft_.language, TextId::MinimumOpacityLabel) + std::to_wstring(draft_.minOpacityPercent) + L"%";
        SetWindowTextW(minLabel_, text.c_str());
    }

    if (strongCheck_ != nullptr) {
        SetWindowTextW(strongCheck_, T(draft_.language, TextId::StrongCompatibilityLabel).c_str());
    }

    if (statusLabel_ != nullptr) {
        const std::wstring text = T(draft_.language, TextId::ActiveTargetsLabel) + std::to_wstring(activeTargetCount_);
        SetWindowTextW(statusLabel_, text.c_str());
    }

    if (openButton_ != nullptr) {
        SetWindowTextW(openButton_, T(draft_.language, TextId::OpenConfig).c_str());
    }
    if (saveButton_ != nullptr) {
        SetWindowTextW(saveButton_, T(draft_.language, TextId::Save).c_str());
    }
    if (closeButton_ != nullptr) {
        SetWindowTextW(closeButton_, T(draft_.language, TextId::Close).c_str());
    }
}

void SettingsDialog::Save() {
    draft_.opacityStepPercent = static_cast<int>(SendMessageW(stepTrack_, TBM_GETPOS, 0, 0));
    draft_.minOpacityPercent = static_cast<int>(SendMessageW(minTrack_, TBM_GETPOS, 0, 0));
    draft_.strongCompatibility = SendMessageW(strongCheck_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    draft_.language = SendMessageW(languageCombo_, CB_GETCURSEL, 0, 0) == 1 ? AppLanguage::ChineseSimplified : AppLanguage::English;

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

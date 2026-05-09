#pragma once

#include "RuleStore.h"

#include <windows.h>

namespace voidlayer {

class SettingsDialog {
public:
    SettingsDialog(HINSTANCE instance, HWND owner, AppSettings& settings, RuleStore& store);
    ~SettingsDialog();

    HWND Show();
    HWND Window() const {
        return hwnd_;
    }
    bool IsOpen() const {
        return hwnd_ != nullptr;
    }
    void SetActiveTargetCount(size_t count);

private:
    HINSTANCE instance_ = nullptr;
    HWND owner_ = nullptr;
    HWND hwnd_ = nullptr;
    AppSettings& settings_;
    RuleStore& store_;
    AppSettings draft_;
    HBRUSH canvasBrush_ = nullptr;
    HBRUSH surfaceBrush_ = nullptr;
    HFONT titleFont_ = nullptr;
    HFONT bodyFont_ = nullptr;
    size_t activeTargetCount_ = 0;

    HWND stepTrack_ = nullptr;
    HWND minTrack_ = nullptr;
    HWND titleLabel_ = nullptr;
    HWND subtitleLabel_ = nullptr;
    HWND languageLabel_ = nullptr;
    HWND languageCombo_ = nullptr;
    HWND stepLabel_ = nullptr;
    HWND minLabel_ = nullptr;
    HWND strongCheck_ = nullptr;
    HWND statusLabel_ = nullptr;
    HWND openButton_ = nullptr;
    HWND saveButton_ = nullptr;
    HWND closeButton_ = nullptr;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    void RegisterClassIfNeeded();
    void CreateControls();
    void ApplyDarkMode();
    void UpdateLabels();
    void Save();
    void Close();
};

}  // namespace voidlayer

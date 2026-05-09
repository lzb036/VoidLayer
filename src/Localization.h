#pragma once

#include "Types.h"

#include <string>

namespace voidlayer {

enum class TextId {
    AppTitle,
    MutexError,
    AlreadyRunning,
    HostCreateError,
    HotkeyConflictTitle,
    StartupBalloon,
    TrayTip,
    TrayTipCurrent,
    TrayTipRestored,
    MenuSettings,
    MenuRestore,
    MenuResumeCompatibility,
    MenuPauseCompatibility,
    MenuOpenConfig,
    MenuHelp,
    MenuExit,
    SettingsSaved,
    CompatibilityPaused,
    CompatibilityResumed,
    NoTargetWindow,
    UnableChangeWindow,
    AdminHint,
    WindowRestored,
    WindowRestoredRuleRemoved,
    RulePinned,
    RuleRemoved,
    HotkeyDecrease,
    HotkeyIncrease,
    HotkeyRestore,
    HotkeyPin,
    HotkeyRegisterFailed,
    SettingsTitle,
    SettingsSubtitle,
    LanguageLabel,
    LanguageEnglish,
    LanguageChinese,
    OpacityStepLabel,
    MinimumOpacityLabel,
    StrongCompatibilityLabel,
    ActiveTargetsLabel,
    OpenConfig,
    Save,
    Close,
    HelpTitle,
    HelpBody
};

std::wstring T(AppLanguage language, TextId id);
std::wstring T(AppLanguage language, TextId id, int value);
const wchar_t* LanguageDisplayName(AppLanguage language);
AppLanguage LanguageFromInt(int value);
int LanguageToInt(AppLanguage language);

}  // namespace voidlayer

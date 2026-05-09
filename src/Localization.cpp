#include "Localization.h"

namespace voidlayer {

std::wstring T(AppLanguage language, TextId id) {
    const bool zh = language == AppLanguage::ChineseSimplified;

    switch (id) {
    case TextId::AppTitle:
        return L"VoidLayer";
    case TextId::MutexError:
        return zh ? L"无法创建单实例锁。" : L"Unable to create single-instance lock.";
    case TextId::AlreadyRunning:
        return zh ? L"VoidLayer 已在运行。" : L"VoidLayer is already running.";
    case TextId::HostCreateError:
        return zh ? L"无法创建宿主窗口。" : L"Unable to create host window.";
    case TextId::HotkeyConflictTitle:
        return zh ? L"热键冲突" : L"Hotkey conflict";
    case TextId::StartupBalloon:
        return zh ? L"已在系统托盘运行。使用 Alt+左/右 调整透明度。" : L"Running in the system tray. Use Alt+Left/Right to adjust opacity.";
    case TextId::TrayTip:
        return L"VoidLayer";
    case TextId::TrayTipCurrent:
        return zh ? L"当前 " : L"current ";
    case TextId::TrayTipRestored:
        return zh ? L"已恢复" : L"restored";
    case TextId::MenuSettings:
        return zh ? L"设置" : L"Settings";
    case TextId::MenuRestore:
        return zh ? L"恢复当前窗口" : L"Restore current window";
    case TextId::MenuResumeCompatibility:
        return zh ? L"恢复强兼容" : L"Resume strong compatibility";
    case TextId::MenuPauseCompatibility:
        return zh ? L"暂停强兼容" : L"Pause strong compatibility";
    case TextId::MenuOpenConfig:
        return zh ? L"打开配置文件夹" : L"Open config folder";
    case TextId::MenuHelp:
        return zh ? L"帮助" : L"Help";
    case TextId::MenuExit:
        return zh ? L"退出" : L"Exit";
    case TextId::SettingsSaved:
        return zh ? L"设置已保存。" : L"Settings saved.";
    case TextId::CompatibilityPaused:
        return zh ? L"强兼容已暂停。" : L"Strong compatibility paused.";
    case TextId::CompatibilityResumed:
        return zh ? L"强兼容已恢复。" : L"Strong compatibility resumed.";
    case TextId::NoTargetWindow:
        return zh ? L"没有找到可控制的前台窗口。" : L"No controllable foreground window found.";
    case TextId::UnableChangeWindow:
        return zh ? L"无法修改这个窗口。" : L"Unable to change this window.";
    case TextId::AdminHint:
        return zh ? L" 请尝试以管理员身份运行 VoidLayer。" : L" Try running VoidLayer as administrator.";
    case TextId::WindowRestored:
        return zh ? L"窗口已恢复。" : L"Window restored.";
    case TextId::WindowRestoredRuleRemoved:
        return zh ? L"窗口已恢复，固定规则已移除。" : L"Window restored and rule removed.";
    case TextId::RulePinned:
        return zh ? L"已为这个应用窗口固定透明度规则。" : L"Pinned opacity rule for this app window.";
    case TextId::RuleRemoved:
        return zh ? L"已移除固定透明度规则。" : L"Removed pinned opacity rule.";
    case TextId::HotkeyDecrease:
        return zh ? L"降低透明度" : L"Decrease opacity";
    case TextId::HotkeyIncrease:
        return zh ? L"提高透明度" : L"Increase opacity";
    case TextId::HotkeyRestore:
        return zh ? L"恢复透明度" : L"Restore opacity";
    case TextId::HotkeyPin:
        return zh ? L"固定规则" : L"Pin rule";
    case TextId::HotkeyRegisterFailed:
        return zh ? L" 热键注册失败。" : L" hotkey could not be registered.";
    case TextId::SettingsTitle:
        return zh ? L"VoidLayer 设置" : L"VoidLayer Settings";
    case TextId::SettingsSubtitle:
        return zh ? L"轻量 Windows 托盘透明度控制工具。" : L"Lightweight tray opacity control for Windows.";
    case TextId::LanguageLabel:
        return zh ? L"界面语言" : L"Interface language";
    case TextId::LanguageEnglish:
        return L"English";
    case TextId::LanguageChinese:
        return L"中文";
    case TextId::OpacityStepLabel:
        return zh ? L"透明度步长: " : L"Opacity step: ";
    case TextId::MinimumOpacityLabel:
        return zh ? L"最低不透明度: " : L"Minimum opacity: ";
    case TextId::StrongCompatibilityLabel:
        return zh ? L"强兼容：每 800 ms 和窗口事件触发时重新应用透明度" : L"Strong compatibility: reapply opacity every 800 ms and on window events";
    case TextId::ActiveTargetsLabel:
        return zh ? L"活动目标和固定规则数量: " : L"Active sticky targets and pinned rules: ";
    case TextId::OpenConfig:
        return zh ? L"打开配置" : L"Open config";
    case TextId::Save:
        return zh ? L"保存" : L"Save";
    case TextId::Close:
        return zh ? L"关闭" : L"Close";
    case TextId::HelpTitle:
        return zh ? L"VoidLayer 使用帮助" : L"VoidLayer Help";
    case TextId::HelpBody:
        return zh
            ? L"VoidLayer 是一个轻量的 Windows 窗口透明度工具，启动后会常驻系统托盘。\n\n"
              L"基础使用：\n"
              L"1. 启动 VoidLayer 后，保持它在托盘运行。\n"
              L"2. 点击你想调整的目标窗口，让它成为前台窗口。\n"
              L"3. 使用热键调整透明度。\n\n"
              L"默认热键：\n"
              L"- Alt + 左方向键：降低不透明度\n"
              L"- Alt + 右方向键：提高不透明度\n"
              L"- Alt + 0：恢复当前窗口到 100%\n"
              L"- Alt + P：固定或取消固定当前窗口规则\n\n"
              L"固定规则：\n"
              L"当你对模拟器或常用软件设置好透明度后，按 Alt + P 可以保存规则。下次窗口重建或透明度被程序重置时，VoidLayer 会尝试自动重新应用。\n\n"
              L"强兼容：\n"
              L"设置里的强兼容模式会监听窗口事件，并每 800 ms 做一次轻量校验，适合 MuMu 模拟器这类会重置窗口样式的软件。\n\n"
              L"权限提示：\n"
              L"如果目标窗口以管理员权限运行，VoidLayer 也需要以管理员身份运行才能控制它。"
            : L"VoidLayer is a lightweight Windows window opacity tool. After startup, it runs from the system tray.\n\n"
              L"Basic usage:\n"
              L"1. Start VoidLayer and keep it running in the tray.\n"
              L"2. Click the target window so it becomes the foreground window.\n"
              L"3. Use hotkeys to adjust opacity.\n\n"
              L"Default hotkeys:\n"
              L"- Alt + Left: lower opacity\n"
              L"- Alt + Right: raise opacity\n"
              L"- Alt + 0: restore the current window to 100%\n"
              L"- Alt + P: pin or unpin the current window rule\n\n"
              L"Pinned rules:\n"
              L"After setting opacity for an emulator or frequently used app, press Alt + P to save a rule. If the window is recreated or resets its opacity, VoidLayer will try to apply it again.\n\n"
              L"Strong compatibility:\n"
              L"The strong compatibility option listens to window events and performs a lightweight check every 800 ms. It is useful for apps such as MuMu emulator that may reset window styles.\n\n"
              L"Permissions:\n"
              L"If the target window runs as administrator, VoidLayer must also run as administrator to control it.";
    default:
        return L"";
    }
}

std::wstring T(AppLanguage language, TextId id, int value) {
    return T(language, id) + std::to_wstring(value);
}

const wchar_t* LanguageDisplayName(AppLanguage language) {
    return language == AppLanguage::ChineseSimplified ? L"中文" : L"English";
}

AppLanguage LanguageFromInt(int value) {
    return value == 1 ? AppLanguage::ChineseSimplified : AppLanguage::English;
}

int LanguageToInt(AppLanguage language) {
    return language == AppLanguage::ChineseSimplified ? 1 : 0;
}

}  // namespace voidlayer

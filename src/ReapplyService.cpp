#include "ReapplyService.h"

#include <algorithm>

namespace voidlayer {
namespace {

ReapplyService* g_service = nullptr;

bool IsInterestingEvent(DWORD event) {
    return event == EVENT_SYSTEM_FOREGROUND ||
           event == EVENT_OBJECT_CREATE ||
           event == EVENT_OBJECT_SHOW ||
           event == EVENT_OBJECT_DESTROY ||
           event == EVENT_OBJECT_NAMECHANGE;
}

bool TickStillActive(DWORD untilTick) {
    return static_cast<LONG>(untilTick - GetTickCount()) > 0;
}

void InstallHook(std::vector<HWINEVENTHOOK>& hooks, DWORD event) {
    HWINEVENTHOOK hook = SetWinEventHook(
        event,
        event,
        nullptr,
        ReapplyService::WinEventProc,
        0,
        0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    if (hook != nullptr) {
        hooks.push_back(hook);
    }
}

}  // namespace

ReapplyService::ReapplyService(WindowResolver& resolver, TransparencyEngine& transparency, RuleStore& store)
    : resolver_(resolver), transparency_(transparency), store_(store) {}

void ReapplyService::Configure(const AppSettings& settings) {
    settings_ = settings;
}

void ReapplyService::LoadRules() {
    rules_ = store_.LoadRules();
}

void ReapplyService::SaveRules() const {
    store_.SaveRules(rules_);
}

void ReapplyService::Start(HWND notifyWindow) {
    notifyWindow_ = notifyWindow;
    g_service = this;

    Stop();
    notifyWindow_ = notifyWindow;
    g_service = this;

    InstallHook(hooks_, EVENT_SYSTEM_FOREGROUND);
    InstallHook(hooks_, EVENT_OBJECT_CREATE);
    InstallHook(hooks_, EVENT_OBJECT_SHOW);
    InstallHook(hooks_, EVENT_OBJECT_DESTROY);
    InstallHook(hooks_, EVENT_OBJECT_NAMECHANGE);
}

void ReapplyService::Stop() {
    for (HWINEVENTHOOK hook : hooks_) {
        UnhookWinEvent(hook);
    }
    hooks_.clear();

    if (g_service == this) {
        g_service = nullptr;
    }
}

void ReapplyService::OnWinEvent() {
    if (HasTrackedTargets()) {
        BoostInteractiveGuard(1200);
    }
    ReconcileStickyTargets(false);
}

void ReapplyService::ReconcileStickyTargets(bool force) {
    const DWORD now = GetTickCount();
    if (!force && now - lastReconcileTick_ < 150) {
        return;
    }
    lastReconcileTick_ = now;

    if (!settings_.strongCompatibility && !force) {
        return;
    }

    ReconcileSessionTargets();
    ReconcilePersistentRules();
}

void ReapplyService::BoostInteractiveGuard(DWORD durationMs) {
    if (!HasTrackedTargets()) {
        return;
    }
    interactiveGuardUntil_ = GetTickCount() + durationMs;
}

bool ReapplyService::ShouldUseInteractiveGuard() const {
    return HasTrackedTargets() && (TickStillActive(interactiveGuardUntil_) || IsTrackedWindowInteractive());
}

void ReapplyService::AddOrUpdateSessionTarget(const WindowIdentity& identity, uint8_t alpha, bool sticky) {
    for (auto& target : sessionTargets_) {
        if (MatchesTarget(target, identity)) {
            target.identity = identity;
            target.alpha = alpha;
            target.sticky = sticky;
            return;
        }
    }

    OpacityTarget target;
    target.identity = identity;
    target.alpha = alpha;
    target.sticky = sticky;
    target.originalLayered = false;
    target.originalExStyle = 0;
    sessionTargets_.push_back(target);
}

void ReapplyService::RemoveSessionTarget(HWND hwnd) {
    sessionTargets_.erase(
        std::remove_if(
            sessionTargets_.begin(),
            sessionTargets_.end(),
            [hwnd](const OpacityTarget& target) {
                return target.identity.hwnd == hwnd || !IsWindow(target.identity.hwnd);
            }),
        sessionTargets_.end());
}

bool ReapplyService::TogglePersistentRule(const WindowIdentity& identity, uint8_t alpha) {
    auto existing = std::find_if(rules_.begin(), rules_.end(), [&](const OpacityRule& rule) {
        return MatchesPersistentRuleIdentity(rule, identity);
    });

    if (existing != rules_.end()) {
        rules_.erase(existing);
        SaveRules();
        return false;
    }

    OpacityRule rule;
    rule.mode = RuleMatchMode::ProcessAndClass;
    rule.processPath = identity.processPath;
    rule.className = identity.className;
    rule.titleContains = L"";
    rule.alpha = alpha;
    rule.enabled = true;
    rules_.push_back(rule);
    SaveRules();
    return true;
}

bool ReapplyService::UpdatePersistentRuleAlpha(const WindowIdentity& identity, uint8_t alpha) {
    for (auto& rule : rules_) {
        if (MatchesPersistentRuleIdentity(rule, identity)) {
            rule.alpha = alpha;
            SaveRules();
            return true;
        }
    }
    return false;
}

bool ReapplyService::RemovePersistentRule(const WindowIdentity& identity) {
    const auto oldSize = rules_.size();
    rules_.erase(
        std::remove_if(
            rules_.begin(),
            rules_.end(),
            [&](const OpacityRule& rule) {
                return MatchesPersistentRuleIdentity(rule, identity);
            }),
        rules_.end());

    if (rules_.size() != oldSize) {
        SaveRules();
        return true;
    }
    return false;
}

bool ReapplyService::IsPinned(const WindowIdentity& identity) const {
    return std::any_of(rules_.begin(), rules_.end(), [&](const OpacityRule& rule) {
        return MatchesPersistentRuleIdentity(rule, identity);
    });
}

size_t ReapplyService::ActiveTargetCount() const {
    return sessionTargets_.size() + rules_.size();
}

bool ReapplyService::HasTrackedTargets() const {
    const bool hasSessionTargets = std::any_of(sessionTargets_.begin(), sessionTargets_.end(), [](const OpacityTarget& target) {
        return target.sticky && target.alpha < 255;
    });
    if (hasSessionTargets) {
        return true;
    }

    return std::any_of(rules_.begin(), rules_.end(), [](const OpacityRule& rule) {
        return rule.enabled && rule.alpha < 255;
    });
}

void ReapplyService::ReconcileSessionTargets() {
    for (auto& target : sessionTargets_) {
        if (!target.sticky || target.alpha >= 255) {
            continue;
        }

        if (target.identity.hwnd == nullptr || !IsWindow(target.identity.hwnd)) {
            if (auto replacement = resolver_.FindBestMatchingWindow(target.identity)) {
                target.identity = *replacement;
            } else {
                continue;
            }
        }

        if (transparency_.NeedsReapply(target.identity.hwnd, target.alpha)) {
            transparency_.ApplyOpacity(target.identity, target.alpha);
        }
    }
}

void ReapplyService::ReconcilePersistentRules() {
    if (rules_.empty()) {
        return;
    }

    const auto windows = resolver_.EnumerateCandidateWindows();
    for (const auto& rule : rules_) {
        if (!rule.enabled || rule.alpha >= 255) {
            continue;
        }

        for (const auto& window : windows) {
            if (resolver_.MatchesRule(window, rule)) {
                AddOrUpdateSessionTarget(window, rule.alpha, true);
                if (transparency_.NeedsReapply(window.hwnd, rule.alpha)) {
                    transparency_.ApplyOpacity(window, rule.alpha);
                }
            }
        }
    }
}

bool ReapplyService::IsTrackedWindowInteractive() const {
    for (const auto& target : sessionTargets_) {
        if (target.sticky && target.alpha < 255 && IsWindowInteractive(target.identity.hwnd)) {
            return true;
        }
    }

    return false;
}

bool ReapplyService::IsWindowInteractive(HWND hwnd) const {
    if (hwnd == nullptr || !IsWindow(hwnd)) {
        return false;
    }

    HWND foreground = GetForegroundWindow();
    if (foreground == hwnd ||
        GetAncestor(foreground, GA_ROOT) == hwnd ||
        GetAncestor(foreground, GA_ROOTOWNER) == hwnd ||
        IsChild(hwnd, foreground)) {
        return true;
    }

    POINT cursor = {};
    if (!GetCursorPos(&cursor)) {
        return false;
    }

    RECT rect = {};
    if (GetWindowRect(hwnd, &rect) && PtInRect(&rect, cursor)) {
        return true;
    }

    HWND underCursor = WindowFromPoint(cursor);
    return underCursor == hwnd ||
           GetAncestor(underCursor, GA_ROOT) == hwnd ||
           GetAncestor(underCursor, GA_ROOTOWNER) == hwnd ||
           IsChild(hwnd, underCursor);
}

bool ReapplyService::MatchesTarget(const OpacityTarget& target, const WindowIdentity& identity) const {
    if (target.identity.hwnd == identity.hwnd) {
        return true;
    }

    return !target.identity.processPath.empty() &&
           !identity.processPath.empty() &&
           IEquals(target.identity.processPath, identity.processPath) &&
           IEquals(target.identity.className, identity.className);
}

bool ReapplyService::MatchesPersistentRuleIdentity(const OpacityRule& rule, const WindowIdentity& identity) const {
    return !rule.processPath.empty() &&
           !identity.processPath.empty() &&
           IEquals(rule.processPath, identity.processPath) &&
           (rule.className.empty() || IEquals(rule.className, identity.className));
}

void CALLBACK ReapplyService::WinEventProc(
    HWINEVENTHOOK,
    DWORD event,
    HWND hwnd,
    LONG objectId,
    LONG childId,
    DWORD,
    DWORD) {
    if (g_service == nullptr || g_service->notifyWindow_ == nullptr) {
        return;
    }

    if (!IsInterestingEvent(event) || objectId != OBJID_WINDOW || childId != CHILDID_SELF) {
        return;
    }

    PostMessageW(g_service->notifyWindow_, WM_VOIDLAYER_REAPPLY_EVENT, static_cast<WPARAM>(event), reinterpret_cast<LPARAM>(hwnd));
}

}  // namespace voidlayer

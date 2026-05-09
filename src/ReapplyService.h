#pragma once

#include "RuleStore.h"
#include "TransparencyEngine.h"
#include "WindowResolver.h"

#include <vector>

namespace voidlayer {

class ReapplyService {
public:
    ReapplyService(WindowResolver& resolver, TransparencyEngine& transparency, RuleStore& store);

    void Configure(const AppSettings& settings);
    void LoadRules();
    void SaveRules() const;

    void Start(HWND notifyWindow);
    void Stop();
    void OnWinEvent();
    void ReconcileStickyTargets(bool force = false);
    void BoostInteractiveGuard(DWORD durationMs = INTERACTIVE_REAPPLY_GUARD_MS);
    bool ShouldUseInteractiveGuard() const;

    void AddOrUpdateSessionTarget(const WindowIdentity& identity, uint8_t alpha, bool sticky);
    void RemoveSessionTarget(HWND hwnd);

    bool TogglePersistentRule(const WindowIdentity& identity, uint8_t alpha);
    bool UpdatePersistentRuleAlpha(const WindowIdentity& identity, uint8_t alpha);
    bool RemovePersistentRule(const WindowIdentity& identity);
    bool IsPinned(const WindowIdentity& identity) const;
    size_t ActiveTargetCount() const;
    bool HasTrackedTargets() const;

    static void CALLBACK WinEventProc(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG objectId, LONG childId, DWORD eventThread, DWORD eventTime);

private:
    WindowResolver& resolver_;
    TransparencyEngine& transparency_;
    RuleStore& store_;
    AppSettings settings_;
    HWND notifyWindow_ = nullptr;
    std::vector<HWINEVENTHOOK> hooks_;
    std::vector<OpacityTarget> sessionTargets_;
    std::vector<OpacityRule> rules_;
    DWORD lastReconcileTick_ = 0;
    DWORD interactiveGuardUntil_ = 0;

    void ReconcileSessionTargets();
    void ReconcilePersistentRules();
    bool IsTrackedWindowInteractive() const;
    bool IsWindowInteractive(HWND hwnd) const;
    bool MatchesTarget(const OpacityTarget& target, const WindowIdentity& identity) const;
    bool MatchesPersistentRuleIdentity(const OpacityRule& rule, const WindowIdentity& identity) const;
};

}  // namespace voidlayer

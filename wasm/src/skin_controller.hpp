#pragma once
#include <cstdlib>
#include <cmath>
#include "mvc.hpp"
#include "skin_model.hpp"
#include "skin_view.hpp"
#include "particle_system.hpp"
#include "bridge.hpp"

// Wires the SkinModel + SkinView together and handles every input event
// forwarded from JS. Also owns the "clicker game" layer on top of the
// base model: a rapid-click combo multiplier, random critical clicks,
// passive/idle income, upgrades, a skill tree, and rebirth/prestige —
// the usual incremental-game toolkit (Cookie Clicker etc.), scaled way
// down.
class SkinController : public mvc::Controller {
public:
    static constexpr double kComboWindowMs = 900.0;
    static constexpr double kComboStep = 0.15;
    static constexpr double kComboMax = 3.0; // before Combo Flex upgrade levels
    static constexpr double kCritChance = 0.08; // before Lucky Skin upgrade levels
    static constexpr int kCritMultiplier = 5;   // before Crit Power upgrade levels
    static constexpr double kFlybyChancePerSecond = 0.00002; // 0.002%

    SkinController() : view_(model_) {
        model_.subscribe([this] { view_.render(); });
        view_.render();
    }

    void backgroundClick(double x, double y) {
        if (skindexOpen_ || progressionOpen_) return;
        if (!debounceOk()) return;
        registerClickAt(x, y);
    }

    void avatarClick(double x, double y) {
        if (!debounceOk()) return;
        registerClickAt(x, y);
    }

    void skindexOpen() {
        skindexOpen_ = true;
        view_.setSkindexOpen(true);
    }

    void skindexClose() {
        skindexOpen_ = false;
        view_.setSkindexOpen(false);
    }

    void skindexTileClick(int id) {
        model_.selectSkin(id);
        skindexClose();
    }

    void progressionOpen() {
        progressionOpen_ = true;
        view_.setProgressionOpen(true);
    }

    void progressionClose() {
        progressionOpen_ = false;
        view_.setProgressionOpen(false);
    }

    void buyUpgrade(int id) {
        model_.buyUpgrade(id);
    }

    void buySkill(int id) {
        model_.buySkill(id);
    }

    void doRebirth() {
        int gained = model_.projectedRebirthEssence();
        bool could = model_.canRebirth();
        model_.doRebirth();
        if (could) {
            std::string msg = std::string(u8"\U0001F30C rebirth! +") + std::to_string(gained) + " essence";
            js_show_toast(msg.c_str());
        }
    }

    void tick() {
        particles_.tick();

        double now = js_now();
        if (lastTick_ == 0.0) { lastTick_ = now; return; } // skip first frame's huge dt
        double dt = now - lastTick_;
        lastTick_ = now;

        // Combo decays back to 1x once you stop clicking for a beat,
        // unless the Infinite Combo skill is owned.
        if (combo_ > 1.0 && !model_.infiniteCombo() && now - lastComboClick_ > kComboWindowMs) {
            combo_ = 1.0;
            view_.setCombo(combo_);
        }

        // Passive income: once you've unlocked enough skins, points trickle
        // in on their own, idle-game style. Interval and unlock threshold
        // can both be improved via upgrades/skills.
        if (model_.unlockedCount() >= model_.idleUnlockCount()) {
            idleAccumMs_ += dt;
            double interval = model_.idleIntervalMs();
            while (idleAccumMs_ >= interval) {
                idleAccumMs_ -= interval;
                long points = (long) std::lround(kIdlePoints * model_.current().multiplier * model_.essenceMultiplier());
                points = std::max(1L, points);
                announceUnlocks(model_.registerPoints(points));
            }
        }

        // Easter egg: once per real second, a tiny (0.002%) chance for
        // Deven himself to fly across the screen.
        flybyAccumMs_ += dt;
        while (flybyAccumMs_ >= 1000.0) {
            flybyAccumMs_ -= 1000.0;
            if (((double) rand() / RAND_MAX) < kFlybyChancePerSecond) {
                js_trigger_flyby();
            }
        }
    }

private:
    bool debounceOk() {
        double now = js_now();
        if (now - lastClick_ < 60.0) return false;
        lastClick_ = now;
        return true;
    }

    void registerClickAt(double x, double y) {
        double now = js_now();
        double comboMax = kComboMax + model_.comboCapBonus();

        // Rapid clicks (within kComboWindowMs of each other) build a
        // multiplier, capped at comboMax; a pause resets it.
        if (now - lastComboClick_ <= kComboWindowMs) {
            combo_ = std::min(comboMax, combo_ + kComboStep);
        } else {
            combo_ = 1.0;
        }
        lastComboClick_ = now;
        view_.setCombo(combo_);

        double critChance = kCritChance + model_.critChanceBonus();
        bool crit = ((double) rand() / RAND_MAX) < critChance;
        int critMult = kCritMultiplier + model_.critDamageBonus();
        double skinMult = model_.current().multiplier;
        double flat = model_.flatClickBonus();
        double essenceMult = model_.essenceMultiplier();

        long points = (long) std::lround((combo_ * skinMult + flat) * (crit ? critMult : 1) * essenceMult);
        points = std::max(1L, points);

        auto justUnlocked = model_.registerPoints(points);
        particles_.burst(x, y, model_.current().particles, crit ? 22 : 12);

        if (!justUnlocked.empty()) {
            announceUnlocks(justUnlocked);
        } else if (crit) {
            std::string msg = std::string(u8"\U0001F4A5 critical click! +") + std::to_string(points);
            js_show_toast(msg.c_str());
        }
    }

    void announceUnlocks(const std::vector<const Skin*>& unlocked) {
        if (unlocked.empty()) return;
        const Skin* last = unlocked.back();
        std::string msg = std::string(u8"\U0001F513 new skin unlocked — ") + last->name;
        js_show_toast(msg.c_str());
    }

    SkinModel model_;
    SkinView view_;
    ParticleSystem particles_;
    bool skindexOpen_ = false;
    bool progressionOpen_ = false;

    double lastClick_ = 0.0;       // debounce guard (duplicate synthetic events)
    double lastComboClick_ = 0.0;  // combo window tracking
    double combo_ = 1.0;
    double lastTick_ = 0.0;
    double idleAccumMs_ = 0.0;
    double flybyAccumMs_ = 0.0;
};

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
// and passive/idle income once you're deep enough into the Skindex —
// the usual incremental-game toolkit (Cookie Clicker etc.), scaled way
// down.
class SkinController : public mvc::Controller {
public:
    static constexpr double kComboWindowMs = 900.0;
    static constexpr double kComboStep = 0.15;
    static constexpr double kComboMax = 3.0;
    static constexpr double kCritChance = 0.08;
    static constexpr int kCritMultiplier = 5;
    // kIdleUnlockCount / kIdleIntervalMs / kIdlePoints live in skin_model.hpp,
    // shared with SkinView (which shows the passive-income indicator).

    SkinController() : view_(model_) {
        model_.subscribe([this] { view_.render(); });
        view_.render();
    }

    void backgroundClick(double x, double y) {
        if (skindexOpen_) return;
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

    void tick() {
        particles_.tick();

        double now = js_now();
        if (lastTick_ == 0.0) { lastTick_ = now; return; } // skip first frame's huge dt
        double dt = now - lastTick_;
        lastTick_ = now;

        // Combo decays back to 1x once you stop clicking for a beat.
        if (combo_ > 1.0 && now - lastComboClick_ > kComboWindowMs) {
            combo_ = 1.0;
            view_.setCombo(combo_);
        }

        // Passive income: once you've unlocked enough skins, points trickle
        // in on their own, idle-game style.
        if (model_.unlockedCount() >= kIdleUnlockCount) {
            idleAccumMs_ += dt;
            while (idleAccumMs_ >= kIdleIntervalMs) {
                idleAccumMs_ -= kIdleIntervalMs;
                int points = std::max(1, (int) std::lround(kIdlePoints * model_.current().multiplier));
                announceUnlocks(model_.registerPoints(points));
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

        // Rapid clicks (within kComboWindowMs of each other) build a
        // multiplier, capped at kComboMax; a pause resets it.
        if (now - lastComboClick_ <= kComboWindowMs) {
            combo_ = std::min(kComboMax, combo_ + kComboStep);
        } else {
            combo_ = 1.0;
        }
        lastComboClick_ = now;
        view_.setCombo(combo_);

        bool crit = ((double) rand() / RAND_MAX) < kCritChance;
        double skinMult = model_.current().multiplier;
        int points = std::max(1, (int) std::lround(combo_ * skinMult * (crit ? kCritMultiplier : 1)));

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

    double lastClick_ = 0.0;       // debounce guard (duplicate synthetic events)
    double lastComboClick_ = 0.0;  // combo window tracking
    double combo_ = 1.0;
    double lastTick_ = 0.0;
    double idleAccumMs_ = 0.0;
};

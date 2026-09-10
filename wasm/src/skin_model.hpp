#pragma once
#include <string>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include "mvc.hpp"
#include "bridge.hpp"

// Shared with SkinController (which drives the timer) and SkinView
// (which shows/hides the "passive income" indicator once it's active).
constexpr int kIdleUnlockCount = 5;   // skins unlocked before idle income kicks in (before Idle Mastery)
constexpr double kIdleIntervalMs = 4000.0;
constexpr int kIdlePoints = 2;
constexpr int kRebirthMinPoints = 500; // must have banked this many points to rebirth

struct Skin {
    int id;
    std::string name;
    std::string emoji;
    int unlockAt;
    std::string accent;
    std::array<std::string, 4> particles;
    double multiplier;  // scales every point gain (clicks, crits, idle) while equipped
};

struct UpgradeDef {
    int id;
    std::string name;
    std::string desc;
    double baseCost;
    double growth;
    int maxLevel;
};

struct SkillDef {
    int id;
    std::string name;
    std::string desc;
    int cost;   // essence
    int tier;   // 1, 2, or 3 — shown as visual grouping
    std::vector<int> prereq; // owning ANY one of these unlocks purchase; empty = no prereq
};

// Owns all Skindex state: the skin roster + lifetime points (resets on
// rebirth), upgrade levels (reset on rebirth), and the skill tree +
// essence + rebirth count (permanent, survive rebirth). Persists
// everything to localStorage through the bridge. Notifies its View
// whenever something changes.
class SkinModel : public mvc::Model {
public:
    SkinModel() {
        roster_ = {
            {0, "Skin #01", u8"\U0001F9D1\U0001F3FB", 0,     "#ffb98a", {"#ffb98a", "#e69a63", "#ffd9b8", "#c97a3f"}, 1.0},
            {1, "Skin #02", u8"\U0001F9D1\U0001F3FC", 100,   "#f2c14e", {"#f2c14e", "#d9a52e", "#ffe08a", "#a67c1a"}, 1.2},
            {2, "Skin #03", u8"\U0001F9D1\U0001F3FD", 300,   "#ff6b6b", {"#ff6b6b", "#e04545", "#ff9e9e", "#b32c2c"}, 1.5},
            {3, "Skin #04", u8"\U0001F9D1\U0001F3FE", 750,   "#9d7fff", {"#9d7fff", "#7d5be0", "#c6b3ff", "#5a3dbf"}, 2.0},
            {4, "Skin #05", u8"\U0001F9D1\U0001F3FF", 1500,  "#4bd2c9", {"#4bd2c9", "#2ea89f", "#8cf0e6", "#1c7e77"}, 2.5},
            {5, "Skin #06", u8"\U0001F476\U0001F3FD", 3000,  "#ff8fc7", {"#ff8fc7", "#e0609f", "#ffc2e0", "#b83d7c"}, 3.5},
            {6, "Skin #07", u8"\U0001F9D3\U0001F3FB", 5500,  "#cfd8dc", {"#cfd8dc", "#90a4ae", "#eceff1", "#607d8b"}, 5.0},
            {7, "Skin #08", u8"\U0001F9D4\U0001F3FE", 9000,  "#d99a3d", {"#d99a3d", "#b87c22", "#f0c57a", "#8f5f14"}, 7.0},
            {8, "Skin #09", u8"\U0001F9D1\U0001F3FF\U0000200D\U0001F9B1", 15000, "#ff4d6d", {"#ff4d6d", "#d92e4d", "#ff8fa3", "#a3132c"}, 10.0},
        };

        upgradeDefs_ = {
            {0, "Thicker Skin",     "+1 flat point per click",            50,  1.4, 10},
            {1, "Combo Flex",       "+0.5 combo cap",                     100, 1.5, 5},
            {2, "Fast Metabolism",  "-300ms idle income interval",        150, 1.6, 10},
            {3, "Lucky Skin",       "+1.5% critical hit chance",          120, 1.5, 10},
            {4, "Crit Power",       "+1 critical hit multiplier",         200, 1.7, 10},
        };

        skillDefs_ = {
            {0, "Thick Skinned",     "Start each rebirth with +100 points",        1, 1, {}},
            {1, "Sticky Aura",       "+10% essence earned per rebirth",            1, 1, {}},
            {2, "Skin Deep Discount","-15% upgrade costs",                         2, 2, {0, 1}},
            {3, "Molting Season",    "Idle income needs only 3 skins, not 5",      2, 2, {0, 1}},
            {4, "Reskinned",         "+0.5 permanent global point multiplier",     4, 3, {2, 3}},
            {5, "Rhino Hide",        "Combo multiplier never decays",              4, 3, {2, 3}},
        };

        clicks_ = js_get_int("skindex_clicks", 0);
        currentId_ = js_get_int("skindex_current", 0);
        essence_ = js_get_int("skindex_essence", 0);
        rebirths_ = js_get_int("skindex_rebirths", 0);

        upgradeLevels_.resize(upgradeDefs_.size());
        for (auto& u : upgradeDefs_) {
            upgradeLevels_[u.id] = js_get_int(("skindex_upg" + std::to_string(u.id)).c_str(), 0);
        }
        skillOwned_.resize(skillDefs_.size());
        for (auto& s : skillDefs_) {
            skillOwned_[s.id] = js_get_int(("skindex_skill" + std::to_string(s.id)).c_str(), 0) != 0;
        }

        if (!isUnlocked(currentId_)) currentId_ = 0;
    }

    // ---------- skins ----------
    const std::vector<Skin>& roster() const { return roster_; }
    long clicks() const { return clicks_; }

    const Skin& current() const {
        return *std::find_if(roster_.begin(), roster_.end(),
                              [&](const Skin& s) { return s.id == currentId_; });
    }

    bool isUnlocked(int id) const {
        auto it = std::find_if(roster_.begin(), roster_.end(),
                                [&](const Skin& s) { return s.id == id; });
        return it != roster_.end() && clicks_ >= it->unlockAt;
    }

    int unlockedCount() const {
        int n = 0;
        for (auto& s : roster_) if (isUnlocked(s.id)) n++;
        return n;
    }

    // Adds `points` (already scaled by combo/skin/crit/essence multipliers
    // upstream) and returns every skin whose threshold got crossed — usually
    // zero or one, but a big crit can leapfrog more than one.
    std::vector<const Skin*> registerPoints(long points) {
        long before = clicks_;
        clicks_ += points;
        js_set_int("skindex_clicks", (int) clicks_);
        std::vector<const Skin*> justUnlocked;
        for (auto& s : roster_) {
            if (s.unlockAt > before && s.unlockAt <= clicks_) justUnlocked.push_back(&s);
        }
        notify();
        return justUnlocked;
    }

    void selectSkin(int id) {
        if (!isUnlocked(id)) return;
        currentId_ = id;
        js_set_int("skindex_current", id);
        notify();
    }

    // ---------- upgrades (reset on rebirth) ----------
    const std::vector<UpgradeDef>& upgradeDefs() const { return upgradeDefs_; }
    int upgradeLevel(int id) const { return upgradeLevels_[id]; }

    long upgradeCost(int id) const {
        const UpgradeDef& u = upgradeDefs_[id];
        double discount = hasSkill(2) ? 0.85 : 1.0;
        return (long) std::llround(u.baseCost * std::pow(u.growth, upgradeLevels_[id]) * discount);
    }

    bool buyUpgrade(int id) {
        const UpgradeDef& u = upgradeDefs_[id];
        if (upgradeLevels_[id] >= u.maxLevel) return false;
        long cost = upgradeCost(id);
        if (clicks_ < cost) return false;
        clicks_ -= cost;
        upgradeLevels_[id]++;
        js_set_int("skindex_clicks", (int) clicks_);
        js_set_int(("skindex_upg" + std::to_string(id)).c_str(), upgradeLevels_[id]);
        notify();
        return true;
    }

    double flatClickBonus() const { return upgradeLevels_[0]; }
    double comboCapBonus() const { return upgradeLevels_[1] * 0.5; }
    double idleIntervalMs() const { return std::max(1000.0, kIdleIntervalMs - upgradeLevels_[2] * 300.0); }
    double critChanceBonus() const { return upgradeLevels_[3] * 0.015; }
    int critDamageBonus() const { return upgradeLevels_[4]; }

    // ---------- skill tree (permanent, essence-funded) ----------
    const std::vector<SkillDef>& skillDefs() const { return skillDefs_; }
    bool hasSkill(int id) const { return skillOwned_[id]; }

    bool skillAvailable(int id) const {
        if (skillOwned_[id]) return false;
        const SkillDef& s = skillDefs_[id];
        if (essence_ < s.cost) return false;
        if (s.prereq.empty()) return true;
        for (int p : s.prereq) if (skillOwned_[p]) return true;
        return false;
    }

    bool buySkill(int id) {
        if (!skillAvailable(id)) return false;
        essence_ -= skillDefs_[id].cost;
        skillOwned_[id] = true;
        js_set_int("skindex_essence", essence_);
        js_set_int(("skindex_skill" + std::to_string(id)).c_str(), 1);
        notify();
        return true;
    }

    int idleUnlockCount() const { return hasSkill(3) ? 3 : kIdleUnlockCount; }
    bool infiniteCombo() const { return hasSkill(5); }

    double essenceMultiplier() const {
        return 1.0 + essence_ * 0.03 + (hasSkill(4) ? 0.5 : 0.0);
    }

    // ---------- rebirth ----------
    int essence() const { return essence_; }
    int rebirths() const { return rebirths_; }
    bool canRebirth() const { return clicks_ >= kRebirthMinPoints; }

    int projectedRebirthEssence() const {
        double bonus = hasSkill(1) ? 1.1 : 1.0;
        return (int) std::floor(std::sqrt((double) clicks_ / 100.0) * bonus);
    }

    void doRebirth() {
        if (!canRebirth()) return;
        essence_ += projectedRebirthEssence();
        rebirths_++;
        clicks_ = hasSkill(0) ? 100 : 0;
        currentId_ = 0;
        std::fill(upgradeLevels_.begin(), upgradeLevels_.end(), 0);

        js_set_int("skindex_essence", essence_);
        js_set_int("skindex_rebirths", rebirths_);
        js_set_int("skindex_clicks", (int) clicks_);
        js_set_int("skindex_current", currentId_);
        for (auto& u : upgradeDefs_) js_set_int(("skindex_upg" + std::to_string(u.id)).c_str(), 0);

        notify();
    }

private:
    std::vector<Skin> roster_;
    long clicks_ = 0;
    int currentId_ = 0;

    std::vector<UpgradeDef> upgradeDefs_;
    std::vector<int> upgradeLevels_;

    std::vector<SkillDef> skillDefs_;
    std::vector<bool> skillOwned_;

    int essence_ = 0;
    int rebirths_ = 0;
};

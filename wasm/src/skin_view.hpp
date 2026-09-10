#pragma once
#include <cstdio>
#include "mvc.hpp"
#include "skin_model.hpp"
#include "bridge.hpp"

// Pushes SkinModel state into the DOM through the bridge. Doesn't own
// input handling — that's the Controller's job.
class SkinView : public mvc::View {
public:
    explicit SkinView(const SkinModel& model) : model_(model) {}

    void setCombo(double multiplier) {
        js_set_combo(multiplier);
    }

    void setSkindexOpen(bool open) {
        skindexOpen_ = open;
        js_skindex_set_open(open ? 1 : 0);
        if (open) renderSkindex();
    }

    void setProgressionOpen(bool open) {
        progressionOpen_ = open;
        js_progression_set_open(open ? 1 : 0);
        if (open) renderProgression();
    }

    void render() override {
        const Skin& s = model_.current();
        js_set_avatar_emoji(s.emoji.c_str());
        js_set_accent(s.accent.c_str());
        js_set_skin_name(formatLabel(s).c_str());
        js_set_click_count((int) model_.clicks());
        js_set_idle_active(model_.unlockedCount() >= model_.idleUnlockCount() ? 1 : 0);
        js_set_prestige(model_.essence(), model_.rebirths(), (model_.essence() > 0 || model_.rebirths() > 0) ? 1 : 0);
        if (skindexOpen_) renderSkindex();
        if (progressionOpen_) renderProgression();
    }

private:
    static std::string formatMultiplier(double m) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1fx", m);
        return std::string(buf);
    }

    static std::string formatLabel(const Skin& s) {
        return s.name + " (" + formatMultiplier(s.multiplier) + ")";
    }

    void renderSkindex() {
        std::string sub = std::to_string(model_.unlockedCount()) + " / " +
                           std::to_string(model_.roster().size()) + " unlocked";
        js_skindex_set_sub(sub.c_str());
        js_skindex_clear();
        for (const Skin& s : model_.roster()) {
            bool unlocked = model_.isUnlocked(s.id);
            bool active = unlocked && s.id == model_.current().id;
            std::string label = unlocked ? formatMultiplier(s.multiplier) : "";
            js_skindex_add_tile(s.id, s.name.c_str(), s.emoji.c_str(), label.c_str(),
                                 unlocked ? 1 : 0, s.unlockAt, active ? 1 : 0);
        }
    }

    void renderProgression() {
        js_upgrades_clear();
        for (const UpgradeDef& u : model_.upgradeDefs()) {
            int level = model_.upgradeLevel(u.id);
            bool maxed = level >= u.maxLevel;
            long cost = model_.upgradeCost(u.id);
            bool canAfford = model_.clicks() >= cost;
            js_upgrades_add_row(u.id, u.name.c_str(), u.desc.c_str(), level, u.maxLevel,
                                 (int) cost, canAfford ? 1 : 0, maxed ? 1 : 0);
        }

        js_skills_clear();
        for (const SkillDef& s : model_.skillDefs()) {
            bool owned = model_.hasSkill(s.id);
            bool available = model_.skillAvailable(s.id);
            js_skills_add_node(s.id, s.name.c_str(), s.desc.c_str(), s.cost, s.tier,
                                owned ? 1 : 0, available ? 1 : 0);
        }

        js_rebirth_set_preview(model_.projectedRebirthEssence(), model_.canRebirth() ? 1 : 0, kRebirthMinPoints);
    }

    const SkinModel& model_;
    bool skindexOpen_ = false;
    bool progressionOpen_ = false;
};

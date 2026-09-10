#pragma once
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

    void render() override {
        const Skin& s = model_.current();
        js_set_avatar_emoji(s.emoji.c_str());
        js_set_accent(s.accent.c_str());
        js_set_skin_name(s.name.c_str());
        js_set_click_count(model_.clicks());
        js_set_idle_active(model_.unlockedCount() >= kIdleUnlockCount ? 1 : 0);
        if (skindexOpen_) renderSkindex();
    }

private:
    void renderSkindex() {
        std::string sub = std::to_string(model_.unlockedCount()) + " / " +
                           std::to_string(model_.roster().size()) + " unlocked";
        js_skindex_set_sub(sub.c_str());
        js_skindex_clear();
        for (const Skin& s : model_.roster()) {
            bool unlocked = model_.isUnlocked(s.id);
            bool active = unlocked && s.id == model_.current().id;
            js_skindex_add_tile(s.id, s.name.c_str(), s.emoji.c_str(),
                                 unlocked ? 1 : 0, s.unlockAt, active ? 1 : 0);
        }
    }

    const SkinModel& model_;
    bool skindexOpen_ = false;
};

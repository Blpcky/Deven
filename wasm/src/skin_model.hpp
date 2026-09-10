#pragma once
#include <string>
#include <array>
#include <vector>
#include <algorithm>
#include "mvc.hpp"
#include "bridge.hpp"

// Shared with SkinController (which drives the timer) and SkinView
// (which shows/hides the "passive income" indicator once it's active).
constexpr int kIdleUnlockCount = 5;   // skins unlocked before idle income kicks in
constexpr double kIdleIntervalMs = 4000.0;
constexpr int kIdlePoints = 2;

struct Skin {
    int id;
    std::string name;
    std::string emoji;
    int unlockAt;
    std::string accent;
    std::array<std::string, 4> particles;
    double multiplier;  // scales every point gain (clicks, crits, idle) while equipped
};

// Owns all Skindex state: the roster, lifetime click count, which skin
// is currently equipped. Persists to localStorage through the bridge.
// Notifies its View whenever something changes.
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

        clicks_ = js_get_int("skindex_clicks", 0);
        currentId_ = js_get_int("skindex_current", 0);
        if (!isUnlocked(currentId_)) currentId_ = 0;
    }

    const std::vector<Skin>& roster() const { return roster_; }

    int clicks() const { return clicks_; }

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

    // Adds `points` (already scaled by combo/crit multipliers upstream)
    // and returns every skin whose threshold got crossed by this gain —
    // usually zero or one, but a big crit can leapfrog more than one.
    std::vector<const Skin*> registerPoints(int points) {
        int before = clicks_;
        clicks_ += points;
        js_set_int("skindex_clicks", clicks_);
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

private:
    std::vector<Skin> roster_;
    int clicks_ = 0;
    int currentId_ = 0;
};

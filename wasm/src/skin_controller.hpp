#pragma once
#include "mvc.hpp"
#include "skin_model.hpp"
#include "skin_view.hpp"
#include "particle_system.hpp"
#include "bridge.hpp"

// Wires the SkinModel + SkinView together and handles every input event
// forwarded from JS.
class SkinController : public mvc::Controller {
public:
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
    }

private:
    bool debounceOk() {
        double now = js_now();
        if (now - lastClick_ < 60.0) return false;
        lastClick_ = now;
        return true;
    }

    void registerClickAt(double x, double y) {
        const Skin* justUnlocked = model_.registerClick();
        particles_.burst(x, y, model_.current().particles);
        if (justUnlocked) {
            std::string msg = std::string(u8"\U0001F513 new skin unlocked — ") + justUnlocked->name;
            js_show_toast(msg.c_str());
        }
    }

    SkinModel model_;
    SkinView view_;
    ParticleSystem particles_;
    bool skindexOpen_ = false;
    double lastClick_ = 0.0;
};

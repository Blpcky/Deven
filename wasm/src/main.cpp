// Entry point: owns the single SkinController instance, exposes the
// C ABI that the thin JS shell in index.html calls into, and drives
// the particle system's per-frame update via the browser's rAF loop.
#include <emscripten.h>
#include <memory>
#include "skin_controller.hpp"

static std::unique_ptr<SkinController> g_controller;

static void main_loop() {
    if (g_controller) g_controller->tick();
}

extern "C" {

EMSCRIPTEN_KEEPALIVE
void app_init() {
    g_controller = std::make_unique<SkinController>();
    // simulate_infinite_loop=0: we don't need it (nothing follows this
    // call), and =1 makes Emscripten throw a JS exception to unwind the
    // C++ stack — fine when its own runtime calls app_init, but fatal
    // here since we invoke it via Module.ccall() ourselves and that
    // exception would otherwise escape into our caller and abort the
    // rest of the JS setup (event listeners never get attached).
    emscripten_set_main_loop(main_loop, 0, 0);
}

EMSCRIPTEN_KEEPALIVE
void app_background_click(double x, double y) {
    if (g_controller) g_controller->backgroundClick(x, y);
}

EMSCRIPTEN_KEEPALIVE
void app_avatar_click(double x, double y) {
    if (g_controller) g_controller->avatarClick(x, y);
}

EMSCRIPTEN_KEEPALIVE
void app_skindex_open() {
    if (g_controller) g_controller->skindexOpen();
}

EMSCRIPTEN_KEEPALIVE
void app_skindex_close() {
    if (g_controller) g_controller->skindexClose();
}

EMSCRIPTEN_KEEPALIVE
void app_skindex_tile_click(int id) {
    if (g_controller) g_controller->skindexTileClick(id);
}

EMSCRIPTEN_KEEPALIVE
void app_progression_open() {
    if (g_controller) g_controller->progressionOpen();
}

EMSCRIPTEN_KEEPALIVE
void app_progression_close() {
    if (g_controller) g_controller->progressionClose();
}

EMSCRIPTEN_KEEPALIVE
void app_buy_upgrade(int id) {
    if (g_controller) g_controller->buyUpgrade(id);
}

EMSCRIPTEN_KEEPALIVE
void app_buy_skill(int id) {
    if (g_controller) g_controller->buySkill(id);
}

EMSCRIPTEN_KEEPALIVE
void app_do_rebirth() {
    if (g_controller) g_controller->doRebirth();
}

} // extern "C"

int main() {
    // Real work happens in app_init(), called once the JS shell is ready.
    return 0;
}

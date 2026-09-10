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
    emscripten_set_main_loop(main_loop, 0, 1);
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

} // extern "C"

int main() {
    // Real work happens in app_init(), called once the JS shell is ready.
    return 0;
}

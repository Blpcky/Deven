#pragma once
// The JS bridge: every function the C++ side uses to touch the DOM.
// Implemented inline with EM_JS so the JS body lives right next to the
// C++ declaration that calls it.
//
// Note: JS string literals in these bodies use double quotes, not
// single quotes — the C preprocessor tokenizes EM_JS bodies too, and a
// bare `'` reads as (the start of) a C character literal.
#include <emscripten.h>

// ---------- persistence (localStorage, wrapped in try/catch on the JS side) ----------
EM_JS(int, js_get_int, (const char* key, int fallback), {
    try {
        var v = parseInt(localStorage.getItem(UTF8ToString(key)), 10);
        return isNaN(v) ? fallback : v;
    } catch (e) { return fallback; }
});

EM_JS(void, js_set_int, (const char* key, int value), {
    try { localStorage.setItem(UTF8ToString(key), String(value)); } catch (e) {}
});

// ---------- avatar / header ----------
EM_JS(void, js_set_avatar_emoji, (const char* emoji), {
    document.getElementById("avatar-frame").textContent = UTF8ToString(emoji);
});

EM_JS(void, js_set_accent, (const char* hex), {
    document.documentElement.style.setProperty("--accent", UTF8ToString(hex));
});

EM_JS(void, js_set_skin_name, (const char* name), {
    document.getElementById("skin-name").textContent = UTF8ToString(name);
});

EM_JS(void, js_set_click_count, (int n), {
    document.getElementById("click-count").textContent = String(n);
});

// ---------- combo readout ----------
EM_JS(void, js_set_combo, (double multiplier), {
    var el = document.getElementById("combo");
    if (!el) return;
    if (multiplier > 1.05) {
        el.textContent = "x" + multiplier.toFixed(1) + " combo";
        el.classList.add("show");
    } else {
        el.classList.remove("show");
    }
});

// ---------- passive income indicator ----------
EM_JS(void, js_set_idle_active, (int active), {
    var el = document.getElementById("idle");
    if (el) el.classList.toggle("show", !!active);
});

// ---------- toast ----------
EM_JS(void, js_show_toast, (const char* text), {
    var el = document.getElementById("toast");
    clearTimeout(el._t);
    el.textContent = UTF8ToString(text);
    el.classList.add("show");
    el._t = setTimeout(function () { el.classList.remove("show"); }, 2200);
});

// ---------- skindex overlay ----------
EM_JS(void, js_skindex_set_open, (int open), {
    document.getElementById("skindex-overlay").classList.toggle("show", !!open);
});

EM_JS(void, js_skindex_set_sub, (const char* text), {
    document.getElementById("skindex-sub").textContent = UTF8ToString(text);
});

EM_JS(void, js_skindex_clear, (), {
    document.getElementById("skindex-grid").innerHTML = "";
});

EM_JS(void, js_skindex_add_tile, (int id, const char* name, const char* emoji, const char* multLabel, int unlocked, int unlockAt, int active), {
    var grid = document.getElementById("skindex-grid");
    var tile = document.createElement("div");
    tile.className = "skindex-tile" + (unlocked ? "" : " locked") + (active ? " active" : "");
    if (unlocked) {
        tile.innerHTML = "<div class=\"skindex-emoji\">" + UTF8ToString(emoji) + "</div><div class=\"skindex-name\">" + UTF8ToString(name) + "</div><div class=\"skindex-mult\">" + UTF8ToString(multLabel) + "</div>";
        tile.addEventListener("click", function (e) {
            e.stopPropagation();
            Module.ccall("app_skindex_tile_click", null, ["number"], [id]);
        });
    } else {
        tile.innerHTML = "<div class=\"skindex-emoji\">❔</div><div class=\"skindex-name\">\?\?\?</div><div class=\"skindex-lock\">🔒 " + unlockAt + "</div>";
    }
    grid.appendChild(tile);
});

// ---------- particles ----------
EM_JS(void, js_particle_create, (int id, const char* colorHex, double size), {
    var p = document.createElement("div");
    p.className = "particle";
    p.id = "p" + id;
    p.style.width = size + "px";
    p.style.height = size + "px";
    p.style.background = UTF8ToString(colorHex);
    document.body.appendChild(p);
});

EM_JS(void, js_particle_update, (int id, double x, double y, double opacity), {
    var p = document.getElementById("p" + id);
    if (!p) return;
    p.style.transform = "translate(" + x + "px," + y + "px)";
    p.style.opacity = opacity;
});

EM_JS(void, js_particle_remove, (int id), {
    var p = document.getElementById("p" + id);
    if (p) p.remove();
});

// ---------- timing ----------
EM_JS(double, js_now, (), {
    return performance.now();
});

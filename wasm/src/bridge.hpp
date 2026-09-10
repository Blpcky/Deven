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

// ---------- prestige stat line (essence / rebirths) ----------
EM_JS(void, js_set_prestige, (int essence, int rebirths, int visible), {
    var el = document.getElementById("prestige-stat");
    if (!el) return;
    el.textContent = "✨ " + essence + " essence · " + rebirths + (rebirths === 1 ? " rebirth" : " rebirths");
    el.classList.toggle("show", !!visible);
});

// ---------- progression overlay (upgrades / skill tree / rebirth) ----------
EM_JS(void, js_progression_set_open, (int open), {
    document.getElementById("progression-overlay").classList.toggle("show", !!open);
});

EM_JS(void, js_upgrades_clear, (), {
    document.getElementById("upgrades-list").innerHTML = "";
});

EM_JS(void, js_upgrades_add_row, (int id, const char* name, const char* desc, int level, int maxLevel, int cost, int canAfford, int maxed), {
    var list = document.getElementById("upgrades-list");
    var row = document.createElement("div");
    row.className = "upg-row";
    var btnLabel = maxed ? "MAX" : ("+" + cost + " pts");
    row.innerHTML =
        "<div class=\"upg-info\">" +
          "<div class=\"upg-name\">" + UTF8ToString(name) + " <span class=\"upg-level\">Lv " + level + "/" + maxLevel + "</span></div>" +
          "<div class=\"upg-desc\">" + UTF8ToString(desc) + "</div>" +
        "</div>" +
        "<button class=\"upg-buy\"" + ((maxed || !canAfford) ? " disabled" : "") + ">" + btnLabel + "</button>";
    if (!maxed) {
        row.querySelector(".upg-buy").addEventListener("click", function (e) {
            e.stopPropagation();
            Module.ccall("app_buy_upgrade", null, ["number"], [id]);
        });
    }
    list.appendChild(row);
});

EM_JS(void, js_skills_clear, (), {
    document.getElementById("skills-list").innerHTML = "";
});

EM_JS(void, js_skills_add_node, (int id, const char* name, const char* desc, int cost, int tier, int owned, int available), {
    var list = document.getElementById("skills-list");
    var node = document.createElement("div");
    node.className = "skill-node" + (owned ? " owned" : "") + (!owned && !available ? " locked" : "");
    node.innerHTML =
        "<div class=\"skill-tier\">tier " + tier + "</div>" +
        "<div class=\"skill-name\">" + UTF8ToString(name) + "</div>" +
        "<div class=\"skill-desc\">" + UTF8ToString(desc) + "</div>" +
        "<div class=\"skill-cost\">" + (owned ? "✓ owned" : ("✨ " + cost + " essence")) + "</div>";
    if (!owned && available) {
        node.addEventListener("click", function (e) {
            e.stopPropagation();
            Module.ccall("app_buy_skill", null, ["number"], [id]);
        });
    }
    list.appendChild(node);
});

EM_JS(void, js_rebirth_set_preview, (int essenceGain, int canRebirth, int minPoints), {
    var btn = document.getElementById("rebirth-btn");
    var note = document.getElementById("rebirth-note");
    if (canRebirth) {
        note.textContent = "Reset your points and upgrades for +" + essenceGain + " essence, permanently.";
        btn.disabled = false;
        btn.textContent = "Rebirth for +" + essenceGain + " essence";
    } else {
        note.textContent = "Reach " + minPoints + " points to unlock rebirth.";
        btn.disabled = true;
        btn.textContent = "Rebirth (locked)";
    }
});

// ---------- flyby easter egg ----------
EM_JS(void, js_trigger_flyby, (), {
    var wrap = document.createElement("div");
    wrap.className = "flyby";
    wrap.style.top = (8 + Math.random() * 70) + "vh";

    var img = document.createElement("img");
    img.src = "deven-flyby.png";
    img.alt = "";
    img.onerror = function () {
        img.remove();
        var fallback = document.createElement("div");
        fallback.className = "flyby-fallback";
        fallback.textContent = "🧑";
        wrap.appendChild(fallback);
    };
    wrap.appendChild(img);

    document.body.appendChild(wrap);
    wrap.addEventListener("animationend", function () { wrap.remove(); });
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

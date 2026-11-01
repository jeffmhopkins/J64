// Clankers on the N64: reads the level and world files from the ROM's filesystem, runs the
// shared game core (src/core/) at 60 ticks a second, draws it with rdpq and plays its sounds.
#include <libdragon.h>
#include <malloc.h>
#include <string.h>

#include "ck.h"
#include "draw.h"
#include "synth.h"
#ifdef CK_AUTOTEST
#include "ck_autotest.h"
#endif

static ck_game G;
static bool scanlines;

// A whole file from the ROM filesystem, zero-terminated. NULL if it is missing.
static char *slurp(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *s = malloc(n + 1);
    n = fread(s, 1, n, f);
    s[n] = 0;
    fclose(f);
    return s;
}

// ---------------------------------------------------------------------------- saving (EEPROM 4K)

static ck_prog saved;
static bool have_eeprom;

__attribute__((unused)) static void save_load(ck_prog *out) {
    have_eeprom = eeprom_present() != EEPROM_NONE;
    if (!have_eeprom) return;
    eeprom_read_bytes(out, 0, sizeof *out);
    saved = *out;
}

// Write the 8-byte blocks that changed. EEPROM writes are slow, so this only runs between scenes.
static void save_now(void) {
    if (!have_eeprom) return;
    const uint8_t *a = (const uint8_t *)&G.prog, *b = (const uint8_t *)&saved;
    for (size_t off = 0; off < sizeof G.prog; off += 8) {
        size_t n = sizeof G.prog - off < 8 ? sizeof G.prog - off : 8;
        if (memcmp(a + off, b + off, n)) eeprom_write_bytes(a + off, off, n);
    }
    saved = G.prog;
}

// ---------------------------------------------------------------------------- input

__attribute__((unused)) static uint16_t read_pad(joypad_port_t port) {
    if (!joypad_is_connected(port)) return 0;
    joypad_inputs_t in = joypad_get_inputs(port);
    uint16_t b = 0;
    if (in.btn.d_left || in.stick_x < -36) b |= CK_LEFT;
    if (in.btn.d_right || in.stick_x > 36) b |= CK_RIGHT;
    if (in.btn.d_up || in.stick_y > 40) b |= CK_UP;
    if (in.btn.d_down || in.stick_y < -40) b |= CK_DOWN;
    if (in.btn.a) b |= CK_A;
    if (in.btn.b || in.btn.c_left) b |= CK_B;
    if (in.btn.start) b |= CK_START;
    return b;
}

// ---------------------------------------------------------------------------- main

static void fatal(const char *what) {
    debugf("clankers: %s\n", what);
    display_close();
    console_init();
    printf("Clankers could not start:\n%s\n", what);
    for (;;) {}
}

#ifdef CK_AUTOTEST
// The self-playing test build: a scripted tour logs CHECK lines (with the slowest frame and the
// free memory since the last one), then PASS or FAIL. tools/win-test.ps1 reads them.
static ck_autotest A;
static char overlay[112] = "TEST";
static uint32_t worst_us;
static void at_report(int r) {
    struct mallinfo m = mallinfo();
    const char *tag = r == AT_PASS ? "PASS" : r == AT_FAIL ? "FAIL" : "CHECK";
    debugf("%s %s | frame %d scene %s lives %d heap %d KB worst %lu us\n", tag, A.msg, A.frames,
           ck_scene_name(G.scene), G.prog.lives, m.uordblks / 1024, (unsigned long)worst_us);
    snprintf(overlay, sizeof overlay, "%s %s", tag, A.msg);
    worst_us = 0;
}
#endif

int main(void) {
    debug_init_usblog();
    debug_init_emulog();
#ifdef CK_AUTOTEST
    debug_init_isviewer();
#endif
    debugf("Clankers %s booting\n", J64_VERSION);

    dfs_init(DFS_DEFAULT_LOCATION);
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    joypad_init();

    char *world = slurp("rom:/data/w1/world.txt");
    if (!world) fatal("rom:/data/w1/world.txt is missing");
    ck_prog prog;
    memset(&prog, 0, sizeof prog);
#ifndef CK_AUTOTEST
    save_load(&prog);   // the test build always starts from a fresh save and never writes one
#endif
    if (!ck_game_init(&G, world, &prog)) fatal("world.txt has no map");
    free(world);
    for (int i = 0; i < G.world.nnodes; i++) {
        if (!G.world.nodes[i].level[0] || G.hasDef[i]) continue;
        char path[64];
        snprintf(path, sizeof path, "rom:/data/w1/%s.txt", G.world.nodes[i].level);
        char *t = slurp(path);
        if (t) { ck_game_add_level(&G, G.world.nodes[i].level, t); free(t); }
        else debugf("missing %s\n", path);
    }
    char *songs = slurp("rom:/data/songs.txt");
    synth_init(songs);
    free(songs);
    draw_init();
#ifdef CK_AUTOTEST
    ck_autotest_init(&A, &G);
    debugf("START clankers-test %s\n", J64_VERSION);
#endif

    // Fixed 60 Hz game ticks, whatever the TV's refresh (PAL runs at 50 Hz).
    const uint32_t tick_us = 16667;
    uint64_t last = get_ticks_us(), acc = 0;
    int prev_scene = G.scene;
    while (1) {
        uint64_t now = get_ticks_us();
        acc += now - last;
        last = now;
        if (acc > tick_us * 4) acc = tick_us * 4;   // after a long load, do not race to catch up
        while (acc >= tick_us) {
            acc -= tick_us;
            joypad_poll();
#ifdef CK_AUTOTEST
            int r = ck_autotest_step(&A, &G);
            if (r != AT_NONE) at_report(r);
#else
            uint16_t p1 = read_pad(JOYPAD_PORT_1), p2 = read_pad(JOYPAD_PORT_2);
            joypad_buttons_t hit = joypad_get_buttons_pressed(JOYPAD_PORT_1);
            if (G.scene == SC_PAUSE && hit.z) scanlines = !scanlines;
            ck_game_step(&G, p1, p2);
#endif
            for (int i = 0; i < G.nsfx; i++) synth_sfx(G.sfx[i]);
            synth_song(G.song);
            if (G.scene != prev_scene) {
                if (G.progDirty && (G.scene == SC_MAP || G.scene == SC_SELECT)) { save_now(); G.progDirty = false; }
                prev_scene = G.scene;
            }
        }
        synth_poll();
        surface_t *disp = display_get();
        rdpq_attach(disp, NULL);
        draw_frame(&G, scanlines);
#ifdef CK_AUTOTEST
        draw_overlay(overlay);
#endif
        rdpq_detach_show();
#ifdef CK_AUTOTEST
        uint32_t spent = (uint32_t)(get_ticks_us() - now);
        if (spent > worst_us) worst_us = spent;
#endif
        synth_poll();
    }
}

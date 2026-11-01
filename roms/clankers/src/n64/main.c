// Clankers on the N64: reads the level and world files from the ROM's filesystem, runs the
// shared game core (src/core/) at 60 ticks a second, draws it with rdpq and plays its sounds.
#include <libdragon.h>
#include <malloc.h>
#include <string.h>

#include "ck.h"
#include "draw.h"
#include "synth.h"

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

static void save_load(ck_prog *out) {
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

static uint16_t read_pad(joypad_port_t port) {
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

int main(void) {
    debug_init_usblog();
    debug_init_emulog();
    debugf("Clankers %s booting\n", J64_VERSION);

    dfs_init(DFS_DEFAULT_LOCATION);
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, FILTERS_RESAMPLE);
    rdpq_init();
    joypad_init();

    char *world = slurp("rom:/data/w1/world.txt");
    if (!world) fatal("rom:/data/w1/world.txt is missing");
    ck_prog prog;
    memset(&prog, 0, sizeof prog);
    save_load(&prog);
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
            uint16_t p1 = read_pad(JOYPAD_PORT_1), p2 = read_pad(JOYPAD_PORT_2);
            joypad_buttons_t hit = joypad_get_buttons_pressed(JOYPAD_PORT_1);
            if (G.scene == SC_PAUSE && hit.z) scanlines = !scanlines;
            ck_game_step(&G, p1, p2);
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
        rdpq_detach_show();
        synth_poll();
    }
}

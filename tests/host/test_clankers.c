// Host tests for the Clankers game core (roms/clankers/src/core): the data files parse the way
// the browser reads them, and the rules behave (bolts, stomps, lives, the map opening up).
// Runs from the repo root so it reads the real data files.
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ck_autotest.h"
#include "ck_internal.h"
#include "test.h"

static char *slurp(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); test_failures++; return NULL; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *s = malloc((size_t)n + 1);
    if (fread(s, 1, (size_t)n, f) != (size_t)n) n = 0;
    s[n] = 0;
    fclose(f);
    return s;
}

#define DATA_ROOT "roms/clankers/data/"
#define DATA DATA_ROOT "w1/"

// The core reads every world's files through this, as the N64 reads them from rom:/data/.
static char *load_data(const char *path, void *user) {
    (void)user;
    char full[128];
    snprintf(full, sizeof full, DATA_ROOT "%s", path);
    FILE *f = fopen(full, "rb");
    if (!f) return NULL;   // a missing world is not a failure: the core asks to find out
    fclose(f);
    return slurp(full);
}

// A fresh game in World 1, from the real data files.
static ck_game *load_game(void) {
    static ck_game G;
    EXPECT_EQ_INT(ck_game_boot(&G, load_data, NULL, NULL), 1);
    EXPECT_EQ_INT(G.worldNum, 1);
    return &G;
}

static void test_parse(void) {
    ck_leveldef L;
    char *t = slurp(DATA "1-1.txt");
    if (!t) return;
    EXPECT_EQ_INT(ck_parse_level(t, &L), 1);
    free(t);
    EXPECT_EQ_INT(strcmp(L.name, "1-1 SCRAP PATH"), 0);
    EXPECT_EQ_INT(L.kind, CK_KIND_LEVEL);
    EXPECT_EQ_INT(L.song, SONG_LEVEL);
    EXPECT_EQ_INT(L.baby, 0xFF8FC7);
    EXPECT_EQ_INT(L.par, 75);
    EXPECT_EQ_INT(L.w, 200);
    EXPECT_EQ_INT(L.rows[11][2], '@');
    EXPECT_EQ_INT(L.rows[12][0], '#');

    t = slurp(DATA "1-2.txt");
    if (t) { ck_parse_level(t, &L); free(t); }
    EXPECT_EQ_INT(L.time, 200);
    EXPECT_EQ_INT(L.song, SONG_LEVEL2);

    t = slurp(DATA "boss.txt");
    if (t) { ck_parse_level(t, &L); free(t); }
    EXPECT_EQ_INT(L.kind, CK_KIND_BOSS);
    EXPECT_EQ_INT(L.w, 20);

    // short maps sit at the bottom, unknown characters become empty
    EXPECT_EQ_INT(ck_parse_level("name: tiny\n---\n#Z#\n", &L), 1);
    EXPECT_EQ_INT(L.w, 20);
    EXPECT_EQ_INT(L.rows[14][0], '#');
    EXPECT_EQ_INT(L.rows[14][1], '.');
    EXPECT_EQ_INT(L.rows[13][0], '.');
}

static void test_world(void) {
    ck_game *G = load_game();
    if (!G) return;
    const ck_world *W = &G->world;
    EXPECT_EQ_INT(W->nnodes, 8);
    EXPECT_EQ_INT(W->nroads, 7);
    EXPECT_EQ_INT(W->gears, 9);
    EXPECT_EQ_INT(strcmp(W->name, "Junkyard Hills"), 0);
    int bonus = ck_world_node(W, "bonus"), l1 = ck_world_node(W, "l1"), l2 = ck_world_node(W, "l2");
    EXPECT_EQ_INT(W->nodes[bonus].gate, 3);
    EXPECT_EQ_INT(W->nodes[bonus].kind, CK_NODE_BONUS);
    EXPECT_EQ_INT(W->nodes[l2].num, 2);
    EXPECT_EQ_INT(W->nodes[l1].x, 5 * 16 + 8);
    EXPECT_EQ_INT(strcmp(W->nodes[bonus].label, "Bonus \xC2\xB7 Bolt Shower"), 0);
    // road l3 -> boss goes via 9,11: three points, 64 px long
    const ck_road *r = &W->roads[5];
    EXPECT_EQ_INT(r->npts, 3);
    EXPECT_EQ_INT((int)r->total, 4 * 16 + 3 * 16);
    for (int i = 0; i < W->nnodes; i++)
        if (W->nodes[i].level[0]) EXPECT_EQ_INT(G->hasDef[i], 1);

    // a fresh save: only the road from home is open
    EXPECT_EQ_INT(ck_node_reachable(G, l1), 1);
    EXPECT_EQ_INT(ck_node_reachable(G, l2), 0);
    // beating 1-1 opens the road to 1-2
    G->map.at = l1;
    ck_map_beat(G, 10, 2);
    EXPECT_EQ_INT(ck_node_beaten(G, l1), 1);
    EXPECT_EQ_INT(ck_node_reachable(G, l2), 1);
    EXPECT_EQ_INT(G->prog.gears, 2);
    EXPECT_EQ_INT(G->prog.bolts, 10);
}

static void steps(ck_game *G, int n, uint16_t pad) { for (int i = 0; i < n; i++) ck_game_step(G, pad, 0); }

static int start_level(ck_game *G, const char *id) {
    int n = ck_world_node(&G->world, id);
    G->scene = SC_LEVEL;
    G->snap.valid = false;
    ck_level_start(G, n, false, false);
    return n;
}

static void test_level_rules(void) {
    ck_game *G = load_game();
    if (!G) return;
    start_level(G, "l1");
    ck_level *lv = &G->lv;
    EXPECT_EQ_INT(lv->nplayers, 1);
    EXPECT_EQ_INT((int)lv->startX, 34);
    steps(G, 60, 0);
    ck_player *p = &lv->players[0];
    EXPECT_EQ_INT(p->ground, 1);
    EXPECT_EQ_INT((int)(p->y + p->h), 192);
    // walking right through the first row of bolts picks them up
    steps(G, 110, CK_RIGHT);
    EXPECT_EQ_INT(lv->bolts >= 5, 1);
    // bumping a Glitchy with no power-ups drops bolts; with no bolts it costs a try
    int lives = G->prog.lives;
    lv->bolts = 0;
    ck_ent *foe = NULL;
    for (int i = 0; i < lv->nents; i++) if (lv->ents[i].type == E_ENEMY && !lv->ents[i].dead) { foe = &lv->ents[i]; break; }
    if (!foe) { test_failures++; return; }
    p->x = foe->x; p->y = foe->y + foe->h - p->h; p->vy = 0; p->inv = 0;
    steps(G, 1, 0);
    EXPECT_EQ_INT(G->prog.lives, lives - 1);
    EXPECT_EQ_INT(lv->dyingT >= 0, 1);
    steps(G, 160, 0);   // try again from the start
    EXPECT_EQ_INT(G->scene, SC_LEVEL);
    EXPECT_EQ_INT(lv->retry, 1);

    // stomping a Bumblebot reboots it
    start_level(G, "l1");
    steps(G, 30, 0);
    foe = NULL;
    for (int i = 0; i < lv->nents; i++) if (lv->ents[i].type == E_ENEMY && lv->ents[i].kind == EN_BUMBLE) { foe = &lv->ents[i]; break; }
    if (!foe) { test_failures++; return; }
    p = &lv->players[0];
    p->x = foe->x + 1; p->y = foe->y - p->h - 4; p->vy = 3; p->inv = 0;
    foe->vx = 0;
    steps(G, 2, 0);
    EXPECT_EQ_INT(foe->state, ES_HAPPY);
    EXPECT_EQ_INT(p->vy < 0, 1);

    // the race clock on 1-2 costs a try when it runs out
    start_level(G, "l2");
    lives = G->prog.lives;
    lv->timeF = lv->def->time * 60 - 1;
    steps(G, 2, 0);
    EXPECT_EQ_INT(lv->timeUp, 1);
    EXPECT_EQ_INT(G->prog.lives, lives - 1);
}

static void test_bot_runs(void) {
    ck_game *G = load_game();
    if (!G) return;
    const char *ids[] = { "l1", "l2", "l3", "bonus", "boss" };
    for (int k = 0; k < 5; k++) {
        G->prog.lives = 3;
        int node = start_level(G, ids[k]);
        int frames = 0;
        float far = 0;
        while (frames < 60 * 240 && G->scene != SC_MAP) {
            if (G->scene == SC_PAUSE) { ck_game_step(G, CK_START, 0); frames++; continue; }
            uint16_t pad = G->scene == SC_LEVEL && G->lv.nplayers ? ck_bot(&G->lv) : CK_RIGHT;
            ck_game_step(G, pad, 0);
            if (G->scene == SC_LEVEL && G->lv.players[0].x > far) far = G->lv.players[0].x;
            frames++;
        }
        printf("  bot on %-6s: %s after %d frames, got to x=%d of %d\n", ids[k],
               ck_node_beaten(G, node) ? "cleared" : G->scene == SC_MAP ? "out of tries" : "still going",
               frames, (int)far, G->lv.w * 16);
        EXPECT_EQ_INT(G->nparts <= CK_MAXPARTS, 1);
        G->scene = SC_LEVEL;
    }
    // the bonus level always ends (the shower stops after 30 s)
    EXPECT_EQ_INT(ck_node_beaten(G, ck_world_node(&G->world, "bonus")), 1);
}

// Mashing: random buttons on both controllers through every screen and every level, with
// checks that nothing runs away. Build with sanitizers (see docs/testing.md) to catch overflows.
static uint32_t fz;
static uint32_t fuzz_rand(void) { fz ^= fz << 13; fz ^= fz >> 17; fz ^= fz << 5; return fz; }
static uint16_t fuzz_pad(int *hold, uint16_t *cur) {
    if (--*hold <= 0) { *cur = (uint16_t)(fuzz_rand() & 0x7F); *hold = 1 + (int)(fuzz_rand() % 40); }
    return *cur;
}
static void fuzz_check(const ck_game *G) {
    EXPECT_EQ_INT(G->scene >= SC_TITLE && G->scene <= SC_SLOTS, 1);
    EXPECT_EQ_INT(G->worldNum >= 1 && G->worldNum <= CK_WORLDS, 1);
    EXPECT_EQ_INT(G->nparts >= 0 && G->nparts <= CK_MAXPARTS, 1);
    EXPECT_EQ_INT(G->nsfx >= 0 && G->nsfx <= CK_MAXSFX, 1);
    EXPECT_EQ_INT(G->song >= 0 && G->song < SONG_COUNT, 1);
    EXPECT_EQ_INT(G->prog.lives >= 0 && G->prog.lives <= 9, 1);
    if (G->scene == SC_LEVEL || G->scene == SC_PAUSE)
        for (int k = 0; k < G->lv.nplayers; k++) {
            const ck_player *p = &G->lv.players[k];
            EXPECT_EQ_INT(p->x == p->x && p->y == p->y, 1);   // not NaN
            EXPECT_EQ_INT(p->x > -1000 && p->x < G->lv.w * 16 + 1000, 1);
        }
}
static void test_fuzz(void) {
    static const char *ids[] = { "l1", "l2", "l3", "bonus", "boss" };
    for (uint32_t seed = 1; seed <= 6; seed++) {
        ck_game *G = load_game();
        if (!G) return;
        fz = seed * 2654435761u;
        int h1 = 0, h2 = 0;
        uint16_t c1 = 0, c2 = 0;
        int before = test_failures;
        // the whole game from the intro, both controllers
        for (int f = 0; f < 60 * 60 * 3 && test_failures == before; f++) {
            ck_game_step(G, fuzz_pad(&h1, &c1), fuzz_pad(&h2, &c2));
            fuzz_check(G);
        }
        // then every level with two players
        G->prog.joined[1] = 1;
        G->prog.hero[1] = HERO_OSTRICH;
        for (int k = 0; k < 5 && test_failures == before; k++) {
            G->prog.lives = 3;
            start_level(G, ids[k]);
            for (int f = 0; f < 60 * 90 && test_failures == before; f++) {
                if (G->scene == SC_MAP) break;
                ck_game_step(G, fuzz_pad(&h1, &c1), fuzz_pad(&h2, &c2));
                fuzz_check(G);
            }
        }
        if (test_failures != before) fprintf(stderr, "fuzz: seed %u failed\n", seed);
    }
    // a damaged save (right magic, random bytes) must load into something playable
    for (uint32_t seed = 1; seed <= 20; seed++) {
        char *w = slurp(DATA "world.txt");
        if (!w) return;
        static ck_game G;
        ck_prog bad;
        fz = seed * 40503u;
        uint8_t *b = (uint8_t *)&bad;
        for (size_t i = 0; i < sizeof bad; i++) b[i] = (uint8_t)fuzz_rand();
        bad.magic = CK_SAVE_MAGIC;
        EXPECT_EQ_INT(ck_game_init(&G, w, &bad), 1);
        free(w);
        EXPECT_EQ_INT(G.prog.at < G.world.nnodes, 1);
        EXPECT_EQ_INT(G.prog.col[0] < CK_NCOLS && G.prog.col[1] < CK_NCOLS, 1);
        int h1 = 0, h2 = 0;
        uint16_t c1 = 0, c2 = 0;
        for (int f = 0; f < 60 * 60; f++) { ck_game_step(&G, fuzz_pad(&h1, &c1), fuzz_pad(&h2, &c2)); fuzz_check(&G); }
    }
}

// The self-playing test ROM's tour passes, checkpoint by checkpoint, on a fresh save.
static void test_autotest(void) {
    ck_game *G = load_game();
    if (!G) return;
    ck_autotest A;
    ck_autotest_init(&A, G);
    int r = AT_NONE, checks = 0;
    for (int i = 0; i < 60 * 600 && r != AT_PASS && r != AT_FAIL; i++) {
        r = ck_autotest_step(&A, G);
        if (r == AT_CHECK) checks++;
    }
    if (r != AT_PASS) fprintf(stderr, "autotest: %s\n", A.msg);
    EXPECT_EQ_INT(r, AT_PASS);
    EXPECT_EQ_INT(checks, 14);
}

// Left alone, the story runs its songs in order, thunders during the storm and ends on the title.
static void test_intro(void) {
    ck_game *G = load_game();
    if (!G) return;
    int thunder = 0, whoosh = 0, storm_at = -1, title_at = -1, frames = 0;
    while (G->scene == SC_INTRO && frames < 60 * 20) {
        ck_game_step(G, 0, 0);
        frames++;
        for (int i = 0; i < G->nsfx; i++) { thunder += G->sfx[i] == SFX_THUNDER; whoosh += G->sfx[i] == SFX_WHOOSH; }
        if (G->song == SONG_STORM && storm_at < 0) storm_at = frames;
        if (G->song == SONG_TITLE && title_at < 0) title_at = frames;
    }
    EXPECT_EQ_INT(G->scene, SC_TITLE);
    EXPECT_EQ_INT(frames >= 911 && frames <= 913, 1);   // 15.2 s
    EXPECT_EQ_INT(storm_at >= 269 && storm_at <= 271, 1);
    EXPECT_EQ_INT(title_at >= 659 && title_at <= 661, 1);
    EXPECT_EQ_INT(whoosh, 1);
    EXPECT_EQ_INT(thunder > 0, 1);
}

static void test_flow(void) {
    ck_game *G = load_game();
    if (!G) return;
    // the story plays first; a button only skips it after 0.4 s
    EXPECT_EQ_INT(G->scene, SC_INTRO);
    EXPECT_EQ_INT(G->song, SONG_LULLABY);
    ck_game_step(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_INTRO);
    for (int i = 0; i < 30; i++) ck_game_step(G, 0, 0);
    ck_game_step(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_TITLE);
    EXPECT_EQ_INT(G->song, SONG_TITLE);
    ck_game_step(G, 0, 0);
    ck_game_step(G, CK_START, 0);
    EXPECT_EQ_INT(G->scene, SC_SLOTS);   // pick a save slot (slot 1 is the one already loaded)
    EXPECT_EQ_INT(G->slotPick, 0);
    ck_game_step(G, 0, 0);
    ck_game_step(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_SELECT);
    ck_game_step(G, 0, 0);
    ck_game_step(G, CK_A, 0);   // player 1 ready
    EXPECT_EQ_INT(G->scene, SC_MAP);
    ck_game_step(G, 0, 0);
    ck_game_step(G, CK_RIGHT, 0);   // walk from home to 1-1
    EXPECT_EQ_INT(G->map.walking, 1);
    steps(G, 120, 0);
    EXPECT_EQ_INT(G->map.walking, 0);
    EXPECT_EQ_INT(G->map.at, ck_world_node(&G->world, "l1"));
    {   // the first baby follows 14 px back along the path walked
        float bx, by;
        ck_map_baby_pos(G, 0, &bx, &by);
        float d = sqrtf((bx - G->map.hx) * (bx - G->map.hx) + (by - G->map.hy) * (by - G->map.hy));
        if (!(d > 8 && d < 14.5f)) printf("baby d=%f n=%d\n", d, G->map.ntrail);
        EXPECT_EQ_INT(d > 8 && d < 14.5f, 1);
    }
    ck_game_step(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_LEVEL);
    steps(G, 30, 0);
    ck_game_step(G, CK_START, 0);
    EXPECT_EQ_INT(G->scene, SC_PAUSE);
    ck_game_step(G, 0, 0);
    ck_game_step(G, CK_B, 0);
    EXPECT_EQ_INT(G->scene, SC_MAP);
}

static int emitted;
static void count_tone(const ck_tone *t, void *u) { (void)u; if (t->dur > 0 && t->vol > 0) emitted++; }

static void test_music(void) {
    {   // bad song files: a zero or negative bpm and a very long note fall back to safe values
        static ck_song bad[SONG_COUNT];
        char text[1400] = "song title bpm=0 loop\nsquare 0.1 C5\nsong map bpm=-5 loop\nsquare 0.1 C5";
        strcat(text, "\nsong level loop\nsquare 0.1 C5");
        for (int i = 0; i < 300; i++) strcat(text, "-");
        strcat(text, "\n");
        EXPECT_EQ_INT(ck_parse_songs(text, bad), 3);
        EXPECT_EQ_INT((int)bad[SONG_TITLE].bpm, 120);
        EXPECT_EQ_INT((int)bad[SONG_MAP].bpm, 120);
        EXPECT_EQ_INT(bad[SONG_LEVEL].tracks[0].len[0] > 0, 1);
        ck_music m;
        ck_music_start(&m, bad, SONG_LEVEL);
        for (int i = 0; i < 600; i++) ck_music_advance(&m, 1.0f / 60, count_tone, NULL);
    }
    static ck_song songs[SONG_COUNT];
    char *t = slurp("roms/clankers/data/songs.txt");
    if (!t) return;
    EXPECT_EQ_INT(ck_parse_songs(t, songs) >= 12, 1);
    free(t);
    const ck_song *s = &songs[SONG_LEVEL];
    EXPECT_EQ_INT(s->valid, 1);
    EXPECT_EQ_INT(s->loop, 1);
    EXPECT_EQ_INT((int)s->bpm, 140);
    EXPECT_EQ_INT(s->ntracks, 3);
    EXPECT_EQ_INT(s->tracks[2].drum, 1);
    EXPECT_EQ_INT((int)(s->tracks[0].freq[0] + 0.5f), 659);   // E5
    EXPECT_EQ_INT(s->tracks[0].len[8], 2);                    // G5- after the first bar
    ck_music m;
    ck_music_start(&m, songs, SONG_LEVEL);
    emitted = 0;
    for (int i = 0; i < 600; i++) ck_music_advance(&m, 1.0f / 60, count_tone, NULL);
    EXPECT_EQ_INT(emitted > 100, 1);   // ten seconds of three tracks
    EXPECT_EQ_INT(m.done, 0);
    ck_music_start(&m, songs, SONG_CLEAR);   // does not loop
    for (int i = 0; i < 600; i++) ck_music_advance(&m, 1.0f / 60, count_tone, NULL);
    EXPECT_EQ_INT(m.done, 1);
    ck_tone tones[8];
    EXPECT_EQ_INT(ck_sfx_tones(SFX_GEAR, tones, 8), 5);
    EXPECT_EQ_INT((int)tones[4].freq, 1319);
}

// ---------------------------------------------------------------------------- Worlds 2 to 4

static void test_new_worlds(void) {
    static const int themes[CK_WORLDS + 1] = { 0, THEME_JUNKYARD, THEME_FACTORY, THEME_MOUNTAIN, THEME_MOON };
    static const int bosses[CK_WORLDS + 1] = { 0, BOSS_CRANE, BOSS_STAMPER, BOSS_BOULDER, BOSS_HAULER };
    static const int songs[CK_WORLDS + 1] = { 0, SONG_MAP, SONG_MAP2, SONG_MAP3, SONG_MAP4 };
    ck_game *G = load_game();
    if (!G) return;
    for (int n = 1; n <= CK_WORLDS; n++) {
        if (n > 1) EXPECT_EQ_INT(ck_game_enter_world(G, n, CK_NODE_HOME), 1);
        EXPECT_EQ_INT(G->worldNum, n);
        if (n > 1) EXPECT_EQ_INT(G->scene, SC_MAP);
        EXPECT_EQ_INT(G->world.themeId, themes[n]);
        EXPECT_EQ_INT(G->world.song, songs[n]);
        EXPECT_EQ_INT(G->world.nnodes, 8);
        EXPECT_EQ_INT(G->world.nodes[G->prog.at].kind, CK_NODE_HOME);
        for (int i = 0; i < G->world.nnodes; i++) {
            const ck_node *N = &G->world.nodes[i];
            if (!N->level[0]) continue;
            EXPECT_EQ_INT(G->hasDef[i], 1);
            const ck_leveldef *L = &G->defs[i];
            if (N->kind == CK_NODE_BOSS) EXPECT_EQ_INT(L->bossType, bosses[n]);
            // every Moon level has low gravity, nowhere else does
            EXPECT_EQ_INT(L->grav < CK_GRAV * 0.9f, n == 4);
            // every level has a start, and a pad unless the boss or the shower ends it
            int start = 0, pad = 0, gears = 0;
            for (int y = 0; y < CK_ROWS; y++)
                for (int x = 0; x < L->w; x++) {
                    char c = L->rows[y][x];
                    start += c == '@'; pad += c == 'P'; gears += c == 'G' || c == '?';
                }
            EXPECT_EQ_INT(start, 1);
            if (L->kind == CK_KIND_LEVEL) { EXPECT_EQ_INT(pad, 1); EXPECT_EQ_INT(gears, 3); }
        }
    }
    // the home node is the house; its yard's signpost takes you back a world, to that world's exit
    G->prog.at = (uint8_t)ck_world_node(&G->world, "home");
    G->keepMap = false;
    ck_map_reset(G);
    ck_game_step(G, 0, 0);
    ck_game_step(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_HOUSE);
    ck_game_step(G, 0, 0);
    for (int i = 0; i < 400 && ck_house_near(G, &G->house.ps[0]) != SPOT_BACK; i++) ck_game_step(G, CK_LEFT, 0);
    for (int i = 0; i < 20; i++) ck_game_step(G, 0, 0);
    ck_game_step(G, CK_A, 0);
    EXPECT_EQ_INT(G->worldNum, 3);
    EXPECT_EQ_INT(G->world.nodes[G->prog.at].kind, CK_NODE_EXIT);
    // a world that does not exist is not entered
    EXPECT_EQ_INT(ck_game_enter_world(G, 5, CK_NODE_HOME), 0);
    EXPECT_EQ_INT(G->worldNum, 3);
}

// Each world keeps its own progress; the heroes and tries come along. Old saves become World 1.
static void test_saves(void) {
    ck_game *G = load_game();
    if (!G) return;
    G->prog.beaten = 0x1F; G->prog.gears = 7; G->prog.hero[0] = HERO_OSTRICH; G->prog.lives = 2;
    EXPECT_EQ_INT(ck_game_enter_world(G, 2, CK_NODE_HOME), 1);
    EXPECT_EQ_INT(G->prog.beaten, 0);
    EXPECT_EQ_INT(G->prog.gears, 0);
    EXPECT_EQ_INT(G->prog.hero[0], HERO_OSTRICH);
    EXPECT_EQ_INT(G->prog.lives, 2);
    G->prog.gears = 3;
    static ck_saves sv;
    sv = *ck_game_saves(G);
    EXPECT_EQ_INT(sv.magic, CK_SAVES_MAGIC);
    EXPECT_EQ_INT(sv.world, 2);
    EXPECT_EQ_INT(sv.w[0].gears, 7);
    EXPECT_EQ_INT(sv.w[1].gears, 3);
    EXPECT_EQ_INT(sv.w[2].magic, 0);
    // load it back: World 2, with both worlds' progress
    static ck_game H;
    EXPECT_EQ_INT(ck_game_boot(&H, load_data, NULL, &sv), 1);
    EXPECT_EQ_INT(H.worldNum, 2);
    EXPECT_EQ_INT(H.world.themeId, THEME_FACTORY);
    EXPECT_EQ_INT(H.prog.gears, 3);
    EXPECT_EQ_INT(ck_game_enter_world(&H, 1, CK_NODE_EXIT), 1);
    EXPECT_EQ_INT(H.prog.gears, 7);
    EXPECT_EQ_INT(H.prog.beaten, 0x1F);
    // a save from before Worlds 2 to 4: one ck_prog at the start of the EEPROM
    static uint8_t raw[512];
    memset(raw, 0xFF, sizeof raw);
    ck_prog old;
    ck_prog_fresh(&old);
    old.gears = 5; old.beaten = 0x3;
    memcpy(raw, &old, sizeof old);
    static ck_saves mig;
    ck_saves_read(&mig, raw);
    EXPECT_EQ_INT(mig.magic, CK_SAVES_MAGIC);
    EXPECT_EQ_INT(mig.world, 1);
    EXPECT_EQ_INT(mig.w[0].gears, 5);
    EXPECT_EQ_INT(mig.w[1].magic, 0);
    EXPECT_EQ_INT(ck_game_boot(&H, load_data, NULL, &mig), 1);
    EXPECT_EQ_INT(H.prog.gears, 5);
    // a blank or foreign EEPROM starts fresh
    memset(raw, 0xAB, sizeof raw);
    ck_saves_read(&mig, raw);
    EXPECT_EQ_INT(mig.world, 1);
    EXPECT_EQ_INT(mig.w[0].magic, 0);
}

// Conveyor belts carry you; the Moon's low gravity jumps higher.
static void test_belts_and_gravity(void) {
    ck_game *G = load_game();
    if (!G) return;
    // World 1: how high a standing jump goes
    float jump[2] = { 0, 0 };
    for (int n = 0; n < 2; n++) {
        if (n) EXPECT_EQ_INT(ck_game_enter_world(G, 4, CK_NODE_HOME), 1);
        start_level(G, "l1");
        for (int f = 0; f < 100; f++) ck_game_step(G, 0, 0);
        float y0 = G->lv.players[0].y, top = y0;
        ck_game_step(G, CK_A, 0);
        for (int f = 0; f < 120; f++) { ck_game_step(G, CK_A, 0); if (G->lv.players[0].y < top) top = G->lv.players[0].y; }
        jump[n] = y0 - top;
    }
    EXPECT_EQ_INT(jump[1] > jump[0] * 1.3f, 1);
    // World 2, 2-1: stand on the belt that runs right, and it carries you right
    EXPECT_EQ_INT(ck_game_enter_world(G, 2, CK_NODE_HOME), 1);
    start_level(G, "l1");
    ck_level *lv = &G->lv;
    int bx = -1;
    for (int x = 0; x < lv->w && bx < 0; x++) if (lv->tiles[12][x] == '>') bx = x;
    EXPECT_EQ_INT(bx > 0, 1);
    ck_player *p = &lv->players[0];
    p->x = bx * 16 + 2; p->y = 12 * 16 - p->h; p->vy = 0;
    for (int f = 0; f < 3; f++) ck_game_step(G, 0, 0);
    float x0 = p->x;
    for (int f = 0; f < 30; f++) ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(p->x > x0 + 15, 1);
    EXPECT_EQ_INT(p->ground, 1);
}

// Every boss can be beaten by bouncing on its three buttons, and every boss can bump you.
static void test_bosses(void) {
    ck_game *G = load_game();
    if (!G) return;
    for (int n = 1; n <= CK_WORLDS; n++) {
        if (n > 1) EXPECT_EQ_INT(ck_game_enter_world(G, n, CK_NODE_HOME), 1);
        // left alone, the boss gets you sooner or later
        G->prog.lives = 9;
        int node = start_level(G, "boss");
        int wobbles = 0;
        for (int f = 0; f < 60 * 40 && G->scene == SC_LEVEL; f++) {
            // pace to and fro (the Stamper follows you, so for it stand still, away from the shelves)
            if (n == 2 && f == 0) G->lv.players[0].x = 150;
            ck_game_step(G, n == 2 ? 0 : (f / 80) % 2 ? CK_LEFT : CK_RIGHT, 0);   // long enough legs to get up to speed
            if (G->lv.players[0].wob > 0 || G->lv.dyingT >= 0) wobbles++;   // no bolts to drop: a bump costs a try
        }
        if (!wobbles) fprintf(stderr, "boss %d never got the hero\n", n);
        EXPECT_EQ_INT(wobbles > 0, 1);
        // bounce on each button as it comes up
        G->prog.lives = 9;
        start_level(G, "boss");
        ck_level *lv = &G->lv;
        int f = 0;
        for (; f < 60 * 60 && G->scene == SC_LEVEL; f++) {
            ck_player *p = &lv->players[0];
            p->inv = 10;   // the boss is tested above; here only the buttons matter
            for (int i = 0; i < lv->nents; i++) {
                ck_ent *e = &lv->ents[i];
                if (e->dead || (e->type != E_BUTTON && e->type != E_PAD) || lv->clearT >= 0) continue;
                if (e->type == E_BUTTON && f % 20) continue;
                p->x = e->x + e->w / 2 - p->w / 2; p->y = e->y - p->h - 4; p->vy = 2; p->vx = 0;
                break;
            }
            ck_game_step(G, lv->clearT > 100 ? CK_A : 0, 0);
        }
        EXPECT_EQ_INT(G->scene, SC_MAP);
        EXPECT_EQ_INT(ck_node_beaten(G, node), 1);
        EXPECT_EQ_INT(G->lv.boss.fixed, 1);
        EXPECT_EQ_INT(G->lv.boss.type, n == 1 ? BOSS_CRANE : n == 2 ? BOSS_STAMPER : n == 3 ? BOSS_BOULDER : BOSS_HAULER);
    }
}

// The films: each one ends in the next world (the last at the title), and START skips.
static void test_films(void) {
    ck_game *G = load_game();
    if (!G) return;
    for (int k = 1; k <= CK_WORLDS; k++) {
        ck_game_film(G, k);
        EXPECT_EQ_INT(G->scene, SC_CUT);
        EXPECT_EQ_INT(G->song, k == CK_WORLDS ? SONG_PARADE : SONG_TRAVEL);
        ck_cut_view v;
        int frames = 0;
        while (G->scene == SC_CUT && frames < 60 * 30) {
            ck_game_step(G, 0, 0);
            frames++;
            if (G->scene != SC_CUT) break;
            ck_cut_cast(G, &v);
            EXPECT_EQ_INT(v.nbabies, k == CK_WORLDS ? 16 : 4);
            EXPECT_EQ_INT(v.caption != NULL, 1);
            EXPECT_EQ_INT(v.heroes[0].x == v.heroes[0].x && v.heroes[0].a >= 0 && v.heroes[0].a <= 1, 1);
        }
        EXPECT_EQ_INT((int)(frames / 60.0f + 0.5f), (int)(ck_cut_len[k] + 0.5f));
        if (k < CK_WORLDS) {
            EXPECT_EQ_INT(G->scene, SC_MAP);
            EXPECT_EQ_INT(G->worldNum, k + 1);
            EXPECT_EQ_INT(G->world.nodes[G->prog.at].kind, CK_NODE_HOME);
        } else EXPECT_EQ_INT(G->scene, SC_TITLE);
    }
    // the parade's babies are all sixteen, every world's own colours
    ck_game_film(G, CK_WORLDS);
    EXPECT_EQ_INT(G->cutBabies[0], 0xFF8FC7);
    EXPECT_EQ_INT(G->cutBabies[15], 0xFFD23F);
    // START skips, but not in the first second
    ck_game_step(G, CK_START, 0);
    EXPECT_EQ_INT(G->scene, SC_CUT);
    for (int f = 0; f < 70; f++) ck_game_step(G, 0, 0);
    ck_game_step(G, CK_START, 0);
    EXPECT_EQ_INT(G->scene, SC_TITLE);
    // walking onto World 1's exit and pressing A plays the first film
    G = load_game();
    G->scene = SC_MAP;
    G->prog.at = (uint8_t)ck_world_node(&G->world, "exit");
    ck_map_reset(G);
    ck_game_step(G, 0, 0);
    ck_game_step(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_CUT);
    EXPECT_EQ_INT(G->cut, 1);
}

static void test_new_bot_runs(void) {
    ck_game *G = load_game();
    if (!G) return;
    const char *ids[] = { "l1", "l2", "l3", "bonus" };
    for (int n = 2; n <= CK_WORLDS; n++) {
        EXPECT_EQ_INT(ck_game_enter_world(G, n, CK_NODE_HOME), 1);
        for (int k = 0; k < 4; k++) {
            G->prog.lives = 3;
            int node = start_level(G, ids[k]);
            int frames = 0;
            float far = 0;
            while (frames < 60 * 240 && G->scene != SC_MAP) {
                if (G->scene == SC_PAUSE) { ck_game_step(G, CK_START, 0); frames++; continue; }
                uint16_t pad = G->scene == SC_LEVEL && G->lv.nplayers ? ck_bot(&G->lv) : CK_RIGHT;
                ck_game_step(G, pad, 0);
                if (G->scene == SC_LEVEL && G->lv.players[0].x > far) far = G->lv.players[0].x;
                frames++;
            }
            printf("  bot on w%d %-5s: %s after %d frames, got to x=%d of %d\n", n, ids[k],
                   ck_node_beaten(G, node) ? "cleared" : G->scene == SC_MAP ? "out of tries" : "still going",
                   frames, (int)far, G->lv.w * 16);
            EXPECT_EQ_INT(G->nparts <= CK_MAXPARTS, 1);
            G->scene = SC_LEVEL;
        }
    }
}

// Movement: speed ramps up, both heroes double jump and glide, the ostrich's kick reaches like the
// robot's arm, and a bubble or a catch-up hop only ever sets a hero down on clear, firm floor.
static float jump_height(ck_game *G, int hero, bool twice) {
    G->prog.hero[0] = (uint8_t)hero;
    start_level(G, "l1");
    for (int f = 0; f < 100; f++) ck_game_step(G, 0, 0);
    ck_player *p = &G->lv.players[0];
    float y0 = p->y, top = y0;
    for (int f = 0; f < 150; f++) {
        uint16_t pad = f < 14 || (twice && f >= 24 && f < 40) ? CK_A : 0;   // let go, then a second press in the air
        ck_game_step(G, pad, 0);
        if (p->y < top) top = p->y;
    }
    return y0 - top;
}
static void test_movement(void) {
    ck_game *G = load_game();
    if (!G) return;
    // the run ramps up over half a second
    G->prog.hero[0] = HERO_ROBOT;
    start_level(G, "l1");
    for (int f = 0; f < 100; f++) ck_game_step(G, 0, 0);
    ck_player *p = &G->lv.players[0];
    for (int f = 0; f < 8; f++) ck_game_step(G, CK_RIGHT, 0);
    EXPECT_EQ_INT(p->vx > 0 && p->vx < 1.75f * 0.4f, 1);
    for (int f = 0; f < 40; f++) ck_game_step(G, CK_RIGHT, 0);
    EXPECT_EQ_INT(p->vx > 1.7f, 1);
    // both heroes: a second press in the air goes higher
    for (int h = 0; h < 2; h++) {
        float one = jump_height(G, h ? HERO_OSTRICH : HERO_ROBOT, false), two = jump_height(G, h ? HERO_OSTRICH : HERO_ROBOT, true);
        if (!(two > one + 12)) fprintf(stderr, "hero %d: jump %.1f, double %.1f\n", h, one, two);
        EXPECT_EQ_INT(two > one + 12, 1);
    }
    // a first jump still reaches as high as it did with the old gravity (0.3, v = -5.5): about 50 px
    float one = jump_height(G, HERO_ROBOT, false);
    EXPECT_EQ_INT(one > 44 && one < 56, 1);
    // both heroes glide while A is held on the way down
    for (int h = 0; h < 2; h++) {
        G->prog.hero[0] = (uint8_t)(h ? HERO_OSTRICH : HERO_ROBOT);
        start_level(G, "l1");
        for (int f = 0; f < 100; f++) ck_game_step(G, 0, 0);
        p = &G->lv.players[0];
        bool glided = false;
        for (int f = 0; f < 120; f++) { ck_game_step(G, CK_A, 0); if (p->flapping && p->vy < 1) glided = true; }
        EXPECT_EQ_INT(glided, 1);
    }
    // the ostrich kick reboots a Glitchy 29 px in front (the old kick stopped at 16), as the robot's arm does
    for (int h = 0; h < 2; h++) {
        G->prog.hero[0] = (uint8_t)(h ? HERO_OSTRICH : HERO_ROBOT);
        start_level(G, "l1");
        for (int f = 0; f < 100; f++) ck_game_step(G, 0, 0);
        ck_level *lv = &G->lv;
        p = &lv->players[0]; p->face = 1; p->inv = 0;
        ck_ent *e = NULL;
        for (int k = 0; k < lv->nents; k++) if (lv->ents[k].type == E_ENEMY && !lv->ents[k].dead) { e = &lv->ents[k]; break; }
        EXPECT_EQ_INT(e != NULL, 1);
        if (!e) return;
        e->x = p->x + p->w + 29 - e->w; e->y = p->y + p->h - e->h; e->vx = 0; e->kind = EN_SPROCKET; e->state = ES_GLITCHY; e->hopT = 999;
        ck_game_step(G, CK_B, 0);
        for (int f = 0; f < 20; f++) { e->x = p->x + p->w + 29 - e->w; ck_game_step(G, 0, 0); }
        EXPECT_EQ_INT(e->state == ES_HAPPY || e->dead, 1);
    }
    // setting a hero down beside a buddy: only on firm floor, never in a wall or over a pit
    G->prog.hero[0] = HERO_ROBOT; G->prog.joined[1] = 1; G->prog.hero[1] = HERO_OSTRICH;
    start_level(G, "l1");
    for (int f = 0; f < 100; f++) ck_game_step(G, 0, 0);
    ck_level *lv = &G->lv;
    EXPECT_EQ_INT(lv->nplayers, 2);
    ck_player *a = &lv->players[0], *b = &lv->players[1];
    EXPECT_EQ_INT(a->ground, 1);
    b->x = a->x + 60; b->y = a->y - 80;
    EXPECT_EQ_INT(ck_set_down(lv, b, a), 1);
    EXPECT_EQ_INT(ck_absf(b->y + b->h - (a->y + a->h)) < 0.01f, 1);
    a->ground = false;   // in the air (over a pit, maybe): nowhere to put anyone
    EXPECT_EQ_INT(ck_set_down(lv, b, a), 0);
    a->ground = true;
    // wall in every spot beside and at the buddy: stay in the bubble
    int ty = (int)((a->y + a->h) / 16) - 1;
    char keep[3];
    for (int k = -1; k <= 1; k++) { int tx = (int)((a->x + a->w / 2) / 16) + k; keep[k + 1] = lv->tiles[ty][tx]; lv->tiles[ty][tx] = '#'; }
    EXPECT_EQ_INT(ck_set_down(lv, b, a), 0);
    for (int k = -1; k <= 1; k++) lv->tiles[ty][(int)((a->x + a->w / 2) / 16) + k] = keep[k + 1];
    // bubble: a hero who lost a life floats to the buddy and lands standing on the floor, not in the air
    G->prog.lives = 3;
    b->y = 240 + 40; ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(b->bubble > 0, 1);
    for (int f = 0; f < 400 && b->bubble; f++) ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(b->bubble, 0);
    for (int f = 0; f < 3; f++) ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(b->ground, 1);
    // out of lives with a buddy still going: stay in the bubble and the level carries on
    G->prog.lives = 1;
    for (int f = 0; f < 120; f++) ck_game_step(G, 0, 0);   // let the landing's safe time run out
    b->y = 240 + 40; ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(G->prog.lives, 0);
    EXPECT_EQ_INT(b->bubble > 0, 1);
    for (int f = 0; f < 400; f++) ck_game_step(G, f % 2 ? CK_A : 0, f % 2 ? CK_A : 0);
    EXPECT_EQ_INT(b->bubble > 0, 1);
    EXPECT_EQ_INT(lv->dyingT, -1);
    // and when the buddy is out too, it is game over
    a->y = 240 + 40;
    for (int f = 0; f < 300 && G->scene == SC_LEVEL; f++) ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(G->scene, SC_MAP);
    EXPECT_EQ_INT(G->prog.lives, 3);   // game over refills the tries
    G->prog.joined[1] = 0;
}

// Every looping song but the two short intro moods and the parade film runs 16 bars before it repeats.
static void test_song_lengths(void) {
    static ck_song songs[SONG_COUNT];
    char *t = slurp("roms/clankers/data/songs.txt");
    if (!t) return;
    ck_parse_songs(t, songs);
    free(t);
    for (int id = 1; id < SONG_COUNT; id++) {
        const ck_song *s = &songs[id];
        if (!s->valid || !s->loop || id == SONG_LULLABY || id == SONG_STORM || id == SONG_PARADE) continue;   // the parade film ends before it loops
        for (int k = 0; k < s->ntracks; k++) {
            int eighths = 0;
            for (int n = 0; n < s->tracks[k].n; n++) eighths += s->tracks[k].len[n];
            if (eighths != 128) fprintf(stderr, "song %d track %d: %d eighths\n", id, k, eighths);
            EXPECT_EQ_INT(eighths, 128);
        }
    }
}

// The parts shop: buy with bolts, one item per slot, items in different slots stack, both players.
static void press(ck_game *G, uint16_t p1, uint16_t p2) { ck_game_step(G, p1, p2); ck_game_step(G, 0, 0); }
static void test_shop(void) {
    ck_game *G = load_game();
    if (!G) return;
    EXPECT_EQ_INT(ACC_COUNT, 36);
    for (int a = 0; a < ACC_COUNT; a++) EXPECT_EQ_INT(!!(ck_acc[a].cost > 0 && ck_acc[a].slot < SLOT_COUNT && ck_acc[a].name[0]), 1);
    G->prog.joined[0] = G->prog.joined[1] = true;
    G->prog.bolts = 100;
    G->scene = SC_SHOP;
    G->shopCur[0] = ACC_PARTY; G->shopCur[1] = ACC_BOWTIE;
    press(G, CK_A, 0);   // P1 buys the party hat and puts it on
    EXPECT_EQ_INT(!!(CK_OWNS(&G->saves.closet, ACC_PARTY)), 1);
    EXPECT_EQ_INT(G->prog.bolts, 100 - ck_acc[ACC_PARTY].cost);
    EXPECT_EQ_INT(ck_wearing(G, 0, SLOT_HEAD), ACC_PARTY);
    press(G, 0, CK_A);   // P2 buys a bow tie
    EXPECT_EQ_INT(ck_wearing(G, 1, SLOT_NECK), ACC_BOWTIE);
    EXPECT_EQ_INT(ck_wearing(G, 1, SLOT_HEAD), -1);
    // P1 walks to the bow tie (already owned): wearing it is free and stacks with the hat
    G->shopCur[0] = ACC_BOWTIE;
    int bolts = G->prog.bolts;
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->prog.bolts, bolts);
    EXPECT_EQ_INT(ck_wearing(G, 0, SLOT_HEAD), ACC_PARTY);
    EXPECT_EQ_INT(ck_wearing(G, 0, SLOT_NECK), ACC_BOWTIE);
    press(G, CK_A, 0);   // and A again takes it off
    EXPECT_EQ_INT(ck_wearing(G, 0, SLOT_NECK), -1);
    // too few bolts: nothing changes
    G->shopCur[0] = ACC_DISCO;
    press(G, CK_A, 0);
    EXPECT_EQ_INT(!!(!CK_OWNS(&G->saves.closet, ACC_DISCO)), 1);
    EXPECT_EQ_INT(!!(G->shopMsgT > 0), 1);
    // the grid is 12 wide: down moves a row, right steps one
    G->shopCur[0] = 0;
    press(G, CK_DOWN, 0); EXPECT_EQ_INT(G->shopCur[0], 12);
    press(G, CK_RIGHT, 0); EXPECT_EQ_INT(G->shopCur[0], 13);
    // the last row: heroes swap for free, paint pots are bought then worn, the bubble gun is bought once
    EXPECT_EQ_INT(SHOP_COUNT, 49);
    G->prog.hero[1] = HERO_ROBOT;
    G->shopCur[1] = SHOP_OSTRICH; bolts = G->prog.bolts;
    press(G, 0, CK_A);
    EXPECT_EQ_INT(G->prog.hero[1], HERO_OSTRICH);
    EXPECT_EQ_INT(G->prog.bolts, bolts);
    EXPECT_EQ_INT(ck_shop_owns(G, SHOP_PAINT0 + 0), 1);   // the first two colours are free
    EXPECT_EQ_INT(ck_shop_owns(G, SHOP_PAINT0 + 4), 0);
    G->shopCur[1] = SHOP_PAINT0 + 4;
    press(G, 0, CK_A);
    EXPECT_EQ_INT(ck_shop_owns(G, SHOP_PAINT0 + 4), 1);
    EXPECT_EQ_INT(G->prog.col[1], 4);
    EXPECT_EQ_INT(G->prog.bolts, bolts - CK_PAINT_COST);
    // the rainbow paint: bought once, worn per player, and a plain paint takes it off again
    G->prog.bolts = 500;
    G->shopCur[1] = SHOP_RAINBOW;
    press(G, 0, CK_A);
    EXPECT_EQ_INT(G->prog.bolts, 500 - CK_RAINBOW_COST);
    EXPECT_EQ_INT(ck_rainbow_skin(G, 1), 1);
    EXPECT_EQ_INT(ck_rainbow_skin(G, 0), 0);
    EXPECT_EQ_INT(G->prog.col[1], CK_RAINBOW_COL);
    G->shopCur[1] = SHOP_PAINT0 + 4;
    press(G, 0, CK_A);
    EXPECT_EQ_INT(ck_rainbow_skin(G, 1), 0);
    EXPECT_EQ_INT(G->prog.col[1], 4);
    EXPECT_EQ_INT(ck_shop_owns(G, SHOP_RAINBOW), 1);
    // a high item past bit 31 still saves and stacks
    G->prog.bolts = 500;
    G->shopCur[0] = ACC_LIGHTUP; press(G, CK_A, 0);
    G->shopCur[0] = ACC_DISCO; press(G, CK_A, 0);
    EXPECT_EQ_INT(ck_wearing(G, 0, SLOT_FEET), ACC_LIGHTUP);
    EXPECT_EQ_INT(ck_wearing(G, 0, SLOT_BODY), ACC_DISCO);
    EXPECT_EQ_INT(ck_wearing(G, 0, SLOT_HEAD), ACC_PARTY);
    G->prog.bolts = 500;
    G->shopCur[0] = SHOP_GUN;
    press(G, CK_A, 0);
    EXPECT_EQ_INT(ck_shop_owns(G, SHOP_GUN), 1);
    EXPECT_EQ_INT(ck_gun_on(G, 0), 1);
    // B leaves to the map; a new game empties the closet
    press(G, CK_B, 0);
    EXPECT_EQ_INT(G->scene, SC_MAP);
    ck_game_new_game(G);
    EXPECT_EQ_INT(!!(!CK_OWNS(&G->saves.closet, ACC_PARTY) && !CK_OWNS(&G->saves.closet, ACC_LIGHTUP)), 1);
    EXPECT_EQ_INT(ck_wearing(G, 0, SLOT_HEAD), -1);
    EXPECT_EQ_INT(ck_wearing(G, 0, SLOT_FEET), -1);
    EXPECT_EQ_INT(ck_shop_owns(G, SHOP_GUN), 0);
    EXPECT_EQ_INT(ck_shop_owns(G, SHOP_PAINT0 + 4), 0);
    EXPECT_EQ_INT(ck_shop_owns(G, SHOP_PAINT0 + 1), 1);
    EXPECT_EQ_INT(ck_shop_owns(G, SHOP_RAINBOW), 0);
}

// The bubble gun: once bought, the trigger (Z or L) fires bubbles; B stays the kick or grab.
static void test_bubble_gun(void) {
    ck_game *G = load_game();
    if (!G) return;
    G->prog.joined[0] = true; G->prog.joined[1] = false; G->prog.hero[0] = HERO_ROBOT;
    G->scene = SC_LEVEL;
    ck_level_start(G, ck_world_node(&G->world, "l1"), false, false);
    steps(G, 60, 0);
    int shots = 0;
    press(G, CK_L, 0);
    for (int k = 0; k < G->lv.nents; k++) if (G->lv.ents[k].type == E_SHOT && !G->lv.ents[k].dead) shots++;
    EXPECT_EQ_INT(shots, 0);   // not bought yet
    EXPECT_EQ_INT(ck_gun_on(G, 0), 0);
    G->saves.closet.gear |= CK_GEAR_GUN;
    EXPECT_EQ_INT(ck_gun_on(G, 0), 1);
    steps(G, 30, 0);
    ck_game_step(G, CK_B, 0);   // B still kicks, no bubble
    for (int k = 0; k < G->lv.nents; k++) if (G->lv.ents[k].type == E_SHOT && !G->lv.ents[k].dead) shots++;
    EXPECT_EQ_INT(shots, 0);
    steps(G, 30, 0);
    ck_game_step(G, CK_L, 0);
    for (int k = 0; k < G->lv.nents; k++) if (G->lv.ents[k].type == E_SHOT && !G->lv.ents[k].dead) shots++;
    EXPECT_EQ_INT(shots, 1);
    steps(G, 30, 0);
    // the super jetpack: holding A in the air sinks almost level
    G->saves.closet.gear |= CK_GEAR_JET;
    steps(G, 30, 0);
    ck_game_step(G, CK_A, 0);
    int f = 0;
    while (G->lv.players[0].vy <= 0 && f++ < 120) ck_game_step(G, CK_A, 0);
    steps(G, 10, CK_A);
    EXPECT_EQ_INT(G->lv.players[0].ground, 0);
    EXPECT_EQ_INT(G->lv.players[0].vy <= CK_SUPERGLIDE_VY + CK_GRAV + 0.001f, 1);   // capped, then one frame of gravity
}

// The map menu lets player 2 leave; the row only shows while two are playing.
static void test_drop_player2(void) {
    ck_game *G = load_game();
    if (!G) return;
    G->prog.joined[0] = G->prog.joined[1] = true;
    G->scene = SC_MAP;
    int items[4];
    EXPECT_EQ_INT(ck_menu_items(G, items), 4);
    EXPECT_EQ_INT(items[2], MENU_DROP);
    press(G, CK_START, 0);
    EXPECT_EQ_INT(G->scene, SC_MENU);
    press(G, CK_DOWN, 0); press(G, CK_DOWN, 0);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->prog.joined[1], 0);
    EXPECT_EQ_INT(G->scene, SC_MAP);
    EXPECT_EQ_INT(ck_menu_items(G, items), 3);
    EXPECT_EQ_INT(items[2], MENU_NEW);
}


// The house: walk along the rooms, use the things in them, and the little games.
static void hold(ck_game *G, int n, uint16_t p1, uint16_t p2) { for (int i = 0; i < n; i++) ck_game_step(G, p1, p2); }
static void walk_to(ck_game *G, float x) {
    for (int i = 0; i < 2000 && fabsf(G->house.ps[0].x - x) > 4; i++) ck_game_step(G, G->house.ps[0].x < x ? CK_RIGHT : CK_LEFT, 0);
    hold(G, 20, 0, 0);
}
static void test_house(void) {
    ck_game *G = load_game();
    if (!G) return;
    G->prog.joined[0] = G->prog.joined[1] = true;
    G->prog.beaten = 0x7;   // home, l1, l2 (two levels): two babies at home
    G->scene = SC_MAP;
    ck_house_enter(G, false);
    EXPECT_EQ_INT(G->scene, SC_HOUSE);
    EXPECT_EQ_INT(G->house.np, 2);
    EXPECT_EQ_INT(G->house.nbabies, 2);
    // the labels follow whoever pressed something last
    press(G, 0, CK_C); EXPECT_EQ_INT(G->house.ps[G->house.last].slot, 1);
    press(G, CK_C, 0); EXPECT_EQ_INT(G->house.ps[G->house.last].slot, 0);
    EXPECT_EQ_INT(G->house.babies[0].hex, G->world.babies[0]);
    // the layout: rooms side by side, every spot inside a room
    for (int r = 1; r < CK_HOUSE_ROOMS; r++) EXPECT_EQ_INT(ck_rooms[r].x0, ck_rooms[r - 1].x1);
    EXPECT_EQ_INT(ck_rooms[CK_HOUSE_ROOMS - 1].x1, CK_HOUSE_W);
    // the fridge: pick a food, hold it, A eats it and fills the batteries
    G->prog.lives = 1;
    walk_to(G, ck_spots[SPOT_SNACK].x);
    EXPECT_EQ_INT(ck_house_near(G, &G->house.ps[0]), SPOT_SNACK);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->house.mode, HM_FRIDGE);
    press(G, CK_RIGHT, 0); press(G, CK_DOWN, 0);
    EXPECT_EQ_INT(G->house.fpick[0], 4);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->house.mode, HM_WALK);
    EXPECT_EQ_INT(G->house.ps[0].hold, FOOD_CAKE + 1);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->prog.lives, 3);
    EXPECT_EQ_INT(G->house.ps[0].eat > 0 && G->house.ps[0].hold == 0, 1);
    hold(G, 80, 0, 0);
    // B throws what you hold; it lands on the floor and goes
    press(G, CK_A, 0); G->house.fpick[0] = 0; press(G, CK_A, 0);
    EXPECT_EQ_INT(G->house.ps[0].hold, FOOD_APPLE + 1);
    float x0 = G->house.ps[0].x;
    press(G, CK_B, 0);
    EXPECT_EQ_INT(G->house.ps[0].hold, 0);
    EXPECT_EQ_INT(G->house.nthrown, 1);
    hold(G, 40, 0, 0);
    EXPECT_EQ_INT(G->house.nthrown == 0 || fabsf(G->house.thrown[0].x - x0) > 30, 1);
    hold(G, 200, 0, 0);
    EXPECT_EQ_INT(G->house.nthrown, 0);
    // the stove: egg and cheese, stirred till done, make a cheesy omelette to hold
    walk_to(G, ck_spots[SPOT_STOVE].x);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->house.mode, HM_COOK);
    G->house.cpick[1] = 0;   // keep player 2 out of the way
    press(G, CK_A, 0);                             // egg
    press(G, CK_RIGHT, 0); press(G, CK_RIGHT, 0);
    press(G, CK_A, 0);                             // cheese
    EXPECT_EQ_INT(G->house.npan, 2);
    press(G, CK_A, 0); press(G, CK_A, 0); press(G, CK_A, 0);   // a full pan of four swaps its last one
    EXPECT_EQ_INT(G->house.npan, 4);
    EXPECT_EQ_INT(G->house.pan[3], FOOD_CHEESE);
    press(G, CK_RIGHT, 0); press(G, CK_A, 0);
    EXPECT_EQ_INT(G->house.pan[3], FOOD_MUSHROOM);
    EXPECT_EQ_INT(G->house.pan[2], FOOD_CHEESE);
    G->house.npan = 2;   // back to egg and cheese
    for (int i = 0; i < 12 && !G->house.dish; i++) press(G, CK_B, 0);
    EXPECT_EQ_INT(G->house.dish, FOOD_DISH0 + DISH_CHEESY_OMELETTE + 1);
    EXPECT_EQ_INT(strcmp(G->house.dishName, "Cheesy omelette"), 0);
    EXPECT_EQ_INT(G->house.dishExtra, 0);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->house.mode, HM_WALK);
    EXPECT_EQ_INT(G->house.ps[0].hold, FOOD_DISH0 + DISH_CHEESY_OMELETTE + 1);
    EXPECT_EQ_INT(G->house.np, 2);
    EXPECT_EQ_INT(G->house.ps[1].hold, FOOD_DISH0 + DISH_CHEESY_OMELETTE + 1);   // everyone playing gets one
    {   // more recipes: the first rule that fits names the dish
        static const struct { int pan[4], n, dish; const char *name; } R[] = {
            { { FOOD_SAUSAGE, FOOD_BREAD, FOOD_TOMATO }, 3, DISH_KETCHUP_HOTDOG, "Hot dog and ketchup" },
            { { FOOD_PASTA, FOOD_CHEESE }, 2, DISH_MAC_CHEESE, "Mac and cheese" },
            { { FOOD_PASTA, FOOD_TOMATO, FOOD_SAUSAGE }, 3, DISH_MEATBALLS, "Spaghetti and meatballs" },
            { { FOOD_RICE, FOOD_EGG }, 2, DISH_FRIED_RICE, "Egg fried rice" },
            { { FOOD_APPLE, FOOD_BANANA }, 2, DISH_FRUIT_PIE, "Fruit pie" },
            { { FOOD_BANANA, FOOD_EGG }, 2, DISH_PANCAKES, "Banana pancakes" },
            { { FOOD_POTATO, FOOD_CHEESE }, 2, DISH_CHEESY_CHIPS, "Cheesy chips" },
            { { FOOD_POTATO, FOOD_CARROT }, 2, DISH_VEGGIE_STEW, "Veggie stew" },
            { { FOOD_EGG, FOOD_SAUSAGE }, 2, DISH_BIG_BREAKFAST, "Big breakfast" },
            { { FOOD_BANANA, FOOD_BREAD }, 2, DISH_BANANA_BREAD, "Banana bread" },
            { { FOOD_CARROT, FOOD_EGG }, 2, DISH_CARROT_CAKE, "Carrot cake" },
            { { FOOD_CHEESE }, 1, DISH_CHEESE_PLATE, "Melted cheese" },
        };
        for (size_t i = 0; i < sizeof R / sizeof R[0]; i++) {
            int dish, extra; char name[40];
            ck_cook_dish(R[i].pan, R[i].n, &dish, &extra, name, sizeof name);
            EXPECT_EQ_INT(extra, 0);
            EXPECT_EQ_INT(dish, FOOD_DISH0 + R[i].dish + 1);
            EXPECT_EQ_INT(strcmp(name, R[i].name), 0);
        }
        // every dish can come out of the pan: try every pan of up to three ingredients
        bool seen[CK_DISHES] = { 0 };
        for (int a = 0; a < CK_COOK_INGR; a++) for (int b = a; b < CK_COOK_INGR; b++) for (int c = b; c < CK_COOK_INGR; c++)
            for (int n = 1; n <= 3; n++) {
                int pan[3] = { ck_cook_ingr[a], ck_cook_ingr[b], ck_cook_ingr[c] }, dish, extra; char name[40];
                ck_cook_dish(pan, n, &dish, &extra, name, sizeof name);
                seen[dish - 1 - FOOD_DISH0] = true;
            }
        for (int k = 0; k < CK_DISHES; k++) { if (!seen[k]) printf("  dish %d never cooks\n", k); EXPECT_EQ_INT(seen[k], 1); }
        // the rest of the pan goes into the name and on top as garnish, so most pans read differently
        {
            int pan[4] = { FOOD_EGG, FOOD_CHEESE, FOOD_SAUSAGE, FOOD_MUSHROOM }, dish, extra; char name[40];
            ck_cook_dish(pan, 4, &dish, &extra, name, sizeof name);
            EXPECT_EQ_INT(dish, FOOD_DISH0 + DISH_CHEESY_OMELETTE + 1);
            EXPECT_EQ_INT(strcmp(name, "Foresty cheesy omelette with sausage"), 0);
            EXPECT_EQ_INT(extra, (1 << 3) | (1 << 8));   // mushroom and sausage on top
            int four[4] = { FOOD_CHEESE, FOOD_CHEESE, FOOD_CHEESE, FOOD_CHEESE };
            ck_cook_dish(four, 4, &dish, &extra, name, sizeof name);
            EXPECT_EQ_INT(strcmp(name, "Mega cheese soup"), 0);
        }
        static char names[2000][40];
        int nn = 0, pans = 0;
        for (int a = 0; a < CK_COOK_INGR; a++) for (int b = a; b < CK_COOK_INGR; b++) for (int c = b; c < CK_COOK_INGR; c++) for (int e = c; e < CK_COOK_INGR; e++) {
            int pan[4] = { ck_cook_ingr[a], ck_cook_ingr[b], ck_cook_ingr[c], ck_cook_ingr[e] }, dish, extra; char name[40];
            ck_cook_dish(pan, 4, &dish, &extra, name, sizeof name);
            pans++;
            EXPECT_EQ_INT(strlen(name) <= 38, 1);
            bool dup = false;
            for (int i = 0; i < nn && !dup; i++) dup = !strcmp(names[i], name);
            if (!dup && nn < 2000) strcpy(names[nn++], name);
        }
        printf("  kitchen: %d full pans make %d different dishes\n", pans, nn);
        EXPECT_EQ_INT(nn * 10 >= pans * 6, 1);
    }
    press(G, CK_A, 0);   // eat it
    hold(G, 60, 0, 0);
    // the paint studio: fly up with the stick, hold A to paint, R changes colour
    walk_to(G, ck_spots[SPOT_PAINT].x);
    press(G, CK_DOWN, 0);   // down no longer opens a game; A does
    EXPECT_EQ_INT(G->scene, SC_HOUSE);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_PAINT);
    int painted = 0;
    hold(G, 30, CK_UP | CK_A | CK_RIGHT, 0);
    for (int i = 0; i < CK_MURAL_W * CK_MURAL_H; i++) if (G->house.mural[i] == 1) painted++;
    EXPECT_EQ_INT(painted > 20, 1);
    EXPECT_EQ_INT(G->house.gs[0].y < CK_MURAL_FLOOR - 20, 1);
    press(G, CK_R, 0);
    EXPECT_EQ_INT(G->house.gs[0].col, 1);
    press(G, CK_L, 0); press(G, CK_L, 0);
    EXPECT_EQ_INT(G->house.gs[0].col, CK_MURAL_CLEAR);
    press(G, CK_L, 0);
    EXPECT_EQ_INT(G->house.gs[0].col, CK_MURAL_ERASER);
    hold(G, 10, CK_A, 0);
    // turning round barely moves the brush
    {
        const ck_hhero *p = &G->house.gs[0];
        float b0 = p->x + p->face * 9;
        hold(G, 2, CK_LEFT, 0);
        EXPECT_EQ_INT(p->face, -1);
        EXPECT_EQ_INT(fabsf(p->x + p->face * 9 - b0) < 6, 1);
    }
    press(G, CK_R, 0);
    EXPECT_EQ_INT(G->house.gs[0].col, CK_MURAL_CLEAR);
    press(G, CK_A, 0);   // wipe the wall
    int left = 0;
    for (int i = 0; i < CK_MURAL_W * CK_MURAL_H; i++) left += G->house.mural[i] != 0;
    EXPECT_EQ_INT(left, 0);
    // the brush starts at one cell; C makes it two, then three cells across
    press(G, CK_R, 0);
    EXPECT_EQ_INT(G->house.gs[0].col, 0);
    hold(G, 40, 0, 0);
    static const int sizes[3] = { 1, 4, 9 };
    for (int sz = 0; sz < 3; sz++) {
        EXPECT_EQ_INT(G->house.gs[0].size, sz);
        ck_game_step(G, CK_A, 0); ck_game_step(G, 0, 0);
        left = 0;
        for (int i = 0; i < CK_MURAL_W * CK_MURAL_H; i++) left += G->house.mural[i] != 0;
        EXPECT_EQ_INT(left, sizes[sz]);
        press(G, CK_C, 0);
    }
    EXPECT_EQ_INT(G->house.gs[0].size, 0);
    // B swaps the brush for a sticker; A stamps it in shades of your colour with an ink outline
    press(G, CK_B, 0);
    EXPECT_EQ_INT(G->house.gs[0].stick, 1);
    press(G, CK_A, 0);
    {
        int n[31] = { 0 };
        for (int i = 0; i < CK_MURAL_W * CK_MURAL_H; i++) n[G->house.mural[i]]++;
        EXPECT_EQ_INT(n[1] > 10 && n[1 + CK_MURAL_PAL] > 0 && n[1 + 2 * CK_MURAL_PAL] > 0 && n[10] > 10, 1);
        EXPECT_EQ_INT(ck_mural_hex(1 + CK_MURAL_PAL) != ck_mural_pal[0] && ck_mural_hex(1 + 2 * CK_MURAL_PAL) != ck_mural_pal[0], 1);
    }
    press(G, CK_C, 0);
    EXPECT_EQ_INT(G->house.gs[0].kind, 1);
    EXPECT_EQ_INT(G->house.gs[0].size, 0);   // C picks the sticker, not the brush size
    for (int k = 0; k < CK_STICKERS; k++) {   // every sticker has some of its colour and an outline
        int g[CK_STICKER_N][CK_STICKER_N], main = 0, ink = 0;
        ck_sticker_cells(k, 3, g);
        for (int y = 0; y < CK_STICKER_N; y++) for (int x = 0; x < CK_STICKER_N; x++) { main += g[y][x] == 4; ink += g[y][x] == 10; }
        EXPECT_EQ_INT(main > 4 && ink > 8, 1);
        EXPECT_EQ_INT(g[0][0], -1);
    }
    press(G, CK_B, 0);
    EXPECT_EQ_INT(G->house.gs[0].stick, 0);
    press(G, CK_START, 0);
    EXPECT_EQ_INT(G->scene, SC_HOUSE);
    EXPECT_EQ_INT(painted > 0, 1);   // the wall keeps its paint
    // the aquarium: swim down and right; the beach is dry land
    walk_to(G, ck_spots[SPOT_AQUA].x);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_AQUA);
    float y0 = G->house.gs[0].y;
    hold(G, 40, CK_DOWN, 0);
    EXPECT_EQ_INT(G->house.gs[0].y > y0 + 10, 1);
    // feeding: B sprinkles food, and a fish swims over and eats it
    hold(G, 60, 0, 0);
    int fed = 0;
    for (int n = 0; n < 6 && !fed; n++) {
        press(G, CK_B, 0);
        for (int i = 0; i < 300 && !fed; i++) { ck_game_step(G, 0, 0); for (int k = 0; k < CK_AQUA_FISH; k++) fed += G->house.fish[k].happy > 0; }
    }
    EXPECT_EQ_INT(fed > 0, 1);
    // a treasure chest opens with B, and has bolts the first time
    int bolts0 = G->prog.bolts;
    G->house.gs[0].x = ck_aqua_chest_x[0]; G->house.gs[0].y = CK_AQUA_SEABED - 10;
    press(G, CK_B, 0);
    EXPECT_EQ_INT(G->house.chestT[0] > 0, 1);
    EXPECT_EQ_INT(G->prog.bolts, bolts0 + 5);
    hold(G, 300, 0, 0);
    press(G, CK_B, 0);
    EXPECT_EQ_INT(G->prog.bolts, bolts0 + 5);
    // a shrimp: B picks it up, it rides in your hand, and B throws it the way you face
    G->house.gs[0].x = 280; G->house.gs[0].y = CK_AQUA_SEABED; G->house.gs[0].face = 1;
    for (int k = 1; k < CK_AQUA_SHRIMP; k++) G->house.shrimp[k].x = 600;
    G->house.shrimp[0].x = 284; G->house.shrimp[0].y = CK_AQUA_SEABED - 2; G->house.shrimp[0].spin = 0;
    press(G, CK_B, 0);
    EXPECT_EQ_INT(G->house.shrimp[0].held, 1);
    hold(G, 30, CK_UP, 0);
    EXPECT_EQ_INT(G->house.shrimp[0].y < CK_AQUA_SEABED - 20, 1);
    float sx = G->house.shrimp[0].x;
    press(G, CK_B, 0);
    EXPECT_EQ_INT(G->house.shrimp[0].held, 0);
    EXPECT_EQ_INT(G->house.shrimp[0].spin > 0, 1);
    hold(G, 20, 0, 0);
    EXPECT_EQ_INT(G->house.shrimp[0].x > sx + 20, 1);
    // five sections of water, each with its own kinds of fish living in it
    {
        int kinds = 0; bool seen[AFISH_KINDS] = { 0 };
        for (int k = 0; k < CK_AQUA_FISH; k++) {
            const ck_fish *f = &G->house.fish[k];
            if (!seen[f->kind]) { seen[f->kind] = true; kinds++; }
            EXPECT_EQ_INT(f->home, k / CK_AQUA_PER_SEC);
        }
        EXPECT_EQ_INT(kinds, AFISH_KINDS);
    }
    // the tank scrolls: the beach is six screens along
    hold(G, 1500, CK_RIGHT | CK_UP, 0);
    hold(G, 100, CK_RIGHT, 0);
    EXPECT_EQ_INT(G->house.gcam > 1500, 1);
    EXPECT_EQ_INT(G->house.gs[0].land, 1);
    EXPECT_EQ_INT(G->house.gs[0].y, CK_AQUA_BEACH_Y);
    // poke a crab: it hops and runs
    G->house.crabs[0].x = G->house.gs[0].x + 10; G->house.crabs[0].wait = 0;
    press(G, CK_B, 0);
    int poked = 0;
    for (int k = 0; k < CK_AQUA_CRABS; k++) poked += G->house.crabs[k].y < 0 && G->house.crabs[k].run > 0;
    EXPECT_EQ_INT(poked, 1);
    for (int i = 0; i < 600; i++) {
        ck_game_step(G, 0, 0);
        for (int k = 0; k < CK_AQUA_FISH; k++) EXPECT_EQ_INT(G->house.fish[k].x >= 12 && G->house.fish[k].x <= CK_AQUA_BEACH_X - 16, 1);
    }
    for (int k = 0; k < CK_AQUA_FISH; k++) {   // left alone, the fish keep to their own sections
        const ck_fish *f = &G->house.fish[k];
        if (f->follow > 0) continue;
        EXPECT_EQ_INT(f->x > f->home * CK_AQUA_SEC_W - 10 && f->x < (f->home + 1) * CK_AQUA_SEC_W + 10, 1);
    }
    press(G, CK_START, 0);
    // the garage: a side-on race; a scripted driver jumps every cone and crate
    walk_to(G, ck_spots[SPOT_CARS].x);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_CARS);
    EXPECT_EQ_INT(G->house.pickTrack, 1);   // pick a track first: right then left comes back to Green Hills
    press(G, CK_RIGHT, 0); EXPECT_EQ_INT(G->house.track, TRACK_DESERT);
    press(G, CK_LEFT, 0); EXPECT_EQ_INT(G->house.track, TRACK_HILLS);
    // a scripted driver steers into a lane that is clear ahead, on every track in turn
    float good = 0;
    for (int tr = 0; tr < CK_RACE_TRACKS; tr++) {
        EXPECT_EQ_INT(G->house.track, tr);
        EXPECT_EQ_INT(G->house.pickTrack, 1);
        press(G, CK_A, 0);
        hold(G, 200, 0, 0);   // the countdown
        EXPECT_EQ_INT(G->house.count <= 0, 1);
        for (int i = 0; i < 60 * 120 && G->house.win < 0; i++) {
            const ck_hhero *c = &G->house.gs[0];
            bool blocked[3] = { false, false, false };
            for (int k = 0; k < CK_RACE_OBST; k++) {
                float od, ox, w, h; int kind;
                ck_race_obst(G->house.track, k, G->house.clock, &od, &ox, &kind, &w, &h);
                if (od - c->a > -30 && od - c->a < 140)
                    for (int l = 0; l < 3; l++) if (fabsf(ox - (l - 1) * CK_RACE_LANE) < w / 2 + 14) blocked[l] = true;
            }
            int lane = (int)lroundf(c->x / CK_RACE_LANE) + 1, best = lane;
            for (int dl = 0; dl <= 2 && blocked[best]; dl++)
                for (int sgn = -1; sgn <= 1; sgn += 2) { int l = lane + sgn * dl; if (l >= 0 && l < 3 && !blocked[l]) { best = l; break; } }
            float tx = (best - 1) * CK_RACE_LANE;
            ck_game_step(G, CK_A | (tx > c->x + 3 ? CK_RIGHT : tx < c->x - 3 ? CK_LEFT : 0), 0);
        }
        EXPECT_EQ_INT(G->house.win, 0);
        int bumps = 0;
        for (int k = 0; k < CK_RACE_OBST; k++) bumps += (G->house.gs[0].hit >> k) & 1;
        if (bumps > 8) printf("  track %d: the scripted driver bumped %d times\n", tr, bumps);
        EXPECT_EQ_INT(bumps <= 8, 1);
        if (tr == 0) good = G->house.gs[0].best;
        hold(G, 60 * 5, 0, 0);   // then the next track comes up
    }
    // a driver who never steers drifts wide on the bends, bumps into things and is slower
    EXPECT_EQ_INT(G->house.track, TRACK_HILLS);
    press(G, CK_A, 0);
    hold(G, 200, 0, 0);   // the countdown
    // with no gas the car sits still; holding gas gets it going
    {
        const ck_hhero *c = &G->house.gs[0];
        float a0 = c->a;
        hold(G, 120, 0, 0);
        EXPECT_EQ_INT(c->a - a0 < 1, 1);
        hold(G, 120, CK_A, 0);
        EXPECT_EQ_INT(c->a - a0 > 200, 1);
        float s = c->s;
        hold(G, 60, 0, 0);   // let go and it slows down
        EXPECT_EQ_INT(c->s < s * 0.6f, 1);
    }
    for (int i = 0; i < 60 * 240 && G->house.win < 0; i++) ck_game_step(G, CK_A, 0);
    int bumps = 0;
    for (int k = 0; k < CK_RACE_OBST; k++) bumps += (G->house.gs[0].hit >> k) & 1;
    if (bumps < 3) printf("  the driver who never steers bumped %d times\n", bumps);
    EXPECT_EQ_INT(bumps >= 3, 1);
    EXPECT_EQ_INT(G->house.gs[0].best > good + 3, 1);
    // the road starts straight then bends both ways; a bend pushes a car that does not steer outwards
    {
        EXPECT_EQ_INT(ck_race_curve(TRACK_HILLS, 100) == 0, 1);
        float lo = 0, hi = 0;
        for (float z = 0; z < CK_RACE_LEN; z += 10) { float c = ck_race_curve(TRACK_HILLS, z); lo = fminf(lo, c); hi = fmaxf(hi, c); }
        EXPECT_EQ_INT(lo < -0.4f && hi > 0.4f, 1);
    }
    // a boost pad speeds you up; a ramp jumps you over what is just past it
    {
        hold(G, 60 * 5, 0, 0);
        press(G, CK_A, 0);
        hold(G, 200, 0, 0);
        ck_hhero *c = &G->house.gs[0];
        float pd, px; int kind;
        ck_race_pad(G->house.track, 0, &pd, &px, &kind);
        EXPECT_EQ_INT(kind, PAD_BOOST);
        c->a = pd - 20; c->x = px; c->s = 3; c->hit = ~0ull;
        hold(G, 20, CK_A, 0);
        EXPECT_EQ_INT(c->boost > 0, 1);
        hold(G, 40, CK_A, 0);
        EXPECT_EQ_INT(c->s > 4, 1);
        ck_race_pad(G->house.track, 2, &pd, &px, &kind);
        EXPECT_EQ_INT(kind, PAD_RAMP);
        c->a = pd - 20; c->x = px;
        float top = 0;
        for (int i = 0; i < 40; i++) { ck_game_step(G, CK_A, 0); c->x = px; top = fmaxf(top, c->jz); }
        EXPECT_EQ_INT(top > 10, 1);
        hold(G, 90, 0, 0);
        EXPECT_EQ_INT(c->jz, 0);
    }
    press(G, CK_START, 0);
    // the piano: A plays the key under the hand, the stick moves along the keys
    walk_to(G, ck_spots[SPOT_PIANO].x);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_PIANO);
    ck_game_step(G, CK_A, 0);
    EXPECT_EQ_INT(G->nnotes, 1);
    EXPECT_EQ_INT(fabsf(G->notes[0] - 261.63f * powf(2, 7 / 12.0f)) < 0.5f, 1);
    ck_game_step(G, 0, 0);
    press(G, CK_RIGHT, 0);
    EXPECT_EQ_INT(G->house.gs[0].key, 8);
    for (int i = 0; i < CK_KEYS_N; i++) { float x; bool b; ck_piano_key(i, &x, &b); EXPECT_EQ_INT(x > CK_KEYS_X && x < CK_KEYS_X + 14 * CK_KEYS_W, 1); }
    press(G, CK_START, 0);
    // the drums: each button is its own hit
    walk_to(G, ck_spots[SPOT_DRUMS].x);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_PIANO);
    EXPECT_EQ_INT(G->house.drums, 1);
    for (int k = 0; k < CK_DRUMS; k++) {
        ck_game_step(G, ck_drums[k].btn, 0);
        bool heard = false;
        for (int i = 0; i < G->nsfx; i++) heard |= G->sfx[i] == SFX_DRUM0 + k;
        EXPECT_EQ_INT(heard, 1);
        ck_game_step(G, 0, 0);
    }
    press(G, CK_START, 0);
    EXPECT_EQ_INT(G->scene, SC_HOUSE);
    // the mailbox has a letter, and the tree an apple
    walk_to(G, ck_spots[SPOT_MAIL].x);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->house.mode, HM_LETTER);   // a big letter to read
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->house.mode, HM_WALK);
    EXPECT_EQ_INT(G->house.ps[0].ground, 1);   // A there reads the mail, it does not jump
    walk_to(G, ck_spots[SPOT_TREE].x);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->house.ps[0].hold, FOOD_APPLE + 1);   // picked: held, not eaten yet
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->house.ps[0].eat > 0 && G->house.ps[0].col == FOOD_APPLE, 1);
    hold(G, 60, 0, 0);
    // the bed saves this game into a slot of your choice
    walk_to(G, ck_spots[SPOT_BED].x);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->house.mode, HM_SLOTS);
    press(G, CK_DOWN, 0);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->slot, 1);
    EXPECT_EQ_INT(G->saveNow, 1);
    EXPECT_EQ_INT(ck_slot_summary(G, 1).used, 1);
    EXPECT_EQ_INT(G->house.mode, HM_SLEEP);
    hold(G, 200, 0, 0);
    EXPECT_EQ_INT(G->house.mode, HM_WALK);
    // the arcade's pinball table: A fires the ball up the lane, the flippers flick it
    walk_to(G, ck_spots[SPOT_PINBALL].x);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_PINBALL);
    ck_pinball *P = &G->house.pin;
    ck_pball *b = &P->ball[0];
    EXPECT_EQ_INT(P->balls, CK_PIN_BALLS);
    EXPECT_EQ_INT(ck_pin_in_lane(b), 1);
    EXPECT_EQ_INT(P->cam > 100, 1);   // the view starts down at the flippers
    press(G, CK_A, 0);
    float topY = 999, leftX = 999, camLo = 999;
    for (int i = 0; i < 160; i += 4) { if (getenv("PINDBG")) printf("  %d: %.1f %.1f  v %.2f %.2f\n", i, b->x, b->y, b->vx, b->vy); hold(G, 4, 0, 0); topY = fminf(topY, b->y); leftX = fminf(leftX, b->x); camLo = fminf(camLo, P->cam); }
    EXPECT_EQ_INT(topY < 60, 1);   // up the lane...
    EXPECT_EQ_INT(leftX < 180, 1);   // ...and out onto the table
    EXPECT_EQ_INT(camLo < 20, 1);   // and the view scrolled up after it
    // a ball dropping on the left flipper gets flicked up
    *b = (ck_pball){ 96, 392, 0, 1, true, 0 };
    hold(G, 3, 0, 0);
    hold(G, 6, CK_B, 0);
    EXPECT_EQ_INT(P->flip[0] < 0, 1);
    EXPECT_EQ_INT(b->vy < -3, 1);
    hold(G, 20, 0, 0);
    // the spinner up the left orbit spins and scores
    int sc = P->score;
    *b = (ck_pball){ 22, 220, 0, -9, true, 0 };
    hold(G, 30, 0, 0);
    EXPECT_EQ_INT(P->spinA > 1, 1);
    EXPECT_EQ_INT(P->score > sc, 1);
    // a slingshot kicks the ball away
    *b = (ck_pball){ 50, 326, -3, 2, true, 0 };
    hold(G, 4, 0, 0);
    EXPECT_EQ_INT(P->sling[0] > 0 && b->vx > 0, 1);
    // knock down all four drop targets for double scoring
    for (int k = 0; k < CK_PIN_DROPS; k++) {
        *b = (ck_pball){ 190, (ck_pin_drops[k].y0 + ck_pin_drops[k].y1) / 2, 6, 0, true, 0 };
        hold(G, 6, 0, 0);
        EXPECT_EQ_INT((P->drops >> k) & 1, 1);
    }
    EXPECT_EQ_INT(P->x2T > 0 && P->msg == PINMSG_DOUBLE, 1);
    hold(G, 70, 0, 0);
    EXPECT_EQ_INT(P->drops, 0);   // the bank pops back up
    // the saucer holds the ball; the second catch starts multiball
    for (int n = 0; n < 2; n++) {
        *b = (ck_pball){ ck_pin_saucer[0] + 1, ck_pin_saucer[1] - 2, 0, 0.5f, true, 0 };
        hold(G, 2, 0, 0);
        EXPECT_EQ_INT(b->held > 0, 1);
        hold(G, 60, 0, 0);
        EXPECT_EQ_INT(b->held, 0);
    }
    int live = 0;
    for (int i = 0; i < CK_PIN_MULTI; i++) live += P->ball[i].on;
    EXPECT_EQ_INT(live, CK_PIN_MULTI);
    EXPECT_EQ_INT(P->balls, CK_PIN_BALLS);
    // play on with a flipper now and then until the balls run out; the score never goes down
    int last = P->score, falls = 0, spins = 0, drops = 0, catches = 0, hits = 0;
    for (int i = 0; i < 60 * 900 && !P->over; i++) {
        int b0 = P->balls, d0 = P->drops;
        bool spun = P->spin > 0;
        ck_game_step(G, (i / 7) % 5 == 0 ? CK_A : (i / 9) % 4 == 0 ? CK_B : 0, 0);
        EXPECT_EQ_INT(P->score >= last, 1);
        last = P->score;
        if (P->balls < b0) falls++;
        spins += P->spin > 0 && !spun;
        drops += P->drops != d0 && P->drops;
        for (int k = 0; k < CK_PIN_MULTI; k++) catches += P->ball[k].held == 49;
        for (int k = 0; k < CK_PIN_BUMPS; k++) hits += P->flash[k] == 11;
        bool out = false;
        for (int k = 0; k < CK_PIN_MULTI; k++) {
            const ck_pball *q = &P->ball[k];
            if (q->on && (q->y < 0 || q->x < 0 || q->x > CK_PIN_W)) { printf("  pinball: a ball got out at %.0f, %.0f\n", q->x, q->y); out = true; }
        }
        if (out) { EXPECT_EQ_INT(out, 0); break; }
    }
    EXPECT_EQ_INT(P->over, 1);
    EXPECT_EQ_INT(falls >= 1, 1);
    EXPECT_EQ_INT(P->best, P->score);
    printf("  pinball: %d points, %d bumps, %d spins, %d drops, %d saucer catches\n", P->score, hits, spins, drops, catches);
    hold(G, 70, 0, 0);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(P->over, 0);
    EXPECT_EQ_INT(P->balls, CK_PIN_BALLS);
    press(G, CK_START, 0);
    EXPECT_EQ_INT(G->scene, SC_HOUSE);
    // Bug Blaster: shoot the bottom bug in a column, clear a wave, and lose the cannons to bombs
    walk_to(G, ck_spots[SPOT_BLAST].x);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_BLAST);
    ck_blaster *B = &G->house.blast;
    hold(G, 95, 0, 0);   // "wave 1"
    float bx, by;
    ck_bug_at(B, 3 * CK_BUG_COLS + 2, &bx, &by);
    B->gun[0] = bx; B->stepT = -1000;   // hold the bugs still
    press(G, CK_A, 0);
    for (int i = 0; i < 60 && B->shot[0].on; i++) ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(B->bugs & (1u << (3 * CK_BUG_COLS + 2)), 0);
    EXPECT_EQ_INT(B->score, 10);
    B->bugs = 1u << 2;   // only one bug in the top row left
    ck_bug_at(B, 2, &bx, &by);
    B->gun[0] = bx;
    press(G, CK_A, 0);
    for (int i = 0; i < 60 && B->wave == 0; i++) ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(B->wave, 1);
    EXPECT_EQ_INT(B->bugs == 0xFFFFFFFFu, 1);
    EXPECT_EQ_INT(B->score, 50);
    hold(G, 95, 0, 0);
    B->stepT = -1000;
    B->bomb[0] = (ck_shot){ B->gun[0], CK_BLAST_Y - 4, true };
    ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(B->lives, 2);
    EXPECT_EQ_INT(B->inv[0] > 0, 1);
    B->lives = 1; B->inv[0] = 0;
    B->bomb[0] = (ck_shot){ B->gun[0], CK_BLAST_Y - 4, true };
    ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(B->over, 1);
    EXPECT_EQ_INT(B->best, 50);
    hold(G, 70, 0, 0);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(B->over, 0);
    EXPECT_EQ_INT(B->lives, 3);
    // left alone, the bugs march down to the shields: game over
    for (int i = 0; i < 60 * 120 && !B->over; i++) { ck_game_step(G, 0, 0); B->inv[0] = B->inv[1] = 10; }
    EXPECT_EQ_INT(B->over, 1);
    press(G, CK_START, 0);
    // the pond: cast, wait for a bite, hook it with A, reel it in
    walk_to(G, ck_spots[SPOT_POND].x);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_FISH);
    ck_pond *Pd = &G->house.pond;
    ck_rod *R = &Pd->rod[0];
    hold(G, 40, CK_A, 0);
    EXPECT_EQ_INT(R->state, ROD_CHARGE);
    ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(R->state, ROD_FLY);
    for (int i = 0; i < 200 && R->state == ROD_FLY; i++) ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(R->state, ROD_WAIT);
    EXPECT_EQ_INT(R->x > CK_POND_BANK + 40, 1);
    for (int k = 0; k < CK_POND_FISH; k++) { Pd->fish[k].x = 300; Pd->fish[k].scare = 9999; }
    Pd->fish[0] = (ck_pfish){ CATCH_GOLDFISH, R->x + 20, R->y + 10, -0.5f, 0 };
    for (int i = 0; i < 600 && R->state == ROD_WAIT; i++) ck_game_step(G, 0, 0);
    EXPECT_EQ_INT(R->state, ROD_BITE);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(R->state, ROD_REEL);
    int fishBolts0 = G->prog.bolts, fished0 = G->saves.closet.fished;
    float slant = 0;   // with a fish on, the bobber trails towards the rod: the line under it slants
    for (int i = 0; i < 3000 && R->state == ROD_REEL; i++) { ck_game_step(G, R->tension < 60 ? CK_A : 0, 0); slant = fmaxf(slant, fabsf(R->bob - R->x)); }
    EXPECT_EQ_INT(slant > 5, 1);
    EXPECT_EQ_INT(R->state, ROD_GOT);
    EXPECT_EQ_INT(Pd->caught, 1);
    EXPECT_EQ_INT(Pd->got[CATCH_GOLDFISH], 1);
    EXPECT_EQ_INT(G->prog.bolts, fishBolts0 + ck_catches[CATCH_GOLDFISH].bolts);
    EXPECT_EQ_INT(G->saves.closet.fished, fished0 + 1);   // kept in the save
    // holding A while a big fish pulls snaps the line
    hold(G, 160, 0, 0);
    EXPECT_EQ_INT(R->state, ROD_IDLE);
    hold(G, 40, CK_A, 0);
    for (int i = 0; i < 200 && R->state != ROD_WAIT; i++) ck_game_step(G, 0, 0);
    for (int k = 0; k < CK_POND_FISH; k++) { Pd->fish[k].x = 300; Pd->fish[k].scare = 9999; }
    Pd->fish[1] = (ck_pfish){ CATCH_BASS, R->x, R->y, 0.5f, 0 };
    for (int i = 0; i < 600 && R->state == ROD_WAIT; i++) ck_game_step(G, 0, 0);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(R->state, ROD_REEL);
    R->pulling = true; R->t = 1;
    for (int i = 0; i < 300 && R->state == ROD_REEL; i++) ck_game_step(G, CK_A, 0);
    EXPECT_EQ_INT(R->state, ROD_SNAP);
    EXPECT_EQ_INT(Pd->caught, 1);
    press(G, CK_START, 0);
    EXPECT_EQ_INT(G->scene, SC_HOUSE);
    // the pond levels up with the fish the family has caught: plain little fish first, bigger later
    EXPECT_EQ_INT(ck_pond_level(0), 0);
    EXPECT_EQ_INT(ck_pond_level(ck_pond_level_at[1]), 1);
    EXPECT_EQ_INT(ck_pond_level(9999), CK_POND_LEVELS - 1);
    for (int lv = 0; lv < CK_POND_LEVELS; lv += CK_POND_LEVELS - 1) {
        G->saves.closet.fished = (uint16_t)ck_pond_level_at[lv];
        press(G, CK_A, 0);
        EXPECT_EQ_INT(G->scene, SC_FISH);
        EXPECT_EQ_INT(Pd->level, lv);
        float biggest = 0;
        for (int n = 0; n < 20; n++) {   // restock a few times to see who lives there
            press(G, CK_START, 0); press(G, CK_A, 0);
            for (int k = 0; k < CK_POND_FISH; k++) if (Pd->fish[k].kind != CATCH_BOOT) biggest = fmaxf(biggest, ck_catches[Pd->fish[k].kind].len);
        }
        if (lv == 0) EXPECT_EQ_INT(biggest <= 7, 1);
        else EXPECT_EQ_INT(biggest >= 18, 1);
        press(G, CK_START, 0);
    }
    // catching the fish that reaches the next level restocks the pond
    G->saves.closet.fished = (uint16_t)(ck_pond_level_at[1] - 1);
    press(G, CK_A, 0);
    hold(G, 40, CK_A, 0);
    for (int i = 0; i < 200 && R->state != ROD_WAIT; i++) ck_game_step(G, 0, 0);
    for (int k = 0; k < CK_POND_FISH; k++) { Pd->fish[k].x = 300; Pd->fish[k].scare = 9999; }
    Pd->fish[0] = (ck_pfish){ CATCH_MINNOW, R->x, R->y, 0.5f, 0 };
    for (int i = 0; i < 600 && R->state == ROD_WAIT; i++) ck_game_step(G, 0, 0);
    press(G, CK_A, 0);
    for (int i = 0; i < 3000 && R->state == ROD_REEL; i++) ck_game_step(G, R->tension < 60 ? CK_A : 0, 0);
    EXPECT_EQ_INT(R->state, ROD_GOT);
    EXPECT_EQ_INT(Pd->level, 1);
    EXPECT_EQ_INT(Pd->levelT > 0, 1);
    press(G, CK_START, 0);
    // START (or walking off the yard) goes back to the map
    press(G, CK_START, 0);
    EXPECT_EQ_INT(G->scene, SC_MAP);
}

// Three save slots: the title picks one, and an empty slot is a new game.
static void test_slots(void) {
    ck_game *G = load_game();
    if (!G) return;
    static ck_saves three[CK_SLOTS];
    memset(three, 0, sizeof three);
    three[0] = *ck_game_saves(G);
    ck_game_slots(G, three);
    G->prog.bolts = 77;   // playing slot 1
    EXPECT_EQ_INT(ck_slot_summary(G, 0).used, 1);
    EXPECT_EQ_INT(ck_slot_summary(G, 2).used, 0);
    G->scene = SC_TITLE;
    press(G, CK_START, 0);
    EXPECT_EQ_INT(G->scene, SC_SLOTS);
    press(G, CK_UP, 0);   // slot 3
    EXPECT_EQ_INT(G->slotPick, 2);
    press(G, CK_A, 0);
    EXPECT_EQ_INT(G->scene, SC_SELECT);
    EXPECT_EQ_INT(G->slot, 2);
    EXPECT_EQ_INT(G->prog.bolts, 0);
    EXPECT_EQ_INT(G->slots[0].w[0].bolts, 77);   // slot 1 is left as it was
}

void test_clankers(void) {
    test_music();
    test_parse();
    test_world();
    test_level_rules();
    test_intro();
    test_autotest();
    test_fuzz();
    test_flow();
    test_bot_runs();
    test_new_worlds();
    test_saves();
    test_belts_and_gravity();
    test_bosses();
    test_films();
    test_new_bot_runs();
    test_movement();
    test_song_lengths();
    test_shop();
    test_bubble_gun();
    test_drop_player2();
    test_house();
    test_slots();
}

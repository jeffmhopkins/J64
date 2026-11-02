// Host tests for the Clankers game core (roms/clankers/src/core): the data files parse the way
// the browser reads them, and the rules behave (bolts, stomps, lives, the map opening up).
// Runs from the repo root so it reads the real data files.
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
    EXPECT_EQ_INT(G->scene >= SC_TITLE && G->scene <= SC_CUT, 1);
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
            EXPECT_EQ_INT(L->grav < 0.25f, n == 4);
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
    // the home node takes you back a world, to that world's exit
    G->prog.at = (uint8_t)ck_world_node(&G->world, "home");
    G->keepMap = false;
    ck_map_reset(G);
    ck_game_step(G, 0, 0);
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
            ck_game_step(G, n == 2 ? 0 : (f / 50) % 2 ? CK_LEFT : CK_RIGHT, 0);
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
}

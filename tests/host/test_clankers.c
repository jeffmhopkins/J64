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

#define DATA "roms/clankers/data/w1/"

static ck_game *load_game(void) {
    static ck_game G;
    char *w = slurp(DATA "world.txt");
    if (!w) return NULL;
    EXPECT_EQ_INT(ck_game_init(&G, w, NULL), 1);
    free(w);
    for (int i = 0; i < G.world.nnodes; i++) {
        if (!G.world.nodes[i].level[0]) continue;
        char path[96];
        snprintf(path, sizeof path, DATA "%s.txt", G.world.nodes[i].level);
        char *t = slurp(path);
        if (t) { ck_game_add_level(&G, G.world.nodes[i].level, t); free(t); }
    }
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
    EXPECT_EQ_INT(checks, 10);
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

void test_clankers(void) {
    test_music();
    test_parse();
    test_world();
    test_level_rules();
    test_intro();
    test_autotest();
    test_flow();
    test_bot_runs();
}
